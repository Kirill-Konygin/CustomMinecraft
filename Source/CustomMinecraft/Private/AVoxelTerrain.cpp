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

void AVoxelTerrain::AddCubes(const TArray<FIntVector>& GridPositions)
{
	if (!VoxelMesh || !VoxelMesh->GetStaticMesh())
	{
		return;
	}
	TArray<FTransform> Transforms;
	Transforms.Reserve(GridPositions.Num());

	const FVector Scale = FVector::OneVector * (VoxelSize / 100.0f);
	for (const auto& IntLocation : GridPositions) {
		Transforms.Emplace(
			FRotator::ZeroRotator,
			FVector(
			static_cast<double>(IntLocation.X) * VoxelSize,
			static_cast<double>(IntLocation.Y) * VoxelSize,
			static_cast<double>(IntLocation.Z) * VoxelSize),
			Scale
		);
	}		

	VoxelMesh->AddInstances(Transforms,false);
}

// Called every frame
void AVoxelTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

