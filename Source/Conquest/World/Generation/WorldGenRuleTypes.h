#pragma once

#include "CoreMinimal.h"
#include "WorldGenRuleTypes.generated.h"

UENUM(BlueprintType)
enum class EWorldGenTileType : uint8
{
	Grassland       UMETA(DisplayName = "Grassland"),
	GrasslandHill   UMETA(DisplayName = "Grassland Hill"),

	Plains          UMETA(DisplayName = "Plains"),
	PlainsHill      UMETA(DisplayName = "Plains Hill"),

	Desert          UMETA(DisplayName = "Desert"),
	DesertHill      UMETA(DisplayName = "Desert Hill"),

	Tundra          UMETA(DisplayName = "Tundra"),
	TundraHill      UMETA(DisplayName = "Tundra Hill"),

	Snow            UMETA(DisplayName = "Snow"),
	SnowHill        UMETA(DisplayName = "Snow Hill"),

	Coast           UMETA(DisplayName = "Coast"),
	Ocean           UMETA(DisplayName = "Ocean"),

	Mountain        UMETA(DisplayName = "Mountain")
};

UENUM(BlueprintType)
enum class EWorldGenBaseTerrain : uint8
{
	Grassland UMETA(DisplayName = "Grassland"),
	Plains    UMETA(DisplayName = "Plains"),
	Desert    UMETA(DisplayName = "Desert"),
	Tundra    UMETA(DisplayName = "Tundra"),
	Snow      UMETA(DisplayName = "Snow"),
	Coast     UMETA(DisplayName = "Coast"),
	Ocean     UMETA(DisplayName = "Ocean"),
	Mountain  UMETA(DisplayName = "Mountain")
};

UENUM(BlueprintType)
enum class EWorldGenElevationType : uint8
{
	Flat     UMETA(DisplayName = "Flat"),
	Hill     UMETA(DisplayName = "Hill"),
	Mountain UMETA(DisplayName = "Mountain")
};

UENUM(BlueprintType)
enum class EWorldGenTransitionStyle : uint8
{
	None  UMETA(DisplayName = "None"),
	Slope UMETA(DisplayName = "Slope"),
	Cliff UMETA(DisplayName = "Cliff")
};

USTRUCT(BlueprintType)
struct FWorldGenTileRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	EWorldGenTileType TileType = EWorldGenTileType::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	EWorldGenBaseTerrain BaseTerrain = EWorldGenBaseTerrain::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	EWorldGenElevationType ElevationType = EWorldGenElevationType::Flat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	float Height = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	bool bIsWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	bool bBlocksMovement = false;

	// Higher value means this tile is more likely to own the visual transition.
	// Example: Hill should own Flat <-> Hill slope so hill material appears on the slope.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	int32 TransitionPriority = 0;

	// Mesh section/material slot used by the procedural actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	int32 MaterialSlot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	FName DebugName = NAME_None;
};

USTRUCT(BlueprintType)
struct FWorldGenTransitionRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	bool bHasTransition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	bool bThisTileOwnsTransition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	EWorldGenTransitionStyle TransitionStyle = EWorldGenTransitionStyle::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	EWorldGenTileType TransitionMaterialTileType = EWorldGenTileType::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	float ThisHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	float OtherHeight = 0.0f;
};

class CONQUEST_API FWorldGenRuleTypes
{
public:
	static TArray<EWorldGenTileType> GetAllTileTypes();

	static FWorldGenTileRule GetTileRule(
		EWorldGenTileType TileType,
		float FlatHeight,
		float HillHeight,
		float MountainHeight,
		float CoastHeight,
		float OceanHeight
	);

	static FWorldGenTransitionRule GetTransitionRule(
		EWorldGenTileType ThisTileType,
		EWorldGenTileType OtherTileType,
		float FlatHeight,
		float HillHeight,
		float MountainHeight,
		float CoastHeight,
		float OceanHeight
	);

	static bool AreTilesSeamless(
		EWorldGenTileType A,
		EWorldGenTileType B
	);

	static bool IsWater(EWorldGenTileType TileType);
	static bool IsHill(EWorldGenTileType TileType);
	static bool IsMountain(EWorldGenTileType TileType);

	static EWorldGenBaseTerrain GetBaseTerrain(EWorldGenTileType TileType);
	static EWorldGenElevationType GetElevationType(EWorldGenTileType TileType);

	static EWorldGenTileType GetFlatVariantForBaseTerrain(EWorldGenBaseTerrain BaseTerrain);
	static EWorldGenTileType GetHillVariantForBaseTerrain(EWorldGenBaseTerrain BaseTerrain);

private:
	static EWorldGenTileType ChooseTransitionOwner(
		EWorldGenTileType A,
		EWorldGenTileType B,
		const FWorldGenTileRule& RuleA,
		const FWorldGenTileRule& RuleB
	);
};