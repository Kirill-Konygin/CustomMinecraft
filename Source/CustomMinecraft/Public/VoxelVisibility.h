#pragma once

#include "CoreMinimal.h"

class FVoxelWorldData;

struct FVoxelVisibilityChanges
{
	TArray<FIntVector> Added;
	TArray<FIntVector> Removed;
};

class CUSTOMMINECRAFT_API FVoxelVisibility final
{
public:
	TOptional<int32> FindSurfaceZ(const FIntPoint& GridPosition) const;

	void UpdateArea(const FVoxelWorldData& WorldData, const FIntRect& Area, int32 Height);
	void UpdateVoxelAround(const FVoxelWorldData& WorldData, const FIntVector& GridPosition);
	// Changes belong to the latest update and are replaced by the next one.
	const FVoxelVisibilityChanges* GetChanges(FName CubeId) const;
	TArray<FIntVector> GetVoxelPositions(FName CubeId) const;
	TArray<FIntVector> GetVoxelPositions(FName CubeId, const FIntRect& Area) const;

private:
	void UpdateVoxel(const FVoxelWorldData& WorldData, const FIntVector& GridPosition, TMap<FName, FVoxelVisibilityChanges>* Changes = nullptr);
	bool HasAnyEmptyNeighbor(const FVoxelWorldData& WorldData, const FIntVector& GridPosition) const;

	FIntRect VisibleArea = FIntRect(FIntPoint::ZeroValue, FIntPoint::ZeroValue);
	TMap<FIntPoint, TMap<int32, FName>> VisibleVoxelsByColumn;
	TMap<FName, FVoxelVisibilityChanges> ChangesByType;
};
