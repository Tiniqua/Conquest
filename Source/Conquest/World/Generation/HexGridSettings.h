#pragma once

#include "CoreMinimal.h"
#include "HexTileTypes.h"
#include "HexGridSettings.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
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