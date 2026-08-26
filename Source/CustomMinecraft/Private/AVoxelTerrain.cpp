// Fill out your copyright notice in the Description page of Project Settings.


#include "AVoxelTerrain.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AVoxelTerrain::AVoxelTerrain()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	VoxelMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("VoxelMesh"));
	SetRootComponent(VoxelMesh);
	VoxelMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	VoxelMesh->SetMobility(EComponentMobility::Static);
	VoxelMesh->SetRemoveSwap();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (CubeMesh.Succeeded())
	{
		VoxelMesh->SetStaticMesh(CubeMesh.Object);
	}
}

// Called when the game starts or when spawned
void AVoxelTerrain::BeginPlay()
{
	Super::BeginPlay();

	GenerateTerrain();
}

void AVoxelTerrain::GenerateTerrain()
{
	VoxelMesh->ClearInstances();
	InstanceIndexByGridPosition.Reset();
	GridPositionByInstanceIndex.Reset();

	TArray<FIntVector> Locations;
	Locations.Reserve(SizeX * SizeY * NoiseAmplitude * 2);
	for (int X = 0; X < SizeX; ++X)
	{
		for (int Y = 0; Y < SizeY; ++Y)
		{
			const int Height = GetHeight(X, Y);
			for (int Z = 0; Z < Height; ++Z) {
				Locations.Emplace(X, Y, Z);
			}		
		}
	}
	AddCubes(Locations);
}

int AVoxelTerrain::GetHeight(const int X, const int Y) const
{
	const FVector2D NoisePosition(static_cast<double>(X) * NoiseFrequency,static_cast<double>(Y) * NoiseFrequency);
	const float NoiseValue = FMath::PerlinNoise2D(NoisePosition);

	return NoiseAmplitude + FMath::RoundToInt(NoiseValue * NoiseAmplitude);
}

bool AVoxelTerrain::IsVoxelMeshReady() const
{
	return VoxelMesh && VoxelMesh->GetStaticMesh();
}

FTransform AVoxelTerrain::MakeCubeTransform(const FIntVector& GridPosition) const
{
	const FVector Location(
		static_cast<double>(GridPosition.X) * VoxelSize,
		static_cast<double>(GridPosition.Y) * VoxelSize,
		static_cast<double>(GridPosition.Z) * VoxelSize);
	const FVector Scale = FVector::OneVector * (VoxelSize / 100.0f);

	return FTransform(FRotator::ZeroRotator, Location, Scale);
}

void AVoxelTerrain::AddCubes(const TArray<FIntVector>& GridPositions)
{
	if (!IsVoxelMeshReady())
	{
		return;
	}

	TArray<FTransform> Transforms;
	Transforms.Reserve(GridPositions.Num());
	TArray<FIntVector> NewGridPositions;
	NewGridPositions.Reserve(GridPositions.Num());
	TSet<FIntVector> PendingGridPositions;

	for (const FIntVector& GridPosition : GridPositions)
	{
		if (InstanceIndexByGridPosition.Contains(GridPosition) ||
			PendingGridPositions.Contains(GridPosition))
		{
			continue;
		}

		PendingGridPositions.Add(GridPosition);
		NewGridPositions.Add(GridPosition);
		Transforms.Emplace(MakeCubeTransform(GridPosition));
	}		

	const auto NewInstanceIndices = VoxelMesh->AddInstances(Transforms, true);
	for (int Index = 0; Index < NewInstanceIndices.Num(); ++Index)
	{
		const FIntVector& GridPosition = NewGridPositions[Index];
		const int InstanceIndex = NewInstanceIndices[Index];

		InstanceIndexByGridPosition.Add(GridPosition, InstanceIndex);
		GridPositionByInstanceIndex.Add(GridPosition);
	}
}

bool AVoxelTerrain::AddCube(const FIntVector& GridPosition)
{
	if (!IsVoxelMeshReady() || InstanceIndexByGridPosition.Contains(GridPosition))
	{
		return false;
	}

	const int InstanceIndex = VoxelMesh->AddInstance(MakeCubeTransform(GridPosition));
	if (InstanceIndex == INDEX_NONE)
	{
		return false;
	}

	InstanceIndexByGridPosition.Add(GridPosition, InstanceIndex);
	GridPositionByInstanceIndex.Add(GridPosition);
	return true;
}

bool AVoxelTerrain::AddCube(const FVector& GridPosition)
{
	return AddCube(FIntVector(
		FMath::RoundToInt(GridPosition.X),
		FMath::RoundToInt(GridPosition.Y),
		FMath::RoundToInt(GridPosition.Z)));
}

bool AVoxelTerrain::RemoveCube(const FIntVector& GridPosition)
{
	const int* InstanceIndexPtr = InstanceIndexByGridPosition.Find(GridPosition);
	if (!VoxelMesh || !InstanceIndexPtr)
	{
		return false;
	}

	const int InstanceIndex = *InstanceIndexPtr;
	const int LastInstanceIndex = GridPositionByInstanceIndex.Num() - 1;
	const FIntVector MovedGridPosition = GridPositionByInstanceIndex[LastInstanceIndex];

	if (!VoxelMesh->RemoveInstance(InstanceIndex))
	{
		return false;
	}

	InstanceIndexByGridPosition.Remove(GridPosition);
	GridPositionByInstanceIndex.RemoveAtSwap(InstanceIndex, 1, EAllowShrinking::No);

	if (InstanceIndex != LastInstanceIndex)
	{
		InstanceIndexByGridPosition[MovedGridPosition] = InstanceIndex;
	}

	return true;
}

// Called every frame
void AVoxelTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

