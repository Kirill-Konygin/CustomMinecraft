#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "VoxelInstanceManager.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
struct FCubeDefinition;

struct FVoxelInstanceHit
{
	FIntVector Position = FIntVector::ZeroValue;
	FIntVector Normal = FIntVector::ZeroValue;
	float Distance = 0.0f;
};

UCLASS(ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class CUSTOMMINECRAFT_API UVoxelInstanceManager final : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoxelInstanceManager();

	void Initialize(const FCubeDefinition& InCubeDefinition, float InVoxelSize);
	void SetCubes(TConstArrayView<FIntVector> GridPositions);
	bool DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter,float SphereRadius) const;
	TOptional<FVoxelInstanceHit> TraceVoxel(const FVector& Start, const FVector& End) const;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	void CreateVoxelMesh();
	bool IsReady() const;
	FTransform MakeCubeTransform(const FIntVector& GridPosition) const;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> VoxelMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	float VoxelSize = 100.0f;
	TMap<FIntVector, int32> InstanceIndexByGridPosition;
	TArray<FIntVector> GridPositionByInstanceIndex;
};
