// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AVoxelTerrain.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class CUSTOMMINECRAFT_API AVoxelTerrain : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVoxelTerrain();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void GenerateTerrain();
	void AddCubes(const TArray<FIntVector>& GridPositions);
	int GetHeight(int X, int Y) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Terrain")
	TObjectPtr<UInstancedStaticMeshComponent> VoxelMesh;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "1"))
	int SizeX = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "1"))
	int SizeY = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain", meta = (ClampMin = "1.0"))
	float VoxelSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Noise", meta = (ClampMin = "0.0001"))
	float NoiseFrequency = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Terrain|Noise", meta = (ClampMin = "0"))
	int NoiseAmplitude = 10;

};
