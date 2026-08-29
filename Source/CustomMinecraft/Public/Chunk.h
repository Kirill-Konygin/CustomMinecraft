#pragma once

#include "CoreMinimal.h"

class CUSTOMMINECRAFT_API FChunk final
{
public:
	explicit FChunk(const FIntVector& InSize);

	const FIntVector& GetSize() const { return Size; }
	FIntVector GetLocalPosition(int32 Index) const;

	bool IsValidLocalPosition(const FIntVector& LocalPosition) const;
	bool HasVoxel(const FIntVector& LocalPosition) const;
	TOptional<FName> GetVoxelType(const FIntVector& LocalPosition) const;
	bool SetVoxel(const FIntVector& LocalPosition, FName CubeId);
	bool RemoveVoxel(const FIntVector& LocalPosition);

	TArray<FIntVector> GetVoxelLocalPositions() const;

private:
	using FPaletteIndex = uint16;
	static constexpr FPaletteIndex EmptyPaletteIndex = MAX_uint16;

	int32 GetVoxelIndex(const FIntVector& LocalPosition) const;
	FPaletteIndex GetVoxelPaletteIndex(const FIntVector& LocalPosition) const;
	FPaletteIndex FindOrAddPaletteIndex(FName CubeId);

	FIntVector Size = FIntVector(1, 1, 1);
	TArray<FName> Palette;
	TMap<FName, FPaletteIndex> PaletteIndexByCubeId;
	TArray<FPaletteIndex> Voxels;
};
