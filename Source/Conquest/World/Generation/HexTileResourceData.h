#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HexTileTypes.h"
#include "HexTileResourceData.generated.h"

class UMaterialInterface;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FHexTileDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	EHexTileType TileType = EHexTileType::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	FHexYield BaseYield;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	float HeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	bool bIsWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UMaterialInterface> Material = nullptr;
};

USTRUCT(BlueprintType)
struct FHexFeatureDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature")
	EHexFeatureType FeatureType = EHexFeatureType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature")
	FHexYield YieldModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature")
	TArray<EHexTileType> ValidTileTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature")
	bool bCountsAsFreshWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature")
	bool bBlocksImprovements = false;
};

// This asset now represents terrain, features, and render materials only.
// Resources live in UHexResourceSetData and improvements live in UHexImprovementSetData.
UCLASS(BlueprintType)
class CONQUEST_API UHexTileResourceData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiles")
	TArray<FHexTileDefinition> TileDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Features")
	TArray<FHexFeatureDefinition> FeatureDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UMaterialInterface> WaterMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overlay")
	TObjectPtr<UMaterialInterface> GridOverlayMaterial = nullptr;

	const FHexTileDefinition* FindTileDefinition(EHexTileType TileType) const
	{
		return TileDefinitions.FindByPredicate([TileType](const FHexTileDefinition& Definition)
		{
			return Definition.TileType == TileType;
		});
	}

	const FHexFeatureDefinition* FindFeatureDefinition(EHexFeatureType FeatureType) const
	{
		return FeatureDefinitions.FindByPredicate([FeatureType](const FHexFeatureDefinition& Definition)
		{
			return Definition.FeatureType == FeatureType;
		});
	}
};
