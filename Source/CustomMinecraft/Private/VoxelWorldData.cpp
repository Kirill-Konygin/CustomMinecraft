#include "VoxelWorldData.h"

void FVoxelWorldData::Initialize(const FIntVector& InRegionSize)
{
	check(InRegionSize.X > 0 && InRegionSize.Y > 0 && InRegionSize.Z > 0);
	check(Regions.IsEmpty());

	RegionSize = InRegionSize;
}

bool FVoxelWorldData::IsEmpty() const
{
	return Regions.IsEmpty();
}

bool FVoxelWorldData::IsInSameRegion(const FIntPoint& FirstGridPosition, const FIntPoint& SecondGridPosition) const
{
	return GetRegionPosition(FirstGridPosition) == GetRegionPosition(SecondGridPosition);
}

void FVoxelWorldData::EnsureAreaAround(const FIntPoint& CenterGridPosition, const int32 Radius, const FAreaGenerator GenerateArea)
{
	check(Radius >= 0);

	const FIntPoint CenterRegionPosition = GetRegionPosition(CenterGridPosition);
	for (int32 X = -Radius; X <= Radius; ++X)
	{
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			const FIntPoint RegionPosition = CenterRegionPosition + FIntPoint(X, Y);
			if (Regions.Contains(RegionPosition))
			{
				continue;
			}

			Regions.Add(RegionPosition, MakeUnique<FChunk>(RegionSize));

			const FIntPoint AreaMin(RegionPosition.X * RegionSize.X, RegionPosition.Y * RegionSize.Y);
			const FIntPoint AreaMax = AreaMin + FIntPoint(RegionSize.X, RegionSize.Y);
			GenerateArea(FIntRect(AreaMin, AreaMax));
		}
	}
}

TArray<FIntVector> FVoxelWorldData::GetVoxelPositionsAround(const FName CubeId, const FIntPoint& CenterGridPosition, const int32 Radius) const
{
	check(Radius >= 0);

	TArray<FIntVector> Positions;
	const FIntPoint CenterRegionPosition = GetRegionPosition(CenterGridPosition);

	for (int32 X = -Radius; X <= Radius; ++X)
	{
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			const FIntPoint RegionPosition = CenterRegionPosition + FIntPoint(X, Y);
			const FChunk* Region = FindRegion(RegionPosition);
			if (!Region)
			{
				continue;
			}

			const FIntVector RegionOffset(
				RegionPosition.X * RegionSize.X,
				RegionPosition.Y * RegionSize.Y,
				0);
			for (const FIntVector& LocalPosition : Region->GetVoxelLocalPositions(CubeId))
			{
				Positions.Add(RegionOffset + LocalPosition);
			}
		}
	}

	return Positions;
}

bool FVoxelWorldData::HasVoxel(const FIntVector& GridPosition) const
{
	return GetVoxelType(GridPosition).IsSet();
}

TOptional<FName> FVoxelWorldData::GetVoxelType(const FIntVector& GridPosition) const
{
	const FChunk* Region = FindRegion(GetRegionPosition(GridPosition));
	return Region ? Region->GetVoxelType(GetRegionLocalPosition(GridPosition)) : TOptional<FName>();
}

bool FVoxelWorldData::SetVoxel(const FIntVector& GridPosition, const FName CubeId)
{
	FChunk* Region = FindRegion(GetRegionPosition(GridPosition));
	return Region && Region->SetVoxel(GetRegionLocalPosition(GridPosition), CubeId);
}

bool FVoxelWorldData::RemoveVoxel(const FIntVector& GridPosition)
{
	FChunk* Region = FindRegion(GetRegionPosition(GridPosition));
	return Region && Region->RemoveVoxel(GetRegionLocalPosition(GridPosition));
}

TOptional<int32> FVoxelWorldData::FindSurfaceZ(const FIntPoint& GridPosition) const
{
	const FChunk* Region = FindRegion(GetRegionPosition(GridPosition));
	if (!Region)
	{
		return {};
	}

	const FIntVector LocalPosition = GetRegionLocalPosition(FIntVector(GridPosition.X, GridPosition.Y, 0));
	for (int32 GridZ = RegionSize.Z - 1; GridZ >= 0; --GridZ)
	{
		if (Region->HasVoxel(FIntVector(LocalPosition.X, LocalPosition.Y, GridZ)))
		{
			return GridZ;
		}
	}

	return INDEX_NONE;
}

FIntPoint FVoxelWorldData::GetRegionPosition(const FIntPoint& GridPosition) const
{
	return GetRegionPosition(FIntVector(GridPosition.X, GridPosition.Y, 0));
}

FIntPoint FVoxelWorldData::GetRegionPosition(const FIntVector& GridPosition) const
{
	return FIntPoint(
		FMath::FloorToInt(static_cast<double>(GridPosition.X) / RegionSize.X),
		FMath::FloorToInt(static_cast<double>(GridPosition.Y) / RegionSize.Y));
}

FIntVector FVoxelWorldData::GetRegionLocalPosition(const FIntVector& GridPosition) const
{
	const FIntPoint RegionPosition = GetRegionPosition(GridPosition);
	return FIntVector(
		GridPosition.X - RegionPosition.X * RegionSize.X,
		GridPosition.Y - RegionPosition.Y * RegionSize.Y,
		GridPosition.Z);
}

FChunk* FVoxelWorldData::FindRegion(const FIntPoint& RegionPosition)
{
	TUniquePtr<FChunk>* FoundRegion = Regions.Find(RegionPosition);
	return FoundRegion ? FoundRegion->Get() : nullptr;
}

const FChunk* FVoxelWorldData::FindRegion(const FIntPoint& RegionPosition) const
{
	const TUniquePtr<FChunk>* FoundRegion = Regions.Find(RegionPosition);
	return FoundRegion ? FoundRegion->Get() : nullptr;
}
