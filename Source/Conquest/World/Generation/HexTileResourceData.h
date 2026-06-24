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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Placement")
	TArray<EHexTileType> ValidTileTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Placement")
	bool bCountsAsFreshWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Placement")
	bool bBlocksImprovements = false;

	// -------------------------------------------------------------------------
	// Generation
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation")
	bool bGenerateFeature = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation", meta = (ClampMin = "0.0"))
	float PlacementWeight = 1.0f;

	// Used when no explicit count is provided.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AutoDensity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation")
	EHexFeaturePlacementPattern PlacementPattern = EHexFeaturePlacementPattern::SmallClumps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation", meta = (ClampMin = "1"))
	int32 MinClumpSize = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation", meta = (ClampMin = "1"))
	int32 MaxClumpSize = 8;

	// Higher means this feature strongly prefers growing beside itself.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation", meta = (ClampMin = "0.0"))
	float SameFeatureAdjacencyBias = 4.0f;

	// Higher means isolated placement is less likely during clump growth.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation", meta = (ClampMin = "0.0"))
	float IsolationPenalty = 1.5f;

	// Allows some broken/non-perfect clumps.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Randomness = 0.25f;

	// If false, this feature cannot appear on a tile that already has another non-None feature.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Generation")
	bool bCanStackWithOtherFeatures = false;

	// -------------------------------------------------------------------------
	// Visual
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual")
	TObjectPtr<UMaterialInterface> WorldMeshMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual")
	FVector MeshOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual")
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual")
	bool bOverrideMeshScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual", meta = (EditCondition = "bOverrideMeshScale"))
	FVector MeshScaleOverride = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual", meta = (ClampMin = "1"))
	int32 MinMeshInstancesPerTile = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual", meta = (ClampMin = "1"))
	int32 MaxMeshInstancesPerTile = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScatterRadiusRatio = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float RandomYawVariationDegrees = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feature|Visual")
	bool bScaleDownByInstanceCount = true;

	bool IsValidForTile(const FHexTileData& Tile) const
	{
		if (FeatureType == EHexFeatureType::None)
		{
			return false;
		}

		if (ValidTileTypes.Num() > 0 && !ValidTileTypes.Contains(Tile.TileType))
		{
			return false;
		}

		if (!bCanStackWithOtherFeatures)
		{
			for (const EHexFeatureType ExistingFeature : Tile.Features)
			{
				if (ExistingFeature != EHexFeatureType::None)
				{
					return false;
				}
			}
		}

		return true;
	}
};

// This asset now represents terrain, features, and render materials only.
// Resources live in UHexResourceSetData and improvements are authored in a FHexImprovementDefinition data table.
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
