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

	if (VoxelMeshComponent && InCubeDefinition.Material)
	{
		VoxelMeshComponent->SetMaterial(0, InCubeDefinition.Material);
	}
}

void UVoxelInstanceRenderer::OnRegister()
{
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
	if (VoxelMeshComponent && VoxelMeshComponent->IsRegistered())
	{
		VoxelMeshComponent->UnregisterComponent();
	}

	Super::OnUnregister();
}

void UVoxelInstanceRenderer::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	if (VoxelMeshComponent && !VoxelMeshComponent->IsBeingDestroyed())
	{
		VoxelMeshComponent->DestroyComponent();
	}
	VoxelMeshComponent = nullptr;

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UVoxelInstanceRenderer::CreateVoxelMesh()
{
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

void UVoxelInstanceRenderer::SetCubes(const TConstArrayView<FIntVector> GridPositions)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SetCubes);
	if (!IsReady())
	{
		return;
	}

	TSet<FIntVector> DesiredGridPositions(GridPositions);
	TSet<FIntVector> CurrentGridPositions;
	CurrentGridPositions.Reserve(InstanceIdByGridPosition.Num());
	InstanceIdByGridPosition.GetKeys(CurrentGridPositions);

	TArray<FPrimitiveInstanceId> InstanceIdsToRemove;
	
	const TSet<FIntVector> GridPositionsToRemove = CurrentGridPositions.Difference(DesiredGridPositions);
	const TArray<FIntVector> GridPositionsToAdd = DesiredGridPositions.Difference(CurrentGridPositions).Array();

	InstanceIdsToRemove.Reserve(GridPositionsToRemove.Num());
	for (const auto& GridPosition : GridPositionsToRemove) {
		InstanceIdsToRemove.Add(InstanceIdByGridPosition.FindRef(GridPosition));
		InstanceIdByGridPosition.Remove(GridPosition);
	}
	VoxelMeshComponent->RemoveInstancesById(InstanceIdsToRemove);

	const auto AddedInstanceIds = VoxelMeshComponent->AddInstancesById(MakeCubeTransforms(GridPositionsToAdd));
	for (int32 Index = 0; Index < AddedInstanceIds.Num(); ++Index) {
		InstanceIdByGridPosition.Add(GridPositionsToAdd[Index], AddedInstanceIds[Index]);
	}
}

const TArray<FTransform> UVoxelInstanceRenderer::MakeCubeTransforms(const TArray<FIntVector>& GridPositions) const
{
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
