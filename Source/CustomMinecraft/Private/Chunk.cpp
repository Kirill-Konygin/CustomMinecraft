#include "Chunk.h"

FChunk::FChunk(const FIntVector& InSize)
	: Size(InSize)
{
	check(Size.X > 0 && Size.Y > 0 && Size.Z > 0);
	Voxels.Init(false, Size.X * Size.Y * Size.Z);
}

FIntVector FChunk::GetLocalPosition(const int32 Index) const
{
	if (!Voxels.IsValidIndex(Index))
	{
		return FIntVector(INDEX_NONE, INDEX_NONE, INDEX_NONE);
	}

	const int32 LayerSize = Size.X * Size.Y;
	return FIntVector(Index % Size.X, (Index / Size.X) % Size.Y, Index / LayerSize);
}

bool FChunk::IsValidLocalPosition(const FIntVector& LocalPosition) const
{
	return		LocalPosition.X >= 0 && LocalPosition.X < Size.X
			&&	LocalPosition.Y >= 0 && LocalPosition.Y < Size.Y
			&&	LocalPosition.Z >= 0 && LocalPosition.Z < Size.Z;
}

bool FChunk::HasVoxel(const FIntVector& LocalPosition) const
{
	return IsValidLocalPosition(LocalPosition) && Voxels[GetVoxelIndex(LocalPosition)];
}

bool FChunk::SetVoxel(const FIntVector& LocalPosition, const bool bIsSolid)
{
	if (!IsValidLocalPosition(LocalPosition))
	{
		return false;
	}

	const int32 VoxelIndex = GetVoxelIndex(LocalPosition);
	if (Voxels[VoxelIndex] == bIsSolid)
	{
		return false;
	}

	Voxels[VoxelIndex] = bIsSolid;
	return true;
}

TArray<FIntVector> FChunk::GetVoxelLocalPositions() const
{
	TArray<FIntVector> LocalPositions;

	for (int32 Index = 0; Index < Voxels.Num(); ++Index)
	{
		if (Voxels[Index])
		{
			LocalPositions.Add(GetLocalPosition(Index));
		}
	}

	return LocalPositions;
}

int32 FChunk::GetVoxelIndex(const FIntVector& LocalPosition) const
{
	return LocalPosition.X + Size.X * (LocalPosition.Y + Size.Y * LocalPosition.Z);
}
