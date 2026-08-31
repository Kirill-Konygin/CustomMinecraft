#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "VoxelInstanceCollision.generated.h"

class UBoxComponent;

UCLASS(ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class CUSTOMMINECRAFT_API UVoxelInstanceCollision final : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoxelInstanceCollision();

	void Initialize(float InVoxelSize);
	void SetPlayerPosition(const FVector& PlayerPosition);
	void Refresh();

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	void UpdateCollisionBoxes(bool bForce);
	void SetCubes(TConstArrayView<FIntVector> GridPositions);
	UBoxComponent* AcquireCollisionBox();
	void ReleaseCollisionBox(UBoxComponent* CollisionBox);

	UPROPERTY(Transient)
	TMap<FIntVector, TObjectPtr<UBoxComponent>> CollisionBoxesByGridPosition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> AvailableCollisionBoxes;

	UPROPERTY(EditAnywhere, Category = "Voxel Collision", meta = (ClampMin = "1"))
	int32 RadiusXY = 2;

	UPROPERTY(EditAnywhere, Category = "Voxel Collision", meta = (ClampMin = "1"))
	int32 RadiusZ = 3;

	float VoxelSize = 100.0f;
	FVector LastPlayerPosition = FVector::ZeroVector;
	FIntVector CenterGridPosition = FIntVector::ZeroValue;
};
