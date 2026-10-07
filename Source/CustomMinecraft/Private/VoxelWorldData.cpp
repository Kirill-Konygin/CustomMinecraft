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

bool FVoxelWorldData::HasData(const FIntVector& GridPosition) const
{
	return GridPosition.Z >= 0 && GridPosition.Z < RegionSize.Z && FindRegion(GetRegionPosition(GridPosition));
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
	if (GridPosition.Z < 0 || GridPosition.Z >= RegionSize.Z)
	{
		return false;
	}

	const FIntPoint RegionPosition = GetRegionPosition(GridPosition);
	TUniquePtr<FChunk>* Region = Regions.Find(RegionPosition);
	const bool bHasRegion = Region != nullptr;
	if (!Region)
	{
		Region = &Regions.Add(RegionPosition, MakeUnique<FChunk>(RegionSize));
	}

	const bool bVoxelChanged = (*Region)->SetVoxel(GetRegionLocalPosition(GridPosition), CubeId);
	return !bHasRegion || bVoxelChanged;
}

bool FVoxelWorldData::RemoveVoxel(const FIntVector& GridPosition)
{
	return HasVoxel(GridPosition) && SetVoxel(GridPosition, NAME_None);
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

const FChunk* FVoxelWorldData::FindRegion(const FIntPoint& RegionPosition) const
{
	const TUniquePtr<FChunk>* FoundRegion = Regions.Find(RegionPosition);
	return FoundRegion ? FoundRegion->Get() : nullptr;
}
