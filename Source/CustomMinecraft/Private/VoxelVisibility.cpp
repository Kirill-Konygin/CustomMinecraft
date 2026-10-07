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

	ChangesByType.Reset();
	const FIntRect PreviousArea = VisibleArea;
	if (Area == PreviousArea)
	{
		return;
	}

	for (int32 X = PreviousArea.Min.X; X < PreviousArea.Max.X; ++X)
	{
		for (int32 Y = PreviousArea.Min.Y; Y < PreviousArea.Max.Y; ++Y)
		{
			const FIntPoint ColumnPosition(X, Y);
			if (Area.Contains(ColumnPosition))
			{
				continue;
			}

			if (const TMap<int32, FName>* VisibleVoxels = VisibleVoxelsByColumn.Find(ColumnPosition))
			{
				for (const auto& [Z, Type] : *VisibleVoxels)
				{
					ChangesByType.FindOrAdd(Type).Removed.Add(FIntVector(X, Y, Z));
				}
			}
		}
	}

	VisibleArea = Area;

	for (int32 X = Area.Min.X; X < Area.Max.X; ++X)
	{
		for (int32 Y = Area.Min.Y; Y < Area.Max.Y; ++Y)
		{
			const FIntPoint ColumnPosition(X, Y);
			const bool WasVisible = PreviousArea.Contains(ColumnPosition);
			// Entering columns can hide voxels along the previous area's boundary.
			const bool HasEnteringNeighbor =
				(X == PreviousArea.Min.X && Area.Min.X < PreviousArea.Min.X) ||
				(X == PreviousArea.Max.X - 1 && Area.Max.X > PreviousArea.Max.X) ||
				(Y == PreviousArea.Min.Y && Area.Min.Y < PreviousArea.Min.Y) ||
				(Y == PreviousArea.Max.Y - 1 && Area.Max.Y > PreviousArea.Max.Y);
			if (WasVisible && !HasEnteringNeighbor)
			{
				continue;
			}

			for (int32 Z = 0; Z < Height; ++Z)
			{
				const FIntVector GridPosition(X, Y, Z);
				UpdateVoxel(WorldData, GridPosition, WasVisible ? &ChangesByType : nullptr);
			}

			// Refresh cached neighbors outside the view.
			for (const FIntVector& NeighborOffset : NeighborOffsets)
			{
				const FIntPoint NeighborColumnPosition(X + NeighborOffset.X, Y + NeighborOffset.Y);
				if (!Area.Contains(NeighborColumnPosition))
				{
					for (int32 Z = 0; Z < Height; ++Z)
					{
						UpdateVoxel(WorldData, FIntVector(NeighborColumnPosition.X, NeighborColumnPosition.Y, Z));
					}
				}
			}

			if (!WasVisible)
			{
				if (const TMap<int32, FName>* VisibleVoxels = VisibleVoxelsByColumn.Find(ColumnPosition))
				{
					for (const auto& [Z, Type] : *VisibleVoxels)
					{
						ChangesByType.FindOrAdd(Type).Added.Add(FIntVector(X, Y, Z));
					}
				}
			}
		}
	}
}

void FVoxelVisibility::UpdateVoxelAround(const FVoxelWorldData& WorldData, const FIntVector& GridPosition)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FVoxelVisibility_UpdateVoxelAround);

	ChangesByType.Reset();
	UpdateVoxel(WorldData, GridPosition, &ChangesByType);
	for (const FIntVector& NeighborOffset : NeighborOffsets)
	{
		UpdateVoxel(WorldData, GridPosition + NeighborOffset, &ChangesByType);
	}
}

const FVoxelVisibilityChanges* FVoxelVisibility::GetChanges(const FName CubeId) const
{
	return ChangesByType.Find(CubeId);
}

void FVoxelVisibility::UpdateVoxel(const FVoxelWorldData& WorldData, const FIntVector& GridPosition, TMap<FName, FVoxelVisibilityChanges>* Changes)
{
	const FIntPoint ColumnPosition(GridPosition.X, GridPosition.Y);
	TMap<int32, FName>* VisibleVoxels = VisibleVoxelsByColumn.Find(ColumnPosition);
	const FName PreviousCubeId = VisibleVoxels ? VisibleVoxels->FindRef(GridPosition.Z) : NAME_None;
	const TOptional<FName> CubeId = WorldData.GetVoxelType(GridPosition);
	const FName VisibleCubeId = CubeId && HasAnyEmptyNeighbor(WorldData, GridPosition) ? *CubeId : NAME_None;
	if (PreviousCubeId == VisibleCubeId)
	{
		return;
	}

	if (Changes && VisibleArea.Contains(ColumnPosition))
	{
		if (!PreviousCubeId.IsNone())
		{
			Changes->FindOrAdd(PreviousCubeId).Removed.Add(GridPosition);
		}
		if (!VisibleCubeId.IsNone())
		{
			Changes->FindOrAdd(VisibleCubeId).Added.Add(GridPosition);
		}
	}

	if (!VisibleCubeId.IsNone())
	{
		VisibleVoxelsByColumn.FindOrAdd(ColumnPosition).Add(GridPosition.Z, VisibleCubeId);
	}
	else if (VisibleVoxels)
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
