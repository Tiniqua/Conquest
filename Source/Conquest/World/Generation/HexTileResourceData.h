#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HexTileTypes.h"
#include "HexTileResourceData.generated.h"

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
	UMaterialInterface* Material = nullptr;
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
};

USTRUCT(BlueprintType)
struct FHexResourceDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	EHexResourceType ResourceType = EHexResourceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	FHexYield YieldModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	TArray<EHexTileType> ValidTileTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "0.0"))
	float PlacementWeight = 1.0f;
};

UCLASS(BlueprintType)
class CONQUEST_API UHexTileResourceData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiles")
	TArray<FHexTileDefinition> TileDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Features")
	TArray<FHexFeatureDefinition> FeatureDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources")
	TArray<FHexResourceDefinition> ResourceDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water")
	UMaterialInterface* WaterMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overlay")
	UMaterialInterface* GridOverlayMaterial = nullptr;

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

	const FHexResourceDefinition* FindResourceDefinition(EHexResourceType ResourceType) const
	{
		return ResourceDefinitions.FindByPredicate([ResourceType](const FHexResourceDefinition& Definition)
		{
			return Definition.ResourceType == ResourceType;
		});
	}
};
