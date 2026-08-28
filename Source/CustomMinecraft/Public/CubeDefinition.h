#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CubeDefinition.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct CUSTOMMINECRAFT_API FCubeDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cube")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cube|Rendering")
	TObjectPtr<UMaterialInterface> Material = nullptr;
};
