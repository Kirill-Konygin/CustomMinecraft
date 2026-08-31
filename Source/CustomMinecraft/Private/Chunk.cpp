#include "Chunk.h"

FChunk::FChunk(const FIntVector& InSize)
	: Size(InSize)
{
	check(Size.X > 0 && Size.Y > 0 && Size.Z > 0);
	Voxels.Init(EmptyPaletteIndex, Size.X * Size.Y * Size.Z);
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
	return GetVoxelPaletteIndex(LocalPosition) != EmptyPaletteIndex;
}

TOptional<FName> FChunk::GetVoxelType(const FIntVector& LocalPosition) const
{
	const FPaletteIndex PaletteIndex = GetVoxelPaletteIndex(LocalPosition);
	if (PaletteIndex == EmptyPaletteIndex || !Palette.IsValidIndex(PaletteIndex))
	{
		return {};
	}

	return Palette[PaletteIndex];
}

FChunk::FPaletteIndex FChunk::GetVoxelPaletteIndex(const FIntVector& LocalPosition) const
{
	return IsValidLocalPosition(LocalPosition) ? Voxels[GetVoxelIndex(LocalPosition)] : EmptyPaletteIndex;
}

bool FChunk::SetVoxel(const FIntVector& LocalPosition, const FName CubeId)
{
	if (!IsValidLocalPosition(LocalPosition))
	{
		return false;
	}

	const int32 VoxelIndex = GetVoxelIndex(LocalPosition);
	const FPaletteIndex PaletteIndex = CubeId.IsNone() ? EmptyPaletteIndex : FindOrAddPaletteIndex(CubeId);
	if (Voxels[VoxelIndex] == PaletteIndex)
	{
		return false;
	}

	Voxels[VoxelIndex] = PaletteIndex;
	return true;
}

bool FChunk::RemoveVoxel(const FIntVector& LocalPosition)
{
	return SetVoxel(LocalPosition, NAME_None);
}

TArray<FIntVector> FChunk::GetVoxelLocalPositions() const
{
	TArray<FIntVector> LocalPositions;

	for (int32 Index = 0; Index < Voxels.Num(); ++Index)
	{
		if (Voxels[Index] != EmptyPaletteIndex && HasAnyEmptyNeighbor(Index))
		{
			LocalPositions.Add(GetLocalPosition(Index));
		}
	}

	return LocalPositions;
}

TArray<FIntVector> FChunk::GetVoxelLocalPositions(const FName CubeId) const
{
	TArray<FIntVector> LocalPositions;
	const FPaletteIndex* PaletteIndex = PaletteIndexByCubeId.Find(CubeId);
	if (!PaletteIndex)
	{
		return LocalPositions;
	}

	for (int32 Index = 0; Index < Voxels.Num(); ++Index)
	{
		if (Voxels[Index] == *PaletteIndex && HasAnyEmptyNeighbor(Index))
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

FChunk::FPaletteIndex FChunk::FindOrAddPaletteIndex(const FName CubeId)
{
	if (const FPaletteIndex* ExistingIndex = PaletteIndexByCubeId.Find(CubeId))
	{
		return *ExistingIndex;
	}

	checkf(Palette.Num() < EmptyPaletteIndex, TEXT("Chunk palette cannot contain more than %u cube types"), EmptyPaletteIndex);
	const FPaletteIndex NewIndex = static_cast<FPaletteIndex>(Palette.Add(CubeId));
	PaletteIndexByCubeId.Add(CubeId, NewIndex);
	return NewIndex;
}

bool FChunk::HasAnyEmptyNeighbor(int32 Index) const
{
	if (!Voxels.IsValidIndex(Index))
	{
		return false;
	}

	const FIntVector LocalPosition = GetLocalPosition(Index);
	static const FIntVector NeighborOffsets[] =
	{
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0),
		FIntVector(0, 0, 1),
		FIntVector(0, 0, -1)
	};

	for (const FIntVector& NeighborOffset : NeighborOffsets)
	{
		if (GetVoxelPaletteIndex(LocalPosition + NeighborOffset) == EmptyPaletteIndex)
		{
			return true;
		}
	}

	return false;
}
