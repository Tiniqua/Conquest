#pragma once

#include "CoreMinimal.h"
#include "HexTileTypes.generated.h"

UENUM(BlueprintType)
enum class EHexTileType : uint8
{
	Grassland UMETA(DisplayName = "Grassland"),
	Plains    UMETA(DisplayName = "Plains"),
	Desert    UMETA(DisplayName = "Desert"),
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
	None   UMETA(DisplayName = "None"),
	Forest UMETA(DisplayName = "Forest"),
	Jungle UMETA(DisplayName = "Jungle"),
	Oasis  UMETA(DisplayName = "Oasis"),
	River  UMETA(DisplayName = "River"),
	Marsh  UMETA(DisplayName = "Marsh"),
	FloodPlains UMETA(DisplayName = "Flood Plains")
};

UENUM(BlueprintType)
enum class EHexResourceType : uint8
{
	None UMETA(DisplayName = "None"),
	Wheat UMETA(DisplayName = "Wheat"),
	Cattle UMETA(DisplayName = "Cattle"),
	Fish UMETA(DisplayName = "Fish"),
	Iron UMETA(DisplayName = "Iron"),
	Horse UMETA(DisplayName = "Horse"),
	Gold UMETA(DisplayName = "Gold"),
	Oil UMETA(DisplayName = "Oil")
};

USTRUCT(BlueprintType)
struct FHexYield
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Food = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Production = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Science = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Culture = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Faith = 0;

	FHexYield& operator+=(const FHexYield& Other)
	{
		Food += Other.Food;
		Production += Other.Production;
		Gold += Other.Gold;
		Science += Other.Science;
		Culture += Other.Culture;
		Faith += Other.Faith;
		return *this;
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
	EHexResourceType ResourceType = EHexResourceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	FHexYield Yield;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile")
	float Height = 0.0f;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	float HeightOffset = 0.0f;
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
