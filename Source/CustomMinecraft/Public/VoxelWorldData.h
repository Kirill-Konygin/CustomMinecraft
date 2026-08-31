#pragma once

#include "Chunk.h"
#include "CoreMinimal.h"

class CUSTOMMINECRAFT_API FVoxelWorldData final
{
public:
	using FAreaGenerator = TFunctionRef<void(const FIntRect&)>;

	FVoxelWorldData() = default;
	FVoxelWorldData(const FVoxelWorldData&) = delete;
	FVoxelWorldData& operator=(const FVoxelWorldData&) = delete;
	FVoxelWorldData(FVoxelWorldData&&) = default;
	FVoxelWorldData& operator=(FVoxelWorldData&&) = default;

	void Initialize(const FIntVector& InRegionSize);

	bool IsEmpty() const;
	bool IsInSameRegion(const FIntPoint& FirstGridPosition, const FIntPoint& SecondGridPosition) const;

	void EnsureAreaAround(const FIntPoint& CenterGridPosition, int32 Radius, FAreaGenerator GenerateArea);
	TArray<FIntVector> GetVoxelPositionsAround(FName CubeId, const FIntPoint& CenterGridPosition, int32 Radius) const;

	bool HasVoxel(const FIntVector& GridPosition) const;
	TOptional<FName> GetVoxelType(const FIntVector& GridPosition) const;
	bool SetVoxel(const FIntVector& GridPosition, FName CubeId);
	bool RemoveVoxel(const FIntVector& GridPosition);
	TOptional<int32> FindSurfaceZ(const FIntPoint& GridPosition) const;

private:
	FIntPoint GetRegionPosition(const FIntPoint& GridPosition) const;
	FIntPoint GetRegionPosition(const FIntVector& GridPosition) const;
	FIntVector GetRegionLocalPosition(const FIntVector& GridPosition) const;
	FChunk* FindRegion(const FIntPoint& RegionPosition);
	const FChunk* FindRegion(const FIntPoint& RegionPosition) const;

	FIntVector RegionSize = FIntVector(1, 1, 1);
	TMap<FIntPoint, TUniquePtr<FChunk>> Regions;
};
