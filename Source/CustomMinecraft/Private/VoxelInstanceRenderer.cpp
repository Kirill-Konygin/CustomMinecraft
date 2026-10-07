#include "VoxelInstanceRenderer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "CubeDefinition.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
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
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_Initialize);

	check(InVoxelSize > 0.0f);
	VoxelSize = InVoxelSize;
	CreateVoxelMesh();

	if (VoxelMeshComponent && InCubeDefinition.Material)
	{
		VoxelMeshComponent->SetMaterial(0, InCubeDefinition.Material);
	}
}

void UVoxelInstanceRenderer::OnRegister()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_OnRegister);

	Super::OnRegister();
	CreateVoxelMesh();

	if (VoxelMeshComponent && !VoxelMeshComponent->IsRegistered())
	{
		if (const AActor* Owner = GetOwner())
		{
			if (const USceneComponent* RootComponent = Owner->GetRootComponent())
			{
				VoxelMeshComponent->SetMobility(RootComponent->GetMobility());
				VoxelMeshComponent->SetupAttachment(Owner->GetRootComponent());
			}
		}
		VoxelMeshComponent->RegisterComponent();
	}
}

void UVoxelInstanceRenderer::OnUnregister()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_OnUnregister);

	if (VoxelMeshComponent && VoxelMeshComponent->IsRegistered())
	{
		VoxelMeshComponent->UnregisterComponent();
	}

	Super::OnUnregister();
}

void UVoxelInstanceRenderer::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_OnComponentDestroyed);

	if (VoxelMeshComponent && !VoxelMeshComponent->IsBeingDestroyed())
	{
		VoxelMeshComponent->DestroyComponent();
	}
	VoxelMeshComponent = nullptr;

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UVoxelInstanceRenderer::CreateVoxelMesh()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_CreateVoxelMesh);

	if (VoxelMeshComponent && VoxelMeshComponent->IsBeingDestroyed())
	{
		VoxelMeshComponent = nullptr;
	}

	if (VoxelMeshComponent || !IsRegistered())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	VoxelMeshComponent = NewObject<UInstancedStaticMeshComponent>(this, TEXT("VoxelMesh"));
	VoxelMeshComponent->SetCanEverAffectNavigation(false);
	VoxelMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	VoxelMeshComponent->SetRemoveSwap();
	VoxelMeshComponent->SetStaticMesh(CubeMesh);
	if (const USceneComponent* RootComponent = Owner->GetRootComponent())
	{
		VoxelMeshComponent->SetMobility(RootComponent->GetMobility());
		VoxelMeshComponent->SetupAttachment(Owner->GetRootComponent());
	}
	VoxelMeshComponent->RegisterComponent();
	Owner->AddInstanceComponent(VoxelMeshComponent);
}

bool UVoxelInstanceRenderer::IsReady() const
{
	return VoxelMeshComponent && VoxelMeshComponent->GetStaticMesh();
}

void UVoxelInstanceRenderer::AddCubes(const TConstArrayView<FIntVector> GridPositions)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_AddCubes);

	if (!IsReady() || GridPositions.IsEmpty())
	{
		return;
	}

	TSet<FIntVector> NewGridPositions;
	NewGridPositions.Reserve(GridPositions.Num());
	for (const FIntVector& GridPosition : GridPositions)
	{
		if (!InstanceIdByGridPosition.Contains(GridPosition))
		{
			NewGridPositions.Add(GridPosition);
		}
	}

	if (NewGridPositions.IsEmpty())
	{
		return;
	}

	const TArray<FIntVector> GridPositionsToAdd = NewGridPositions.Array();
	const auto AddedInstanceIds = VoxelMeshComponent->AddInstancesById(MakeCubeTransforms(GridPositionsToAdd));
	for (int32 Index = 0; Index < AddedInstanceIds.Num(); ++Index)
	{
		InstanceIdByGridPosition.Add(GridPositionsToAdd[Index], AddedInstanceIds[Index]);
	}
}

void UVoxelInstanceRenderer::RemoveCubes(const TConstArrayView<FIntVector> GridPositions)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_RemoveCubes);

	if (!IsReady() || GridPositions.IsEmpty())
	{
		return;
	}

	TArray<FPrimitiveInstanceId> InstanceIdsToRemove;
	InstanceIdsToRemove.Reserve(GridPositions.Num());
	for (const FIntVector& GridPosition : GridPositions)
	{
		FPrimitiveInstanceId InstanceId;
		if (InstanceIdByGridPosition.RemoveAndCopyValue(GridPosition, InstanceId))
		{
			InstanceIdsToRemove.Add(InstanceId);
		}
	}

	if (!InstanceIdsToRemove.IsEmpty())
	{
		VoxelMeshComponent->RemoveInstancesById(InstanceIdsToRemove);
	}
}

const TArray<FTransform> UVoxelInstanceRenderer::MakeCubeTransforms(const TArray<FIntVector>& GridPositions) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVoxelInstanceRenderer_MakeCubeTransforms);

	TArray<FTransform> Transforms;
	const FVector Scale = FVector::OneVector * (VoxelSize / 100.0f);
	for (const auto& GridPosition : GridPositions) {
		const FVector Location(
			static_cast<double>(GridPosition.X) * VoxelSize,
			static_cast<double>(GridPosition.Y) * VoxelSize,
			static_cast<double>(GridPosition.Z) * VoxelSize);
		Transforms.Emplace(FRotator::ZeroRotator, Location, Scale);
	}

	return Transforms;
}
