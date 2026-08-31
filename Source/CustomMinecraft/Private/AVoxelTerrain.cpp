// Fill out your copyright notice in the Description page of Project Settings.


#include "AVoxelTerrain.h"
#include "Components/SceneComponent.h"
#include "CubeDefinition.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "String/LexFromString.h"
#include "VoxelInstanceCollision.h"
#include "VoxelInstanceRenderer.h"

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

	VoxelCollision = CreateDefaultSubobject<UVoxelInstanceCollision>(TEXT("VoxelCollision"));
}

// Called when the game starts or when spawned
void AVoxelTerrain::BeginPlay()
{
	Super::BeginPlay();

	if (!ValidateCubeDefinitions()) 
	{
		return;
	}

	InitializeVoxelInstances();
	VoxelCollision->Initialize(VoxelSize);
	FillCubeTypesByLayer();
	WorldData.Initialize(FIntVector(ChunkSizeX, ChunkSizeY, SizeZ));
	GenerateTerrain();
	OnTerrainGenerated.Broadcast();
}

const FCubeDefinition* AVoxelTerrain::FindCubeDefinition(const FName Type) const
{
	if (CubeDefinitions) 
	{
		return CubeDefinitions->FindRow<FCubeDefinition>(Type, TEXT("AVoxelTerrain::FindCubeDefinition"));
	}
	return nullptr;
}

bool AVoxelTerrain::ValidateCubeDefinitions() const
{
	if (!CubeDefinitions)
	{
		UE_LOG(LogTemp, Warning, TEXT("Voxel terrain has no cube definitions DataTable selected"));
		return false;
	}

	if (CubeDefinitions->GetRowNames().IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cube definitions DataTable contains no rows"));
		return false;
	}

	return true;
}

void AVoxelTerrain::InitializeVoxelInstances()
{
	VoxelInstanceManagers.Reset();
	const TArray<FName> RowNames = CubeDefinitions->GetRowNames();

	for (const FName RowName : RowNames)
	{
		const FCubeDefinition* CubeDefinition = FindCubeDefinition(RowName);
		if (!CubeDefinition)
		{
			continue;
		}

		UVoxelInstanceRenderer* InstanceManager = NewObject<UVoxelInstanceRenderer>(this);
		if (!InstanceManager)
		{
			continue;
		}
		AddInstanceComponent(InstanceManager);
		InstanceManager->RegisterComponent();

		if (!CubeDefinition->Material)
		{
			UE_LOG(LogTemp, Warning,TEXT("Cube definition '%s' has no material"),*RowName.ToString());
		}

		InstanceManager->Initialize(*CubeDefinition, VoxelSize);
		VoxelInstanceManagers.Add(RowName, InstanceManager);
	}
}

void AVoxelTerrain::FillCubeTypesByLayer()
{
	CubeTypesByLayer.Reset();

	for (const FName RowName : CubeDefinitions->GetRowNames())
	{
		const FCubeDefinition* CubeDefinition = FindCubeDefinition(RowName);

		if (!CubeDefinition || !CubeDefinition->GenerationLayer.IsValid())
		{
			continue;
		}

		CubeTypesByLayer.FindOrAdd(CubeDefinition->GenerationLayer).Add(RowName);
	}
}

void AVoxelTerrain::GenerateTerrain()
{
	ApplySeed();
	RenderWorld();
	VoxelCollision->Refresh();
}

void AVoxelTerrain::GenerateArea(const FIntRect& Area)
{
	for (int WorldX = Area.Min.X; WorldX < Area.Max.X; ++WorldX)
	{
		for (int WorldY = Area.Min.Y; WorldY < Area.Max.Y; ++WorldY)
		{
			const int Height = FMath::Clamp(GetHeight(WorldX, WorldY), 0, SizeZ);

			for (int Z = 0; Z < Height; ++Z)
			{
				WorldData.SetVoxel(FIntVector(WorldX, WorldY, Z), GetCubeTypeByHeight(Z));
			}
		}
	}
}

void AVoxelTerrain::SetPlayerPosition(const FVector& Pos)
{
	const FVector LocalPosition = GetActorTransform().InverseTransformPosition(Pos) / VoxelSize;
	const FIntPoint NewGridPosition(FMath::FloorToInt(LocalPosition.X + 0.5), FMath::FloorToInt(LocalPosition.Y + 0.5));
	if (!WorldData.IsInSameRegion(PlayerGridPosition, NewGridPosition))
	{
		PlayerGridPosition = NewGridPosition;
		RenderWorld();
	}
	VoxelCollision->SetPlayerPosition(Pos);
}

void AVoxelTerrain::ApplySeed()
{
	const double Range = 100000.0;

	if (Seed == 0)
	{
		FRandomStream SeedGenerator;
		SeedGenerator.GenerateNewSeed();
		Seed = SeedGenerator.GetInitialSeed();
	}

	FRandomStream RandomStream(Seed);

	NoiseOffset = FVector2D(RandomStream.FRandRange(-Range, Range), RandomStream.FRandRange(-Range, Range));
	UE_LOG(LogTemp, Log, TEXT("Applied terrain seed: %d"), Seed);
}

int AVoxelTerrain::GetHeight(const int X, const int Y) const
{
	const FVector2D NoisePosition(static_cast<double>(X) * NoiseFrequency + NoiseOffset.X, static_cast<double>(Y) * NoiseFrequency + NoiseOffset.Y);
	const float NoiseValue = FMath::PerlinNoise2D(NoisePosition);

	return BaseTerrainHeight + NoiseAmplitude + FMath::RoundToInt(NoiseValue * NoiseAmplitude);
}

FVector AVoxelTerrain::GetLocationAboveSurface(const int32 GridX, const int32 GridY) const
{
	if (WorldData.IsEmpty())
	{
		return FVector::ZeroVector;
	}

	const TOptional<int32> SurfaceZ = WorldData.FindSurfaceZ(FIntPoint(GridX, GridY));
	if (!SurfaceZ)
	{
		return FVector::ZeroVector;
	}

	const FVector LocalLocation(
		static_cast<double>(GridX) * VoxelSize,
		static_cast<double>(GridY) * VoxelSize,
		static_cast<double>(*SurfaceZ + 1) * VoxelSize);
	return GetActorTransform().TransformPosition(LocalLocation);
}

FName AVoxelTerrain::GetCubeTypeByHeight(int32 height)
{
	for (const auto& [tag, range] : LayerRanges) 
	{
		if (range.InRange(height))
		{
			const TArray<FName>* CubeTypes = CubeTypesByLayer.Find(tag);
			if (!CubeTypes || CubeTypes->IsEmpty())
			{
				return NAME_None;
			}

			return CubeTypes->Last();
		}
	}

	return NAME_None;
}

void AVoxelTerrain::RenderWorld()
{
	WorldData.EnsureAreaAround(	PlayerGridPosition,	RenderDistanceInChunks, 
								[this](const FIntRect& Area) { GenerateArea(Area); });

	for (const auto& [Name, ManagerPtr] : VoxelInstanceManagers)
	{
		if (ManagerPtr)
		{
			ManagerPtr->SetCubes(WorldData.GetVoxelPositionsAround(Name, PlayerGridPosition, RenderDistanceInChunks));
		}
	}
}

bool AVoxelTerrain::AddCube(const FIntVector& GridPosition)
{
	if (WorldData.IsEmpty() || VoxelInstanceManagers.IsEmpty() || !CubeDefinitions)
	{
		return false;
	}

	const TArray<FName> RowNames = CubeDefinitions->GetRowNames();
	if (RowNames.IsEmpty() || !WorldData.SetVoxel(GridPosition, GetCubeTypeByHeight(GridPosition.Z)))
	{
		return false;
	}

	RenderWorld();
	VoxelCollision->Refresh();
	return true;
}

bool AVoxelTerrain::RemoveCube(const FIntVector& GridPosition)
{
	if (WorldData.IsEmpty() || VoxelInstanceManagers.IsEmpty() || !CanRemoveCube(GridPosition))
	{
		return false;
	}
	if (!WorldData.RemoveVoxel(GridPosition))
	{
		return false;
	}

	RenderWorld();
	VoxelCollision->Refresh();
	return true;
}

bool AVoxelTerrain::HasVoxel(const FIntVector& GridPosition) const
{
	return WorldData.HasVoxel(GridPosition);
}

bool AVoxelTerrain::CanRemoveCube(const FIntVector& GridPosition) const
{
	return GridPosition.Z > 0;
}

bool AVoxelTerrain::DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, const float SphereRadius) const
{
	return VoxelCollision && VoxelCollision->DoesCubeOverlapSphere(GridPosition, SphereCenter, SphereRadius);
}

float AVoxelTerrain::GetMiningDuration(const FName Type)
{
	if (const FCubeDefinition* Definition = FindCubeDefinition(Type))
	{
		return Definition->MiningDuration;
	}

	checkNoEntry();
	return 0.f;
}

TOptional<FVoxelHit> AVoxelTerrain::TraceVoxel(const FVector& Start, const FVector& End) const
{
	if (WorldData.IsEmpty() || VoxelSize <= 0.0f)
	{
		return {};
	}

	const FTransform TerrainTransform = GetActorTransform();
	const FVector GridStart = TerrainTransform.InverseTransformPosition(Start) / VoxelSize;
	const FVector GridEnd = TerrainTransform.InverseTransformPosition(End) / VoxelSize;
	return TraceVoxelGridDDA(GridStart, GridEnd);
}

FIntVector AVoxelTerrain::CalculateVoxelHitNormal(const FIntVector& PreviousGridPosition, const FIntVector& GridPosition)
{
	return PreviousGridPosition - GridPosition;
}

TOptional<FVoxelHit> AVoxelTerrain::TraceVoxelGridDDA(const FVector& GridStart, const FVector& GridEnd) const
{
	// Delta represents the entire ray segment in grid space. The ray parameter ranges from 0 to 1.
	const FVector Delta = GridEnd - GridStart;

	// Voxel centers are at integer coordinates, so their boundaries are offset by 0.5.
	FIntVector GridPosition(FMath::FloorToInt(GridStart.X + 0.5), FMath::FloorToInt(GridStart.Y + 0.5), FMath::FloorToInt(GridStart.Z + 0.5));

	// The ray may start inside an occupied voxel.
	if (const TOptional<FName> Type = WorldData.GetVoxelType(GridPosition))
	{
		return FVoxelHit{GridPosition, FIntVector::ZeroValue, *Type};
	}

	// Step stores the traversal direction for each axis: -1, 0, or 1.
	FIntVector Step = FIntVector::ZeroValue;

	// ParameterDelta is the ray-parameter distance between adjacent boundaries on an axis.
	// NextBoundaryParameter is the parameter at which the ray reaches the next boundary on an axis.
	const double MaxParameter = TNumericLimits<double>::Max();
	FVector ParameterDelta(MaxParameter, MaxParameter, MaxParameter);
	FVector NextBoundaryParameter = ParameterDelta;

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		// A stationary axis never crosses another boundary.
		if (Delta[Axis] == 0.0)
		{
			continue;
		}

		Step[Axis] = Delta[Axis] > 0.0 ? 1 : -1;
		ParameterDelta[Axis] = 1.0 / FMath::Abs(Delta[Axis]);

		const double NextBoundary = GridPosition[Axis] + 0.5 * Step[Axis];
		NextBoundaryParameter[Axis] = (NextBoundary - GridStart[Axis]) / Delta[Axis];
	}

	// Traverse voxels while the nearest boundary is still within the ray segment.
	while (FMath::Min3(NextBoundaryParameter.X, NextBoundaryParameter.Y, NextBoundaryParameter.Z) <= 1.0)
	{
		// Select the axis whose boundary the ray reaches first.
		int32 NextAxis = 0;
		if (NextBoundaryParameter.Y < NextBoundaryParameter[NextAxis])
		{
			NextAxis = 1;
		}
		if (NextBoundaryParameter.Z < NextBoundaryParameter[NextAxis])
		{
			NextAxis = 2;
		}

		// Enter the adjacent voxel and advance the selected axis boundary by one cell.
		const FIntVector PreviousGridPosition = GridPosition;
		GridPosition[NextAxis] += Step[NextAxis];
		NextBoundaryParameter[NextAxis] += ParameterDelta[NextAxis];

		if (const TOptional<FName> Type = WorldData.GetVoxelType(GridPosition))
		{
			return FVoxelHit{GridPosition, CalculateVoxelHitNormal(PreviousGridPosition, GridPosition), *Type};
		}
	}

	return {};
}
