#pragma once

#include "Chunk.h"
#include "CoreMinimal.h"

class CUSTOMMINECRAFT_API FVoxelWorldData final
{
public:
	FVoxelWorldData() = default;
	FVoxelWorldData(const FVoxelWorldData&) = delete;
	FVoxelWorldData& operator=(const FVoxelWorldData&) = delete;
	FVoxelWorldData(FVoxelWorldData&&) = default;
	FVoxelWorldData& operator=(FVoxelWorldData&&) = default;

	void Initialize(const FIntVector& InRegionSize);

	bool IsEmpty() const;

	bool HasData(const FIntVector& GridPosition) const;
	bool HasVoxel(const FIntVector& GridPosition) const;
	TOptional<FName> GetVoxelType(const FIntVector& GridPosition) const;
	bool SetVoxel(const FIntVector& GridPosition, FName CubeId);
	bool RemoveVoxel(const FIntVector& GridPosition);

private:
	FIntPoint GetRegionPosition(const FIntVector& GridPosition) const;
	const FChunk* FindRegion(const FIntPoint& RegionPosition) const;
	FIntVector GetRegionLocalPosition(const FIntVector& GridPosition) const;

	FIntVector RegionSize = FIntVector(1, 1, 1);
	TMap<FIntPoint, TUniquePtr<FChunk>> Regions;
};
