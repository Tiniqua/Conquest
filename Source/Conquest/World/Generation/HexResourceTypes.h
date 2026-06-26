#pragma once

#include "CoreMinimal.h"
#include "Conquest/Core/ConquestModifierTypes.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Modifiers")
	TArray<FConquestModifierDefinition> Modifiers;

	// Civ V style luxuries default to +4 happiness.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Happiness", meta = (ClampMin = "0"))
	int32 Happiness = 0;

	// Used only by strategic resources. Quantity is rolled and stored on FHexTileResourceInstance.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Strategic", meta = (ClampMin = "0"))
	int32 MinStrategicQuantity = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Strategic", meta = (ClampMin = "0"))
	int32 MaxStrategicQuantity = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Strategic", meta = (ClampMin = "0"))
	int32 StartingStorageCap = 10;

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

	// -----------------------------------------------------------------------------
	// Visual
	// -----------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	// Optional material override for the resource mesh.
	// If null, the mesh keeps its default materials.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> WorldMeshMaterialOverride = nullptr;

	// Offset applied after choosing the per-instance location.
	// Z is useful for lifting rocks/plants slightly above the terrain.
	// For underwater resources like Fish, keep this near 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector MeshOffset = FVector(0.0f, 0.0f, 8.0f);

	// Base rotation. Pitch/Roll are allowed here if your mesh import needs correction,
	// but runtime random variation below only changes yaw.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

	// How many visual instances to spawn on a tile containing this resource.
	// Defaults to one instance.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1"))
	int32 MinMeshInstancesPerTile = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1"))
	int32 MaxMeshInstancesPerTile = 1;

	// How far from the tile centre instances can scatter.
	// 0.0 = centre only.
	// 1.0 = close to the hex corners.
	// Recommended: 0.35 - 0.65.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScatterRadiusRatio = 0.45f;

	// Random yaw variation in degrees.
	// Keeps the mesh vertical; only yaw changes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float RandomYawVariationDegrees = 25.0f;

	// When multiple instances are spawned, each instance scale is multiplied by:
	// 1.0 / SpawnedInstanceCount
	// This matches your requested behaviour: 3 spawned means one-third scale.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	bool bScaleDownByInstanceCount = true;

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
