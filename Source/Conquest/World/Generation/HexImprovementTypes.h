#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HexResourceTypes.h"
#include "HexImprovementTypes.generated.h"

class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class EHexImprovementCategory : uint8
{
	Basic    UMETA(DisplayName = "Basic"),
	Resource UMETA(DisplayName = "Resource"),
	Special  UMETA(DisplayName = "Special"),
	Unique   UMETA(DisplayName = "Unique")
};

USTRUCT(BlueprintType)
struct FHexImprovementDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement")
	FName ImprovementId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement")
	EHexImprovementCategory Category = EHexImprovementCategory::Basic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Yield")
	FHexYield YieldModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	TArray<EHexTileType> ValidTileTypes;

	// Empty means any feature state is allowed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	TArray<EHexFeatureType> ValidFeatures;

	// If true, the tile must have one of RequiredResources.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	bool bRequiresResource = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	TArray<FName> RequiredResources;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	bool bRequiresRiver = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	bool bRequiresFreshWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	bool bCannotBeBuiltOnWater = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	bool bRemovesFeature = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	TArray<EHexFeatureType> RemovedFeatures;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules", meta = (ClampMin = "1"))
	int32 BuildTurns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	TObjectPtr<UMaterialInterface> IconMaterial = nullptr;

	bool IsValidForTile(const FHexTileData& Tile, bool bTileIsWater) const
	{
		if (ImprovementId.IsNone())
		{
			return false;
		}

		if (bCannotBeBuiltOnWater && bTileIsWater)
		{
			return false;
		}

		if (ValidTileTypes.Num() > 0 && !ValidTileTypes.Contains(Tile.TileType))
		{
			return false;
		}

		if (ValidFeatures.Num() > 0)
		{
			bool bHasValidFeature = false;

			if (Tile.Features.Num() <= 0 && ValidFeatures.Contains(EHexFeatureType::None))
			{
				bHasValidFeature = true;
			}

			for (const EHexFeatureType Feature : Tile.Features)
			{
				if (ValidFeatures.Contains(Feature))
				{
					bHasValidFeature = true;
					break;
				}
			}

			if (!bHasValidFeature)
			{
				return false;
			}
		}

		if (bRequiresResource && !RequiredResources.Contains(Tile.Resource.ResourceId))
		{
			return false;
		}

		if (bRequiresRiver && !Tile.bHasRiver)
		{
			return false;
		}

		if (bRequiresFreshWater && !Tile.bHasFreshWater)
		{
			return false;
		}

		return true;
	}
};
