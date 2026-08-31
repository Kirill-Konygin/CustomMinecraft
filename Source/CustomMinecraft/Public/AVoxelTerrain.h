// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Chunk.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelInstanceManager.h"
#include "GameplayTagContainer.h"
#include "AVoxelTerrain.generated.h"

class UDataTable;
class USceneComponent;
struct FCubeDefinition;

struct FVoxelHit
{
	FIntVector Position = FIntVector::ZeroValue;
	FIntVector Normal = FIntVector::ZeroValue;
	FName Type;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTerrainGenerated);

USTRUCT(BlueprintType)
struct CUSTOMMINECRAFT_API FCubeHeightRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
	int32 MinHeight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
	int32 MaxHeight = 0;

	bool InRange(int32 val) const
	{
		return val <= MaxHeight && val >= MinHeight;
	}
};

UCLASS()
class CUSTOMMINECRAFT_API AVoxelTerrain : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVoxelTerrain();

	UPROPERTY(BlueprintAssignable, Category = "Voxel Terrain|Generation")
	FOnTerrainGenerated OnTerrainGenerated;

	UFUNCTION(BlueprintPure, Category = "Voxel Terrain|Generation")
	FVector GetLocationAboveSurface(int32 GridX = 0, int32 GridY = 0) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void GenerateTerrain();
	void GenerateChunk(const FIntPoint& ChunkPosition);
	FIntPoint GetChunkPosition(const FIntPoint& GridPosition) const;
	FIntPoint GetChunkPosition(const FIntVector& GridPosition) const;
	FIntVector GetChunkLocalPosition(const FIntVector& GridPosition) const;
	FChunk* FindChunk(const FIntPoint& ChunkPosition);
	const FChunk* FindChunk(const FIntPoint& ChunkPosition) const;
	FChunk* FindChunk(const FIntVector& ChunkPosition);
	const FChunk* FindChunk(const FIntVector& ChunkPosition) const;
	void ApplySeed();
	void RenderChunk();
	void InitializeVoxelInstances();
	void FillCubeTypesByLayer();
	bool ValidateCubeDefinitions() const;
	bool CanRemoveCube(const FIntVector& GridPosition) const;
	const FCubeDefinition* FindCubeDefinition(FName Type) const;
	int GetHeight(int X, int Y) const;
	FName GetCubeTypeByHeight(int32 height);
	static FIntVector CalculateVoxelHitNormal(const FIntVector& PreviousGridPosition, const FIntVector& GridPosition);
	TOptional<FVoxelHit> TraceVoxelGridDDA(const FVector& GridStart, const FVector& GridEnd) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Terrain")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TMap<FName, TObjectPtr<UVoxelInstanceManager>> VoxelInstanceManagers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel Terrain|Cube Definitions")
	TObjectPtr<UDataTable> CubeDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Layers", meta = (Categories = "World.Layer"))
	TMap<FGameplayTag, FCubeHeightRange> LayerRanges;

	TMap<FGameplayTag, TArray<FName>> CubeTypesByLayer;

	TMap<FIntPoint, TUniquePtr<FChunk>> Chunks;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool AddCube(const FIntVector& GridPosition);
	bool RemoveCube(const FIntVector& GridPosition);
	float GetMiningDuration(FName Type);
	bool DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, float SphereRadius) const;
	TOptional<FVoxelHit> TraceVoxel(const FVector& Start, const FVector& End) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Seed")
	int Seed = 0;

	FVector2D NoiseOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "1"))
	int ChunksX = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "1"))
	int ChunksY = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Chunk", meta = (ClampMin = "1"))
	int ChunkSizeX = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Chunk", meta = (ClampMin = "1"))
	int ChunkSizeY = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "1"))
	int SizeZ = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "1.0"))
	float VoxelSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "0"))
	int BaseTerrainHeight = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Noise", meta = (ClampMin = "0.0001"))
	float NoiseFrequency = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Noise", meta = (ClampMin = "0"))
	int NoiseAmplitude = 10;

};
