#pragma once

#include "CoreMinimal.h"
#include "HexYieldTypes.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "HexTileTypes.generated.h"

UENUM(BlueprintType)
enum class EHexTileType : uint8
{
	Grassland UMETA(DisplayName = "Grassland"),
	Plains    UMETA(DisplayName = "Plains"),
	Desert    UMETA(DisplayName = "Desert"),
	Jungle    UMETA(DisplayName = "Jungle"),
	Tundra    UMETA(DisplayName = "Tundra"),
	Snow      UMETA(DisplayName = "Snow"),
	Coast     UMETA(DisplayName = "Coast"),
	Ocean     UMETA(DisplayName = "Ocean"),
	Lake      UMETA(DisplayName = "Lake"),
	Mountain  UMETA(DisplayName = "Mountain")
};

UENUM(BlueprintType)
enum class EHexFeatureType : uint8
{
	None        UMETA(DisplayName = "None"),
	Forest      UMETA(DisplayName = "Forest"),
	Jungle      UMETA(DisplayName = "Jungle"),
	Hill      UMETA(DisplayName = "Hill"),
	Oasis       UMETA(DisplayName = "Oasis"),
	Marsh       UMETA(DisplayName = "Marsh"),
	FloodPlains UMETA(DisplayName = "Flood Plains")
};

UENUM(BlueprintType)
enum class EHexFeaturePlacementPattern : uint8
{
	RandomSparse     UMETA(DisplayName = "Random Sparse"),
	RandomIntermittent UMETA(DisplayName = "Random Intermittent"),
	SmallClumps     UMETA(DisplayName = "Small Clumps"),
	BigClumps        UMETA(DisplayName = "Big Clumps")
};

USTRUCT(BlueprintType)
struct FHexTileResourceInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Resource")
	FName ResourceId = NAME_None;

	// Used by strategic resources. Bonus/luxury resources normally leave this as 0 or 1.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Resource")
	int32 Quantity = 0;

	bool HasResource() const
	{
		return !ResourceId.IsNone();
	}
};


USTRUCT(BlueprintType)
struct FHexTileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	EHexTileType TileType = EHexTileType::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	TArray<EHexFeatureType> Features;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	FHexTileResourceInstance Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	FName ImprovementId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	bool bHasRiver = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	bool bHasFreshWater = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile")
	FHexYield FinalYield;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile")
	float Height = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexTileGameplayState Gameplay;
};

USTRUCT(BlueprintType)
struct FHexTileGenerationRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	EHexTileType TileType = EHexTileType::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "1"))
	int32 MinClumpSize = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "1"))
	int32 MaxClumpSize = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<EHexTileType> PreferredAdjacentTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<EHexTileType> AvoidAdjacentTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<EHexTileType> RequiredAdjacentTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	bool bSoftCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	bool bRejectBadAdjacency = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	float MinPlacementScore = 0.5f;

	// Used as fallback if your terrain data asset has no definition for this tile type.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	float HeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Height", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RingVertexHeightVariancePercent = 0.0f;

	// 0 = cold/polar, 1 = hot/equator.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PreferredTemperature = 0.5f;

	// How far from PreferredTemperature this tile can comfortably appear.
	// Smaller = stricter banding.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float TemperatureTolerance = 0.5f;

	// How strongly this specific rule cares about temperature.
	// Snow/desert high, grass/plains medium, water maybe low.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature", meta = (ClampMin = "0.0"))
	float TemperatureWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature")
	bool bUseTemperatureRangeGate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinAllowedTemperature = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxAllowedTemperature = 1.0f;
	
	// If true, this terrain cannot be placed when temperature suitability is too low.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature")
	bool bRejectBadTemperature = false;

	// Minimum suitability required if bRejectBadTemperature is true.
	// 0 = no restriction, 1 = must be perfect.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation|Temperature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinTemperatureSuitability = 0.0f;
};

struct FHexVertexKey
{
	int32 X = 0;
	int32 Y = 0;

	bool operator==(const FHexVertexKey& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
};

FORCEINLINE uint32 GetTypeHash(const FHexVertexKey& Key)
{
	uint32 Hash = ::GetTypeHash(Key.X);
	Hash = HashCombine(Hash, ::GetTypeHash(Key.Y));
	return Hash;
}
