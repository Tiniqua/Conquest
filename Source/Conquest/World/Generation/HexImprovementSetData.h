#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HexImprovementTypes.h"
#include "HexImprovementSetData.generated.h"

class UHexResourceSetData;

UCLASS(BlueprintType)
class CONQUEST_API UHexImprovementSetData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvements")
	TArray<FHexImprovementDefinition> Improvements;

	const FHexImprovementDefinition* FindImprovement(FName ImprovementId) const;

	UFUNCTION(BlueprintCallable, Category = "Hex Improvements")
	void GetPossibleImprovementIdsForTile(const FHexTileData& Tile, bool bTileIsWater, TArray<FName>& OutImprovementIds) const;

	void GetPossibleImprovementsForTile(const FHexTileData& Tile, bool bTileIsWater, TArray<const FHexImprovementDefinition*>& OutImprovements) const;
};
