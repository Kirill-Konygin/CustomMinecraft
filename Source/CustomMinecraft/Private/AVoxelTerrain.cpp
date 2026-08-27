// Fill out your copyright notice in the Description page of Project Settings.


#include "AVoxelTerrain.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "String/LexFromString.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	AVoxelTerrain* FindVoxelTerrain(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AVoxelTerrain> TerrainIt(World); TerrainIt; ++TerrainIt)
		{
			return *TerrainIt;
		}

		return nullptr;
	}

	bool TryParseGridPosition(const TArray<FString>& Args, FIntVector& OutGridPosition)
	{
		if (Args.Num() != 3)
		{
			return false;
		}

		int X;
		int Y;
		int Z;
		if (!LexTryParseString(X, *Args[0]) ||
			!LexTryParseString(Y, *Args[1]) ||
			!LexTryParseString(Z, *Args[2]))
		{
			return false;
		}

		OutGridPosition = FIntVector(X, Y, Z);
		return true;
	}

	void AddCubeIntFromConsole(const TArray<FString>& Args, UWorld* World)
	{
		FIntVector GridPosition;
		if (!TryParseGridPosition(Args, GridPosition))
		{
			UE_LOG(LogTemp, Warning, TEXT("Usage: Voxel.AddCube X Y Z"));
			return;
		}

		AVoxelTerrain* Terrain = FindVoxelTerrain(World);
		if (!Terrain)
		{
			UE_LOG(LogTemp, Warning, TEXT("Voxel.AddCube: terrain not found"));
			return;
		}

		if (!Terrain->AddCube(GridPosition))
		{
			UE_LOG(LogTemp, Warning, TEXT("Voxel.AddCube: cube already exists or is outside terrain"));
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("Added cube at (%d, %d, %d)"),
			GridPosition.X, GridPosition.Y, GridPosition.Z);
	}

	void RemoveCubeFromConsole(const TArray<FString>& Args, UWorld* World)
	{
		FIntVector GridPosition;
		if (!TryParseGridPosition(Args, GridPosition))
		{
			UE_LOG(LogTemp, Warning, TEXT("Usage: Voxel.RemoveCube X Y Z"));
			return;
		}

		AVoxelTerrain* Terrain = FindVoxelTerrain(World);
		if (!Terrain)
		{
			UE_LOG(LogTemp, Warning, TEXT("Voxel.RemoveCube: terrain not found"));
			return;
		}

		if (!Terrain->RemoveCube(GridPosition))
		{
			UE_LOG(LogTemp, Warning, TEXT("Voxel.RemoveCube: cube not found"));
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("Removed cube at (%d, %d, %d)"),
			GridPosition.X, GridPosition.Y, GridPosition.Z);
	}

	FAutoConsoleCommandWithWorldAndArgs AddCubeIntConsoleCommand(
		TEXT("Voxel.AddCube"),
		TEXT("Adds a cube at grid coordinates: Voxel.AddCube X Y Z"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AddCubeIntFromConsole));

	FAutoConsoleCommandWithWorldAndArgs RemoveCubeConsoleCommand(
		TEXT("Voxel.RemoveCube"),
		TEXT("Removes a cube at grid coordinates: Voxel.RemoveCube X Y Z"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RemoveCubeFromConsole));
}

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
	Chunk = MakeUnique<FChunk>(FIntVector(SizeX, SizeY, SizeZ));

	for (int X = 0; X < SizeX; ++X)
	{
		for (int Y = 0; Y < SizeY; ++Y)
		{
			const int Height = FMath::Clamp(GetHeight(X, Y), 0, SizeZ);
			for (int Z = 0; Z < Height; ++Z)
			{
				Chunk->SetVoxel(FIntVector(X, Y, Z), true);
			}
		}
	}

	RenderChunk();
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

void AVoxelTerrain::RenderChunk()
{
	if (!IsVoxelMeshReady() || !Chunk)
	{
		return;
	}

	VoxelMesh->ClearInstances();
	InstanceIndexByGridPosition.Reset();
	GridPositionByInstanceIndex.Reset();

	const TArray<FIntVector> GridPositions = Chunk->GetVoxelLocalPositions();
	AddVoxelInstances(GridPositions);
}

void AVoxelTerrain::AddVoxelInstances(const TArray<FIntVector>& GridPositions)
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
	GridPositionByInstanceIndex.SetNum(VoxelMesh->GetInstanceCount());
	for (int Index = 0; Index < NewInstanceIndices.Num(); ++Index)
	{
		const FIntVector& GridPosition = NewGridPositions[Index];
		const int InstanceIndex = NewInstanceIndices[Index];
		if (!GridPositionByInstanceIndex.IsValidIndex(InstanceIndex))
		{
			continue;
		}

		InstanceIndexByGridPosition.Add(GridPosition, InstanceIndex);
		GridPositionByInstanceIndex[InstanceIndex] = GridPosition;
	}
}

bool AVoxelTerrain::AddCube(const FIntVector& GridPosition)
{
	if (!Chunk || !Chunk->SetVoxel(GridPosition, true))
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

bool AVoxelTerrain::RemoveCube(const FIntVector& GridPosition)
{
	if (!Chunk || !Chunk->SetVoxel(GridPosition, false))
	{
		return false;
	}

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

bool AVoxelTerrain::DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, const float SphereRadius) const
{
	if (!IsVoxelMeshReady())
	{
		return false;
	}

	const FTransform CubeWorldTransform = MakeCubeTransform(GridPosition) * GetActorTransform();
	const FBox CubeBounds = VoxelMesh->GetStaticMesh()->GetBoundingBox().TransformBy(CubeWorldTransform);

	return FMath::SphereAABBIntersection( SphereCenter, FMath::Square(static_cast<double>(SphereRadius)), CubeBounds);
}

TOptional<FVoxelHit> AVoxelTerrain::TraceVoxel(const FVector& Start, const FVector& End) const
{
	if (!VoxelMesh)
	{
		return {};
	}

	FHitResult HitResult;
	if (	!VoxelMesh->LineTraceComponent( HitResult, Start, End, FCollisionQueryParams::DefaultQueryParam) 
		||	!GridPositionByInstanceIndex.IsValidIndex(HitResult.Item))
	{
		return {};
	}

	const FVector LocalNormal = GetActorTransform().InverseTransformVectorNoScale(HitResult.ImpactNormal);

	return FVoxelHit{
		GridPositionByInstanceIndex[HitResult.Item],
		FIntVector(FMath::RoundToInt(LocalNormal.X), FMath::RoundToInt(LocalNormal.Y), FMath::RoundToInt(LocalNormal.Z))
	};
}

// Called every frame
void AVoxelTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
