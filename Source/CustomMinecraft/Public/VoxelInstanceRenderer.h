#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "VoxelInstanceRenderer.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
struct FCubeDefinition;

UCLASS(ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class CUSTOMMINECRAFT_API UVoxelInstanceRenderer final : public UActorComponent
{
	GENERATED_BODY()

public:
	UVoxelInstanceRenderer();

	void Initialize(const FCubeDefinition& InCubeDefinition, float InVoxelSize);
	void SetCubes(TConstArrayView<FIntVector> GridPositions);

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	void CreateVoxelMesh();
	bool IsReady() const;
	const TArray<FTransform> MakeCubeTransforms(const TArray<FIntVector>& GridPositions) const;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> VoxelMeshComponent;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	float VoxelSize = 100.0f;
	TMap<FIntVector, FPrimitiveInstanceId> InstanceIdByGridPosition;
};
