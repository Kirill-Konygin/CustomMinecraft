#include "VoxelVisibility.h"

#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "VoxelWorldData.h"

namespace
{
	const FIntVector NeighborOffsets[] =
	{
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0),
		FIntVector(0, 0, 1),
		FIntVector(0, 0, -1)
	};
}

TOptional<int32> FVoxelVisibility::FindSurfaceZ(const FIntPoint& GridPosition) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FVoxelVisibility_FindSurfaceZ);

	const TMap<int32, FName>* VisibleVoxels = VisibleVoxelsByColumn.Find(GridPosition);
	if (!VisibleVoxels)
	{
		return {};
	}

	TOptional<int32> SurfaceZ;
	for (const auto& VisibleVoxel : *VisibleVoxels)
	{
		if (!SurfaceZ || VisibleVoxel.Key > *SurfaceZ)
		{
			SurfaceZ = VisibleVoxel.Key;
		}
	}

	return SurfaceZ;
}

void FVoxelVisibility::UpdateArea(const FVoxelWorldData& WorldData, const FIntRect& Area, const int32 Height)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FVoxelVisibility_UpdateArea);

	check(Height >= 0);

	VisibleArea = Area;

	for (int32 X = Area.Min.X; X < Area.Max.X; ++X)
	{
		for (int32 Y = Area.Min.Y; Y < Area.Max.Y; ++Y)
		{
			for (int32 Z = 0; Z < Height; ++Z)
			{
				const FIntVector GridPosition(X, Y, Z);
				UpdateVoxel(WorldData, GridPosition);

				// Changed area boundaries can hide or expose adjacent voxels.
				for (const FIntVector& NeighborOffset : NeighborOffsets)
				{
					const FIntVector NeighborPosition = GridPosition + NeighborOffset;
					if (!Area.Contains(FIntPoint(NeighborPosition.X, NeighborPosition.Y)))
					{
						UpdateVoxel(WorldData, NeighborPosition);
					}
				}
			}
		}
	}
}

void FVoxelVisibility::UpdateVoxelAround(const FVoxelWorldData& WorldData, const FIntVector& GridPosition)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FVoxelVisibility_UpdateVoxelAround);

	UpdateVoxel(WorldData, GridPosition);
	for (const FIntVector& NeighborOffset : NeighborOffsets)
	{
		UpdateVoxel(WorldData, GridPosition + NeighborOffset);
	}
}

void FVoxelVisibility::UpdateVoxel(const FVoxelWorldData& WorldData, const FIntVector& GridPosition)
{
	const FIntPoint ColumnPosition(GridPosition.X, GridPosition.Y);
	const TOptional<FName> CubeId = WorldData.GetVoxelType(GridPosition);
	if (CubeId && HasAnyEmptyNeighbor(WorldData, GridPosition))
	{
		VisibleVoxelsByColumn.FindOrAdd(ColumnPosition).Add(GridPosition.Z, *CubeId);
	}
	else if (TMap<int32, FName>* VisibleVoxels = VisibleVoxelsByColumn.Find(ColumnPosition))
	{
		VisibleVoxels->Remove(GridPosition.Z);
		if (VisibleVoxels->IsEmpty())
		{
			VisibleVoxelsByColumn.Remove(ColumnPosition);
		}
	}
}

bool FVoxelVisibility::HasAnyEmptyNeighbor(const FVoxelWorldData& WorldData, const FIntVector& GridPosition) const
{
	for (const FIntVector& NeighborOffset : NeighborOffsets)
	{
		if (!WorldData.HasVoxel(GridPosition + NeighborOffset))
		{
			return true;
		}
	}

	return false;
}

TArray<FIntVector> FVoxelVisibility::GetVoxelPositions(const FName CubeId) const
{
	return GetVoxelPositions(CubeId, VisibleArea);
}

TArray<FIntVector> FVoxelVisibility::GetVoxelPositions(const FName CubeId, const FIntRect& Area) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FVoxelVisibility_GetVoxelPositions);

	TArray<FIntVector> Positions;
	for (int32 X = Area.Min.X; X < Area.Max.X; ++X)
	{
		for (int32 Y = Area.Min.Y; Y < Area.Max.Y; ++Y)
		{
			const TMap<int32, FName>* VisibleVoxels = VisibleVoxelsByColumn.Find(FIntPoint(X, Y));
			if (!VisibleVoxels)
			{
				continue;
			}

			for (const auto& [Z, Type] : *VisibleVoxels)
			{
				if (Type == CubeId)
				{
					Positions.Add(FIntVector(X, Y, Z));
				}
			}
		}
	}

	return Positions;
}
