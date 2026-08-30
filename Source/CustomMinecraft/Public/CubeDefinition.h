#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "CubeDefinition.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct CUSTOMMINECRAFT_API FCubeDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cube")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cube", meta = (ClampMin = "0.1"))
	float MiningDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cube|Rendering")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cube|Generation", meta = (Categories = "World.Layer"))
	FGameplayTag GenerationLayer;
};
