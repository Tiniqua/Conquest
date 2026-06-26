#pragma once

#include "CoreMinimal.h"
#include "Conquest/Core/ConquestModifierTypes.h"
#include "Engine/DataTable.h"
#include "ConquestProceduralPlaceholderTypes.h"
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
struct FHexImprovementDefinition : public FTableRowBase
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Cost", meta = (ClampMin = "0"))
	int32 PurchaseGoldCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Combat", meta = (ClampMin = "0"))
	int32 TileCombatStrengthBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Combat", meta = (ClampMin = "0"))
	int32 TileHealRateBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Combat", meta = (ClampMin = "0.0"))
	float UnitDefenderModifierBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Modifiers")
	TArray<FConquestModifierDefinition> Modifiers;

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

	// If true, any visible resource blocks this improvement unless that resource is explicitly required.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	bool bBlockedByAnyResource = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Rules")
	TArray<FName> BlockedResources;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	TObjectPtr<UMaterialInterface> WorldMeshMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	FVector MeshOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Visual")
	TObjectPtr<UMaterialInterface> IconMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Improvement|Procedural Visual")
	FConquestProceduralPlaceholderVisual ProceduralVisual;

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

		if (Tile.Resource.HasResource())
		{
			const bool bResourceIsRequired =
				bRequiresResource && RequiredResources.Contains(Tile.Resource.ResourceId);

			if (!bResourceIsRequired && bBlockedByAnyResource)
			{
				return false;
			}

			if (BlockedResources.Contains(Tile.Resource.ResourceId))
			{
				return false;
			}
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
