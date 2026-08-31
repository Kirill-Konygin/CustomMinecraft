#include "VoxelInstanceRenderer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "CubeDefinition.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

UVoxelInstanceRenderer::UVoxelInstanceRenderer()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CubeMesh = CubeMeshFinder.Object;
	}
}

void UVoxelInstanceRenderer::Initialize(const FCubeDefinition& InCubeDefinition, const float InVoxelSize)
{
	check(InVoxelSize > 0.0f);
	VoxelSize = InVoxelSize;
	CreateVoxelMesh();

	if (VoxelMesh && InCubeDefinition.Material)
	{
		VoxelMesh->SetMaterial(0, InCubeDefinition.Material);
	}
}

void UVoxelInstanceRenderer::OnRegister()
{
	Super::OnRegister();
	CreateVoxelMesh();

	if (VoxelMesh && !VoxelMesh->IsRegistered())
	{
		if (const AActor* Owner = GetOwner())
		{
			if (const USceneComponent* RootComponent = Owner->GetRootComponent())
			{
				VoxelMesh->SetMobility(RootComponent->GetMobility());
				VoxelMesh->SetupAttachment(Owner->GetRootComponent());
			}
		}
		VoxelMesh->RegisterComponent();
	}
}

void UVoxelInstanceRenderer::OnUnregister()
{
	if (VoxelMesh && VoxelMesh->IsRegistered())
	{
		VoxelMesh->UnregisterComponent();
	}

	Super::OnUnregister();
}

void UVoxelInstanceRenderer::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	if (VoxelMesh && !VoxelMesh->IsBeingDestroyed())
	{
		VoxelMesh->DestroyComponent();
	}
	VoxelMesh = nullptr;

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UVoxelInstanceRenderer::CreateVoxelMesh()
{
	if (VoxelMesh && VoxelMesh->IsBeingDestroyed())
	{
		VoxelMesh = nullptr;
	}

	if (VoxelMesh || !IsRegistered())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	VoxelMesh = NewObject<UInstancedStaticMeshComponent>(this, TEXT("VoxelMesh"));
	VoxelMesh->SetCanEverAffectNavigation(false);
	VoxelMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	VoxelMesh->SetRemoveSwap();
	VoxelMesh->SetStaticMesh(CubeMesh);
	if (const USceneComponent* RootComponent = Owner->GetRootComponent())
	{
		VoxelMesh->SetMobility(RootComponent->GetMobility());
		VoxelMesh->SetupAttachment(Owner->GetRootComponent());
	}
	VoxelMesh->RegisterComponent();
	Owner->AddInstanceComponent(VoxelMesh);
}

bool UVoxelInstanceRenderer::IsReady() const
{
	return VoxelMesh && VoxelMesh->GetStaticMesh();
}

void UVoxelInstanceRenderer::SetCubes(const TConstArrayView<FIntVector> GridPositions)
{
	if (!IsReady())
	{
		return;
	}

	TSet<FIntVector> DesiredGridPositions;
	DesiredGridPositions.Reserve(GridPositions.Num());
	for (const FIntVector& GridPosition : GridPositions)
	{
		DesiredGridPositions.Add(GridPosition);
	}

	if (!RemoveObsoleteInstances(DesiredGridPositions))
	{
		return;
	}

	AddMissingInstances(DesiredGridPositions);
}

bool UVoxelInstanceRenderer::RemoveObsoleteInstances(const TSet<FIntVector>& DesiredGridPositions)
{
	TArray<int32> InstanceIndicesToRemove;
	InstanceIndicesToRemove.Reserve(InstanceIndexByGridPosition.Num());
	for (const auto& [GridPosition, InstanceIndex] : InstanceIndexByGridPosition)
	{
		if (!DesiredGridPositions.Contains(GridPosition))
		{
			InstanceIndicesToRemove.Add(InstanceIndex);
		}
	}

	if (InstanceIndicesToRemove.IsEmpty())
	{
		return true;
	}

	// Remove highest indices first so each removal does not invalidate the remaining indices.
	// The same order mirrors swap removal in GridPositionByInstanceIndex.
	InstanceIndicesToRemove.Sort(TGreater<int32>());
	if (!VoxelMesh->RemoveInstances(InstanceIndicesToRemove, true))
	{
		return false;
	}

	for (const int32 InstanceIndex : InstanceIndicesToRemove)
	{
		if (GridPositionByInstanceIndex.IsValidIndex(InstanceIndex))
		{
			GridPositionByInstanceIndex.RemoveAtSwap(InstanceIndex, 1, EAllowShrinking::No);
		}
	}

	RebuildInstanceIndexLookup();
	return true;
}

void UVoxelInstanceRenderer::RebuildInstanceIndexLookup()
{
	InstanceIndexByGridPosition.Reset();
	InstanceIndexByGridPosition.Reserve(GridPositionByInstanceIndex.Num());
	for (int32 InstanceIndex = 0; InstanceIndex < GridPositionByInstanceIndex.Num(); ++InstanceIndex)
	{
		InstanceIndexByGridPosition.Add(GridPositionByInstanceIndex[InstanceIndex], InstanceIndex);
	}
}

void UVoxelInstanceRenderer::AddMissingInstances(const TSet<FIntVector>& DesiredGridPositions)
{
	TArray<FTransform> Transforms;
	TArray<FIntVector> GridPositionsToAdd;
	Transforms.Reserve(DesiredGridPositions.Num());
	GridPositionsToAdd.Reserve(DesiredGridPositions.Num());

	for (const FIntVector& GridPosition : DesiredGridPositions)
	{
		if (InstanceIndexByGridPosition.Contains(GridPosition))
		{
			continue;
		}

		GridPositionsToAdd.Add(GridPosition);
		Transforms.Emplace(MakeCubeTransform(GridPosition));
	}

	if (Transforms.IsEmpty())
	{
		return;
	}

	const TArray<int32> NewInstanceIndices = VoxelMesh->AddInstances(Transforms, true, false, false);
	GridPositionByInstanceIndex.SetNum(VoxelMesh->GetInstanceCount());

	for (int32 Index = 0; Index < NewInstanceIndices.Num(); ++Index)
	{
		const int32 InstanceIndex = NewInstanceIndices[Index];
		if (!GridPositionsToAdd.IsValidIndex(Index) || !GridPositionByInstanceIndex.IsValidIndex(InstanceIndex))
		{
			continue;
		}

		const FIntVector& GridPosition = GridPositionsToAdd[Index];
		InstanceIndexByGridPosition.Add(GridPosition, InstanceIndex);
		GridPositionByInstanceIndex[InstanceIndex] = GridPosition;
	}
}

FTransform UVoxelInstanceRenderer::MakeCubeTransform(const FIntVector& GridPosition) const
{
	const FVector Location(
		static_cast<double>(GridPosition.X) * VoxelSize,
		static_cast<double>(GridPosition.Y) * VoxelSize,
		static_cast<double>(GridPosition.Z) * VoxelSize);
	const FVector Scale = FVector::OneVector * (VoxelSize / 100.0f);

	return FTransform(FRotator::ZeroRotator, Location, Scale);
}
