#pragma once

#include "CoreMinimal.h"
#include "HexTileTypes.h"
#include "HexMapTypePresets.h"
#include "HexGridSettings.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct FHexGridSizeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Size", meta = (ClampMin = "1"))
	int32 GridWidth = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Size", meta = (ClampMin = "1"))
	int32 GridHeight = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Size", meta = (ClampMin = "1.0"))
	float HexRadius = 100.0f;
};

USTRUCT(BlueprintType)
struct FHexFeatureGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Features")
	bool bGenerateFeatures = true;

	// Optional explicit total. If <= 0, each feature uses its own AutoDensity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Features", meta = (ClampMin = "0"))
	int32 TotalFeatureCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Features", meta = (ClampMin = "1"))
	int32 SeedSelectionAttempts = 64;

	// Prevents the generator from looping forever when valid terrain is rare.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Features", meta = (ClampMin = "1"))
	int32 MaxPlacementAttemptsMultiplier = 20;
};

USTRUCT(BlueprintType)
struct FHexHeightSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Height")
	bool bUseHeightOffsets = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Height")
	float HeightScale = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Height", meta = (ClampMin = "1.0"))
	float VertexKeyPrecision = 10.0f;
};

USTRUCT(BlueprintType)
struct FHexTemperatureSettings
{
	GENERATED_BODY()

	// Enables north/south temperature bias.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	bool bUseTemperatureBias = true;

	// How strongly temperature affects terrain placement.
	// 0 = no effect, 1 = noticeable, 2+ = strong.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = "0.0"))
	float TemperatureBiasStrength = 2.0f;

	// Controls how wide the cold polar bands are.
	// Higher values make the cold area tighter to the top/bottom.
	// Lower values make cold reach further toward the middle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = "0.1"))
	float PolarFalloffPower = 1.0f;

	// Small noise so terrain bands do not look perfectly horizontal.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = "0.0"))
	float TemperatureNoiseStrength = 0.12f;
};

USTRUCT(BlueprintType)
struct FHexGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation")
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Map Type")
	EHexMapTypePreset MapTypePreset = EHexMapTypePreset::Continents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Map Type")
	FHexMapShapeSettings MapShapeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Map Type")
	bool bUseCustomMapShapeSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CountRandomness = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float SameTypeAdjacencyBonus = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float PreferredAdjacencyBonus = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float AvoidAdjacencyPenalty = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float HardAvoidAdjacencyPenalty = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation", meta = (ClampMin = "1"))
	int32 SeedSelectionAttempts = 48;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float MountainWeightScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Temperature")
	FHexTemperatureSettings TemperatureSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Generation")
	TArray<FHexTileGenerationRule> GenerationRules;
};

USTRUCT(BlueprintType)
struct FHexResourceGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources")
	bool bGenerateResources = true;

	// If a count is <= 0, the generator computes a simple automatic count from tile count and density.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources", meta = (ClampMin = "0"))
	int32 BonusResourceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources", meta = (ClampMin = "0"))
	int32 LuxuryResourceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources", meta = (ClampMin = "0"))
	int32 StrategicResourceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AutoBonusDensity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AutoLuxuryDensity = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AutoStrategicDensity = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Resources", meta = (ClampMin = "0"))
	int32 ResourceMinSpacing = 2;
};

USTRUCT(BlueprintType)
struct FHexSimpleRiverEdge
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Simple Rivers")
	FIntPoint Tile = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Simple Rivers")
	int32 EdgeIndex = 0;
};

USTRUCT(BlueprintType)
struct FHexSimpleRiverPath
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Simple Rivers")
	int32 RiverId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Simple Rivers")
	TArray<FHexSimpleRiverEdge> Edges;
};

USTRUCT(BlueprintType)
struct FHexSimpleRiverSettings
{
	GENERATED_BODY()

	FHexSimpleRiverSettings()
	{
		AvoidTileTypes = {
			EHexTileType::Coast,
			EHexTileType::Ocean,
			EHexTileType::Mountain
		};
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers")
	bool bGenerateRivers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers")
	bool bShowRiverLayer = true;

	// Rivers generated per 100 tiles before MaxRiverCount is applied.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "0.0"))
	float RiverDensityPer100Tiles = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "0"))
	int32 MaxRiverCount = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "1"))
	int32 MinRiverLength = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "1"))
	int32 MaxRiverLength = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "1"))
	int32 MaxStartAttemptsPerRiver = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "0"))
	int32 RiverAvoidanceRadius = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExistingRiverAvoidanceChance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "1", ClampMax = "6"))
	int32 MaxRiverEdgesPerTile = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers")
	TArray<EHexTileType> AvoidTileTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "0.0"))
	float ForwardBias = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "0.0"))
	float TurnBias = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers", meta = (ClampMin = "1.0"))
	float RiverWidth = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers")
	float RiverSurfaceOffset = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers")
	int32 TranslucencySortPriority = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Simple Rivers")
	TObjectPtr<UMaterialInterface> RiverMaterial = nullptr;
};

USTRUCT(BlueprintType)
struct FHexWaterSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Water")
	bool bShowWaterLayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Water")
	float WaterSurfaceZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Water")
	float WaterSurfaceRenderOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Water")
	bool bFillAdjacentSlopedLandWaterGaps = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Water", meta = (ClampMin = "0.0"))
	float WaterGapFillBelowSurfaceTolerance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Water", meta = (ClampMin = "0.0"))
	float WaterGapFillMinSlopeDelta = 1.0f;
};

USTRUCT(BlueprintType)
struct FHexOverlaySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Overlay")
	bool bShowHexGridOverlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Overlay", meta = (ClampMin = "0.1"))
	float GridLineWidth = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Overlay", meta = (ClampMin = "0.0"))
	float GridOverlaySurfaceOffset = 2.0f;
};

USTRUCT(BlueprintType)
struct FHexFogOfWarSettings
{
	GENERATED_BODY()

	// Material using your cloud / fog texture.
	// Recommended material setup:
	// Blend Mode: Translucent or Masked
	// Shading Model: Unlit
	// Two Sided: true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Of War")
	TObjectPtr<UMaterialInterface> FogMaterial = nullptr;

	// FOW is generated at:
	// HighestTileHeight + HeightAboveHighestTile
	// so the whole layer is flat and safely above mountains/resources.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Of War")
	float HeightAboveHighestTile = 350.0f;

	// Makes each FOW hex slightly larger than terrain hexes.
	// Useful to hide tiny seams between translucent hexes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Of War", meta = (ClampMin = "0.9", ClampMax = "1.25"))
	float HexScale = 1.01f;

	// Optional UV tiling per hex. 1 = one full texture per hex.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Of War", meta = (ClampMin = "0.01"))
	float UVScale = 1.0f;

	// Higher than water/overlay so the fog draws on top.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Of War")
	int32 TranslucencySortPriority = 10;
};
