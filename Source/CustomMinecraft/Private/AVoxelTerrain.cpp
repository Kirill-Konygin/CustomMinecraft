// Fill out your copyright notice in the Description page of Project Settings.


#include "AVoxelTerrain.h"
#include "Components/SceneComponent.h"
#include "CubeDefinition.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "String/LexFromString.h"
#include "VoxelInstanceManager.h"

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

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);
	VoxelInstanceManager = CreateDefaultSubobject<UVoxelInstanceManager>(TEXT("VoxelInstanceManager"));
}

// Called when the game starts or when spawned
void AVoxelTerrain::BeginPlay()
{
	Super::BeginPlay();

	InitializeVoxelInstances();
	GenerateTerrain();
}

void AVoxelTerrain::InitializeVoxelInstances()
{
	if (!CubeDefinitions)
	{
		UE_LOG(LogTemp, Warning,TEXT("Voxel terrain has no cube definitions DataTable selected"));
		return;
	}

	const TArray<FName> RowNames = CubeDefinitions->GetRowNames();
	if (RowNames.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,TEXT("Cube definitions DataTable contains no rows"));
		return;
	}

	const FName FirstRowName = RowNames[0];
	const FCubeDefinition* CubeDefinition = CubeDefinitions->FindRow<FCubeDefinition>(FirstRowName,TEXT("AVoxelTerrain::InitializeVoxelInstances"));
	if (!CubeDefinition || !VoxelInstanceManager)
	{
		return;
	}

	if (!CubeDefinition->Material)
	{
		UE_LOG(LogTemp, Warning,TEXT("Cube definition '%s' has no material"),*FirstRowName.ToString());
	}

	VoxelInstanceManager->Initialize(*CubeDefinition, VoxelSize);
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

void AVoxelTerrain::RenderChunk()
{
	if (!VoxelInstanceManager || !Chunk)
	{
		return;
	}

	VoxelInstanceManager->SetCubes(Chunk->GetVoxelLocalPositions());
}

bool AVoxelTerrain::AddCube(const FIntVector& GridPosition)
{
	if (!Chunk || !VoxelInstanceManager || !Chunk->SetVoxel(GridPosition, true))
	{
		return false;
	}

	VoxelInstanceManager->SetCubes(Chunk->GetVoxelLocalPositions());
	return true;
}

bool AVoxelTerrain::RemoveCube(const FIntVector& GridPosition)
{
	if (!Chunk || !VoxelInstanceManager || !Chunk->SetVoxel(GridPosition, false))
	{
		return false;
	}

	VoxelInstanceManager->SetCubes(Chunk->GetVoxelLocalPositions());
	return true;
}

bool AVoxelTerrain::DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, const float SphereRadius) const
{
	return VoxelInstanceManager && VoxelInstanceManager->DoesCubeOverlapSphere(GridPosition, SphereCenter, SphereRadius);
}

TOptional<FVoxelHit> AVoxelTerrain::TraceVoxel(const FVector& Start, const FVector& End) const
{
	if (!VoxelInstanceManager)
	{
		return {};
	}

	const TOptional<FVoxelInstanceHit> Hit = VoxelInstanceManager->TraceVoxel(Start, End);
	if (!Hit)
	{
		return {};
	}

	return FVoxelHit{Hit->Position, Hit->Normal};
}

// Called every frame
void AVoxelTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
