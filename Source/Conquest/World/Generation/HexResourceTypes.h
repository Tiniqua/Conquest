#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HexTileTypes.h"
#include "HexResourceTypes.generated.h"

class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class EHexResourceCategory : uint8
{
	Bonus     UMETA(DisplayName = "Bonus"),
	Luxury    UMETA(DisplayName = "Luxury"),
	Strategic UMETA(DisplayName = "Strategic")
};

USTRUCT(BlueprintType)
struct FHexResourceDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	FName ResourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	EHexResourceCategory Category = EHexResourceCategory::Bonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Yield")
	FHexYield YieldModifier;

	// Civ V style luxuries default to +4 happiness.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Happiness", meta = (ClampMin = "0"))
	int32 Happiness = 0;

	// Used only by strategic resources. Quantity is rolled and stored on FHexTileResourceInstance.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Strategic", meta = (ClampMin = "0"))
	int32 MinStrategicQuantity = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Strategic", meta = (ClampMin = "0"))
	int32 MaxStrategicQuantity = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Placement")
	TArray<EHexTileType> ValidTileTypes;

	// Empty means any feature state is allowed. Include None to explicitly allow featureless tiles.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Placement")
	TArray<EHexFeatureType> ValidFeatures;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Placement")
	bool bCanAppearWithoutFeature = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Placement")
	bool bRequiresRiver = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Placement")
	bool bRequiresFreshWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Placement", meta = (ClampMin = "0.0"))
	float PlacementWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector MeshOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Visual")
	TObjectPtr<UMaterialInterface> IconMaterial = nullptr;

	bool IsValidForTile(const FHexTileData& Tile) const
	{
		if (ResourceId.IsNone())
		{
			return false;
		}

		if (ValidTileTypes.Num() > 0 && !ValidTileTypes.Contains(Tile.TileType))
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

		if (ValidFeatures.Num() <= 0)
		{
			return bCanAppearWithoutFeature || Tile.Features.Num() > 0;
		}

		if (Tile.Features.Num() <= 0)
		{
			return bCanAppearWithoutFeature || ValidFeatures.Contains(EHexFeatureType::None);
		}

		for (const EHexFeatureType Feature : Tile.Features)
		{
			if (ValidFeatures.Contains(Feature))
			{
				return true;
			}
		}

		return false;
	}
};
