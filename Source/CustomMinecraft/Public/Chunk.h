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
	bool SetVoxel(const FIntVector& LocalPosition, bool bIsSolid);

	TArray<FIntVector> GetVoxelLocalPositions() const;

private:
	int32 GetVoxelIndex(const FIntVector& LocalPosition) const;

	FIntVector Size = FIntVector(1, 1, 1);
	TArray<bool> Voxels;
};
