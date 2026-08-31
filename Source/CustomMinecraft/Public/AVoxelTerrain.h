// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "VoxelInstanceRenderer.h"
#include "VoxelWorldData.h"
#include "AVoxelTerrain.generated.h"

class UDataTable;
class USceneComponent;
class UVoxelInstanceCollision;
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
	void GenerateArea(const FIntRect& Area);

	void ApplySeed();
	void RenderWorld();
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
	TMap<FName, TObjectPtr<UVoxelInstanceRenderer>> VoxelInstanceManagers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Terrain|Collision")
	TObjectPtr<UVoxelInstanceCollision> VoxelCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel Terrain|Cube Definitions")
	TObjectPtr<UDataTable> CubeDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Layers", meta = (Categories = "World.Layer"))
	TMap<FGameplayTag, FCubeHeightRange> LayerRanges;

	TMap<FGameplayTag, TArray<FName>> CubeTypesByLayer;

	FVoxelWorldData WorldData;

	FIntPoint PlayerGridPosition = FIntPoint::ZeroValue;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool AddCube(const FIntVector& GridPosition);
	bool RemoveCube(const FIntVector& GridPosition);
	bool HasVoxel(const FIntVector& GridPosition) const;
	void SetPlayerPosition(const FVector& Pos);
	float GetMiningDuration(FName Type);
	bool DoesCubeOverlapSphere(const FIntVector& GridPosition, const FVector& SphereCenter, float SphereRadius) const;
	TOptional<FVoxelHit> TraceVoxel(const FVector& Start, const FVector& End) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Seed")
	int Seed = 0;

	FVector2D NoiseOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Chunk", meta = (ClampMin = "0"))
	int RenderDistanceInChunks = 1;

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
