#include "VoxelInstanceCollision.h"

#include "AVoxelTerrain.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Actor.h"

UVoxelInstanceCollision::UVoxelInstanceCollision()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVoxelInstanceCollision::Initialize(const float InVoxelSize)
{
	check(InVoxelSize > 0.0f);
	VoxelSize = InVoxelSize;
}

void UVoxelInstanceCollision::SetPlayerPosition(const FVector& PlayerPosition)
{
	LastPlayerPosition = PlayerPosition;
	UpdateCollisionBoxes(false);
}

void UVoxelInstanceCollision::Refresh()
{
	UpdateCollisionBoxes(true);
}

bool UVoxelInstanceCollision::DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, const float SphereRadius) const
{
	const AActor* Owner = GetOwner();
	const USceneComponent* RootComponent = Owner ? Owner->GetRootComponent() : nullptr;
	if (!RootComponent || VoxelSize <= 0.0f)
	{
		return false;
	}

	const FVector LocalCenter(
		static_cast<double>(GridPosition.X) * VoxelSize,
		static_cast<double>(GridPosition.Y) * VoxelSize,
		static_cast<double>(GridPosition.Z) * VoxelSize);
	const FVector Extent = FVector::OneVector * VoxelSize * 0.5f;
	const FBox WorldBounds = FBox::BuildAABB(LocalCenter, Extent).TransformBy(RootComponent->GetComponentTransform());

	return FMath::SphereAABBIntersection(SphereCenter, FMath::Square(static_cast<double>(SphereRadius)), WorldBounds);
}

void UVoxelInstanceCollision::UpdateCollisionBoxes(const bool bForce)
{
	const AVoxelTerrain* Terrain = Cast<AVoxelTerrain>(GetOwner());
	if (!Terrain || VoxelSize <= 0.0f)
	{
		return;
	}

	const FVector LocalPosition = Terrain->GetActorTransform().InverseTransformPosition(LastPlayerPosition) / VoxelSize;
	const FIntVector NewCenterGridPosition(FMath::FloorToInt(LocalPosition.X + 0.5), FMath::FloorToInt(LocalPosition.Y + 0.5), FMath::FloorToInt(LocalPosition.Z + 0.5));
	if (!bForce && CenterGridPosition == NewCenterGridPosition)
	{
		return;
	}

	CenterGridPosition = NewCenterGridPosition;
	SetCubes(GetCollisionGridPositions(*Terrain, CenterGridPosition));
}

TArray<FIntVector> UVoxelInstanceCollision::GetCollisionGridPositions(
	const AVoxelTerrain& Terrain,
	const FIntVector& CenterPosition) const
{
	TArray<FIntVector> CollisionGridPositions;
	CollisionGridPositions.Reserve((2 * RadiusXY + 1) * (2 * RadiusXY + 1) * (2 * RadiusZ + 1));

	for (int32 X = -RadiusXY; X <= RadiusXY; ++X)
	{
		for (int32 Y = -RadiusXY; Y <= RadiusXY; ++Y)
		{
			for (int32 Z = -RadiusZ; Z <= RadiusZ; ++Z)
			{
				const FIntVector GridPosition = CenterPosition + FIntVector(X, Y, Z);
				if (GridPosition.Z >= 0 && Terrain.HasVoxel(GridPosition))
				{
					CollisionGridPositions.Add(GridPosition);
				}
			}
		}
	}

	return CollisionGridPositions;
}

void UVoxelInstanceCollision::SetCubes(const TConstArrayView<FIntVector> GridPositions)
{
	if (!IsRegistered())
	{
		return;
	}

	TSet<FIntVector> DesiredGridPositions;
	DesiredGridPositions.Reserve(GridPositions.Num());
	for (const FIntVector& GridPosition : GridPositions)
	{
		DesiredGridPositions.Add(GridPosition);
	}

	RemoveObsoleteCollisionBoxes(DesiredGridPositions);
	AddMissingCollisionBoxes(DesiredGridPositions);
}

void UVoxelInstanceCollision::RemoveObsoleteCollisionBoxes(const TSet<FIntVector>& DesiredGridPositions)
{
	TArray<FIntVector> GridPositionsToRemove;
	GridPositionsToRemove.Reserve(CollisionBoxesByGridPosition.Num());
	for (const auto& CollisionBoxPair : CollisionBoxesByGridPosition)
	{
		if (!DesiredGridPositions.Contains(CollisionBoxPair.Key))
		{
			GridPositionsToRemove.Add(CollisionBoxPair.Key);
		}
	}

	for (const FIntVector& GridPosition : GridPositionsToRemove)
	{
		UBoxComponent* CollisionBox = CollisionBoxesByGridPosition.FindRef(GridPosition);
		CollisionBoxesByGridPosition.Remove(GridPosition);
		ReleaseCollisionBox(CollisionBox);
	}
}

void UVoxelInstanceCollision::AddMissingCollisionBoxes(const TSet<FIntVector>& DesiredGridPositions)
{
	for (const FIntVector& GridPosition : DesiredGridPositions)
	{
		if (CollisionBoxesByGridPosition.Contains(GridPosition))
		{
			continue;
		}

		UBoxComponent* CollisionBox = AcquireCollisionBox();
		if (!CollisionBox)
		{
			continue;
		}

		const FVector Location(static_cast<double>(GridPosition.X) * VoxelSize, static_cast<double>(GridPosition.Y) * VoxelSize, static_cast<double>(GridPosition.Z) * VoxelSize);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionBox->SetRelativeLocation(Location);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionBoxesByGridPosition.Add(GridPosition, CollisionBox);
	}
}

void UVoxelInstanceCollision::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	for (const auto& CollisionBoxPair : CollisionBoxesByGridPosition)
	{
		if (CollisionBoxPair.Value && !CollisionBoxPair.Value->IsBeingDestroyed())
		{
			CollisionBoxPair.Value->DestroyComponent();
		}
	}

	for (UBoxComponent* CollisionBox : AvailableCollisionBoxes)
	{
		if (CollisionBox && !CollisionBox->IsBeingDestroyed())
		{
			CollisionBox->DestroyComponent();
		}
	}

	CollisionBoxesByGridPosition.Reset();
	AvailableCollisionBoxes.Reset();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

UBoxComponent* UVoxelInstanceCollision::AcquireCollisionBox()
{
	if (!AvailableCollisionBoxes.IsEmpty())
	{
		return AvailableCollisionBoxes.Pop(EAllowShrinking::No);
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetRootComponent())
	{
		return nullptr;
	}

	UBoxComponent* CollisionBox = NewObject<UBoxComponent>(this);
	CollisionBox->SetMobility(EComponentMobility::Movable);
	CollisionBox->SetCanEverAffectNavigation(false);
	CollisionBox->SetGenerateOverlapEvents(false);
	CollisionBox->SetBoxExtent(FVector::OneVector * VoxelSize * 0.5f, false);
	CollisionBox->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionBox->SetupAttachment(Owner->GetRootComponent());
	CollisionBox->RegisterComponent();
	Owner->AddInstanceComponent(CollisionBox);
	return CollisionBox;
}

void UVoxelInstanceCollision::ReleaseCollisionBox(UBoxComponent* CollisionBox)
{
	if (!CollisionBox)
	{
		return;
	}

	CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AvailableCollisionBoxes.Add(CollisionBox);
}
