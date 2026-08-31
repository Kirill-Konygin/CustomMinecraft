#include "VoxelInstanceManager.h"

#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "CubeDefinition.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

UVoxelInstanceManager::UVoxelInstanceManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CubeMesh = CubeMeshFinder.Object;
	}
}

void UVoxelInstanceManager::Initialize(const FCubeDefinition& InCubeDefinition, const float InVoxelSize)
{
	check(InVoxelSize > 0.0f);
	VoxelSize = InVoxelSize;
	CreateVoxelMesh();

	if (VoxelMesh && InCubeDefinition.Material)
	{
		VoxelMesh->SetMaterial(0, InCubeDefinition.Material);
	}
}

void UVoxelInstanceManager::OnRegister()
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

void UVoxelInstanceManager::OnUnregister()
{
	if (VoxelMesh && VoxelMesh->IsRegistered())
	{
		VoxelMesh->UnregisterComponent();
	}

	Super::OnUnregister();
}

void UVoxelInstanceManager::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	if (VoxelMesh && !VoxelMesh->IsBeingDestroyed())
	{
		VoxelMesh->DestroyComponent();
	}
	VoxelMesh = nullptr;

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UVoxelInstanceManager::CreateVoxelMesh()
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
	VoxelMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
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

bool UVoxelInstanceManager::IsReady() const
{
	return VoxelMesh && VoxelMesh->GetStaticMesh();
}

void UVoxelInstanceManager::SetCubes(const TConstArrayView<FIntVector> GridPositions)
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

	TArray<int32> InstanceIndicesToRemove;
	InstanceIndicesToRemove.Reserve(InstanceIndexByGridPosition.Num());
	for (const auto& [GridPosition, InstanceIndex] : InstanceIndexByGridPosition)
	{
		if (!DesiredGridPositions.Contains(GridPosition))
		{
			InstanceIndicesToRemove.Add(InstanceIndex);
		}
	}

	if (!InstanceIndicesToRemove.IsEmpty())
	{
		InstanceIndicesToRemove.Sort(TGreater<int32>());
		if (!VoxelMesh->RemoveInstances(InstanceIndicesToRemove, true))
		{
			return;
		}

		for (const int32 InstanceIndex : InstanceIndicesToRemove)
		{
			if (GridPositionByInstanceIndex.IsValidIndex(InstanceIndex))
			{
				GridPositionByInstanceIndex.RemoveAtSwap(InstanceIndex, 1, EAllowShrinking::No);
			}
		}

		InstanceIndexByGridPosition.Reset();
		InstanceIndexByGridPosition.Reserve(GridPositionByInstanceIndex.Num());
		for (int32 InstanceIndex = 0; InstanceIndex < GridPositionByInstanceIndex.Num(); ++InstanceIndex)
		{
			InstanceIndexByGridPosition.Add(GridPositionByInstanceIndex[InstanceIndex], InstanceIndex);
		}
	}

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

bool UVoxelInstanceManager::DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, const float SphereRadius) const
{
	if (!IsReady())
	{
		return false;
	}

	const FTransform CubeWorldTransform = MakeCubeTransform(GridPosition) * VoxelMesh->GetComponentTransform();
	const FBox CubeBounds = VoxelMesh->GetStaticMesh()->GetBoundingBox().TransformBy(CubeWorldTransform);

	return FMath::SphereAABBIntersection(SphereCenter, FMath::Square(static_cast<double>(SphereRadius)), CubeBounds);
}

TOptional<FVoxelInstanceHit> UVoxelInstanceManager::TraceVoxel(const FVector& Start, const FVector& End) const
{
	if (!IsReady())
	{
		return {};
	}

	FHitResult HitResult;
	if (	!VoxelMesh->LineTraceComponent(HitResult, Start, End, FCollisionQueryParams::DefaultQueryParam) 
		||	!GridPositionByInstanceIndex.IsValidIndex(HitResult.Item))
	{
		return {};
	}

	const FVector LocalNormal = VoxelMesh->GetComponentTransform().InverseTransformVectorNoScale(HitResult.ImpactNormal);

	return FVoxelInstanceHit{	GridPositionByInstanceIndex[HitResult.Item],	FIntVector( FMath::RoundToInt(LocalNormal.X),
								FMath::RoundToInt(LocalNormal.Y),				FMath::RoundToInt(LocalNormal.Z)),
								HitResult.Distance};
}

FTransform UVoxelInstanceManager::MakeCubeTransform(const FIntVector& GridPosition) const
{
	const FVector Location(
		static_cast<double>(GridPosition.X) * VoxelSize,
		static_cast<double>(GridPosition.Y) * VoxelSize,
		static_cast<double>(GridPosition.Z) * VoxelSize);
	const FVector Scale = FVector::OneVector * (VoxelSize / 100.0f);

	return FTransform(FRotator::ZeroRotator, Location, Scale);
}
