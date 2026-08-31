// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerPawn.generated.h"

class AVoxelTerrain;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;
class UPawnMovementComponent;
class USphereComponent;
struct FInputActionValue;

UCLASS()
class CUSTOMMINECRAFT_API APlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APlayerPawn();
	virtual UPawnMovementComponent* GetMovementComponent() const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void AddCube();
	void MineCube();
	bool GetInteractionRay(FVector& OutStart, FVector& OutEnd) const;
	void ResetMining();

	void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Input", meta = (ClampMin = "0.0"))
	float LookSensitivity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> MineCubeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> AddCubeAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Interaction", meta = (ClampMin = "0.0"))
	float InteractionDistance = 500.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Player|Interaction")
	TObjectPtr<AVoxelTerrain> VoxelTerrain;

	TOptional<FIntVector> CurrentMiningVoxel;
	float MiningTimeRemaining = 0.f;
	float MiningDuration = 0.f;

public:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure)
	bool IsMining() const;

	UFUNCTION(BlueprintPure)
	float GetMiningProgress() const;
};
