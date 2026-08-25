// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

// Sets default values
APlayerPawn::APlayerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

UPawnMovementComponent* APlayerPawn::GetMovementComponent() const
{
	return MovementComponent;
}

// Called when the game starts or when spawned
void APlayerPawn::BeginPlay()
{
	Super::BeginPlay();

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
	AddControllerPitchInput(-LookValue.Y * LookSensitivity);
}

