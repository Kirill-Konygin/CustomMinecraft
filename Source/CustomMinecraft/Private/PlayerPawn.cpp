// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerPawn.h"
#include "AVoxelTerrain.h"
#include "Components/SphereComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/CollisionProfile.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APlayerPawn::APlayerPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(50.0f);
	CollisionSphere->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
}

UPawnMovementComponent* APlayerPawn::GetMovementComponent() const
{
	return MovementComponent;
}

// Called when the game starts or when spawned
void APlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!VoxelTerrain)
	{
		VoxelTerrain = Cast<AVoxelTerrain>(
			UGameplayStatics::GetActorOfClass(this, AVoxelTerrain::StaticClass()));
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	if (InputMappingContext)
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

	PlayerController->SetShowMouseCursor(false);
	PlayerController->SetInputMode(FInputModeGameOnly());
}

// Called every frame
void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}
	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerPawn::Move);
	}
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerPawn::Look);
	}
	if (MineCubeAction)
	{
		EnhancedInputComponent->BindAction(MineCubeAction, ETriggerEvent::Triggered, this, &APlayerPawn::MineCube);
		EnhancedInputComponent->BindAction(MineCubeAction, ETriggerEvent::Completed, this, &APlayerPawn::ResetMining); 
		EnhancedInputComponent->BindAction(MineCubeAction, ETriggerEvent::Canceled, this, &APlayerPawn::ResetMining);
	}
	if (AddCubeAction)
	{
		EnhancedInputComponent->BindAction(AddCubeAction, ETriggerEvent::Started, this, &APlayerPawn::AddCube);
	}
}

bool APlayerPawn::IsMining() const
{
	return CurrentMiningVoxel.IsSet();
}

float APlayerPawn::GetMiningProgress() const
{
	if (!IsMining()) 
	{
		return 0.f;
	}

	return FMath::Clamp(1.0f - MiningTimeRemaining / MiningDuration, 0.0f, 1.0f);
}

void APlayerPawn::Move(const FInputActionValue& Value)
{
	const FVector MoveValue = Value.Get<FVector>();

	if (const AController* PlayerController = GetController())
	{
		const FRotator ControlRotation = PlayerController->GetControlRotation();
		const FRotator YawRotation(0.0, ControlRotation.Yaw, 0.0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MoveValue.X);
		AddMovementInput(RightDirection, MoveValue.Y);
		AddMovementInput(FVector::UpVector, MoveValue.Z);
	}
}

void APlayerPawn::Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();
	AddControllerYawInput(LookValue.X * LookSensitivity);
	AddControllerPitchInput(LookValue.Y * LookSensitivity);
}

bool APlayerPawn::GetInteractionRay(FVector& OutStart, FVector& OutEnd) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return false;
	}

	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(OutStart, ViewRotation);
	OutEnd = OutStart + ViewRotation.Vector() * InteractionDistance;

	return true;
}

void APlayerPawn::ResetMining()
{
	CurrentMiningVoxel.Reset();
	MiningTimeRemaining = MiningDuration;
}

void APlayerPawn::AddCube()
{
	FVector TraceStart;
	FVector TraceEnd;
	if (!VoxelTerrain || !GetInteractionRay(TraceStart, TraceEnd))
	{
		return;
	}

	if (const TOptional<FVoxelHit> VoxelHit = VoxelTerrain->TraceVoxel(TraceStart, TraceEnd))
	{
		const FIntVector GridPosition = VoxelHit->Position + VoxelHit->Normal;
		if (!VoxelTerrain->DoesCubeOverlapSphere(GridPosition, CollisionSphere->GetComponentLocation(), CollisionSphere->GetScaledSphereRadius()))
		{
			VoxelTerrain->AddCube(GridPosition);
		}
	}
}

void APlayerPawn::MineCube()
{
	FVector TraceStart;
	FVector TraceEnd;
	if (!VoxelTerrain || !GetInteractionRay(TraceStart, TraceEnd))
	{
		ResetMining();
		return;
	}

	const TOptional<FVoxelHit> VoxelHit = VoxelTerrain->TraceVoxel(TraceStart, TraceEnd);
	if (!VoxelHit)
	{
		ResetMining();
		return;
	}

	const FIntVector& VoxelPosition = VoxelHit->Position;

	if (!CurrentMiningVoxel || CurrentMiningVoxel.GetValue() != VoxelPosition)
	{
		CurrentMiningVoxel = VoxelPosition;
		MiningTimeRemaining = MiningDuration;
		return;
	}

	MiningTimeRemaining -= GetWorld()->GetDeltaSeconds();

	if (MiningTimeRemaining <= 0.f) {
		VoxelTerrain->RemoveCube(VoxelPosition);
		ResetMining();
	}
}

