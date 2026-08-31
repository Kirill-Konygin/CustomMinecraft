#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "VoxelInstanceCollision.generated.h"

class UBoxComponent;
class AVoxelTerrain;

UCLASS(ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class CUSTOMMINECRAFT_API UVoxelInstanceCollision final : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoxelInstanceCollision();

	void Initialize(float InVoxelSize);
	void SetPlayerPosition(const FVector& PlayerPosition);
	void Refresh();
	bool DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, float SphereRadius) const;

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	void UpdateCollisionBoxes(bool bForce);
	TArray<FIntVector> GetCollisionGridPositions(const AVoxelTerrain& Terrain, const FIntVector& CenterPosition) const;
	void SetCubes(TConstArrayView<FIntVector> GridPositions);
	void RemoveObsoleteCollisionBoxes(const TSet<FIntVector>& DesiredGridPositions);
	void AddMissingCollisionBoxes(const TSet<FIntVector>& DesiredGridPositions);
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
