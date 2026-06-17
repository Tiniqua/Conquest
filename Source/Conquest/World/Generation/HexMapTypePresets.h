#pragma once

#include "CoreMinimal.h"
#include "HexTileTypes.h"
#include "HexMapTypePresets.generated.h"

UENUM(BlueprintType)
enum class EHexMapTypePreset : uint8
{
	Pangaea       UMETA(DisplayName = "Pangaea"),
	Continents   UMETA(DisplayName = "Continents"),
	Archipelago  UMETA(DisplayName = "Archipelago"),
	Fractal      UMETA(DisplayName = "Fractal"),
	Arctic       UMETA(DisplayName = "Arctic"),
	InlandSea    UMETA(DisplayName = "Inland Sea"),
	SmallIslands UMETA(DisplayName = "Small Islands"),
	Lakes        UMETA(DisplayName = "Lakes")
};

UENUM(BlueprintType)
enum class EHexLandmassStyle : uint8
{
	SingleLargeLandmass UMETA(DisplayName = "Single Large Landmass"),
	FewContinents       UMETA(DisplayName = "Few Continents"),
	ManyIslands         UMETA(DisplayName = "Many Islands"),
	FractalMixed        UMETA(DisplayName = "Fractal Mixed"),
	InlandOcean         UMETA(DisplayName = "Inland Ocean"),
	ColdWorld           UMETA(DisplayName = "Cold World"),
	LakeHeavy           UMETA(DisplayName = "Lake Heavy")
};

USTRUCT(BlueprintType)
struct FHexMapShapeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetLandRatio = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "1"))
	int32 MajorLandmassCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CoastlineNoise = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LandmassFragmentation = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IslandChainChance = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LakeFrequency = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0"))
	int32 LakeCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "1"))
	int32 LakeMinSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "1"))
	int32 LakeMaxSize = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0"))
	int32 LakeSpacing = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0"))
	int32 OceanBorderWidth = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0"))
	int32 LandmassSeparation = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape")
	bool bUseInlandSea = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InlandSeaRadiusRatio = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.0"))
	float TemperatureBiasStrength = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Shape", meta = (ClampMin = "0.1"))
	float PolarFalloffPower = 1.0f;
};

USTRUCT(BlueprintType)
struct FHexMapTypePreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Preset")
	EHexMapTypePreset PresetType = EHexMapTypePreset::Continents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Preset")
	FName PresetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Preset")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Preset", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Preset")
	EHexLandmassStyle LandmassStyle = EHexLandmassStyle::FewContinents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Preset")
	FHexMapShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Preset")
	TMap<EHexTileType, float> TerrainWeights;
};

class FHexMapTypePresets
{
public:
	static TArray<FHexMapTypePreset> GetAllPresets();
	static bool GetPreset(EHexMapTypePreset PresetType, FHexMapTypePreset& OutPreset);

private:
	static FHexMapTypePreset MakeBase(EHexMapTypePreset Type, const FName Id, const FText& Name, const FText& Description, EHexLandmassStyle LandmassStyle);
	static FHexMapTypePreset MakePangaea();
	static FHexMapTypePreset MakeContinents();
	static FHexMapTypePreset MakeArchipelago();
	static FHexMapTypePreset MakeFractal();
	static FHexMapTypePreset MakeArctic();
	static FHexMapTypePreset MakeInlandSea();
	static FHexMapTypePreset MakeSmallIslands();
	static FHexMapTypePreset MakeLakes();
};
