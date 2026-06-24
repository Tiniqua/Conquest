#pragma once

#include "CoreMinimal.h"
#include "Conquest/World/Generation/HexYieldTypes.h"
#include "ConquestGameplayTypes.generated.h"

class UConquestBuildingData;
class UConquestTechData;

UENUM(BlueprintType)
enum class EConquestTurnPhase : uint8
{
	None,
	GameSetup,
	AwaitingFirstCity,
	StartTurnProcessing,
	PlayerActions,
	EndTurnProcessing,
	GameOver
};

UENUM(BlueprintType)
enum class EConquestTurnMode : uint8
{
	Simultaneous UMETA(DisplayName = "Simultaneous"),
	Sequential UMETA(DisplayName = "Sequential")
};

UENUM(BlueprintType)
enum class ECityProductionType : uint8
{
	None,
	Building,
	Unit,
	Project
};

UENUM(BlueprintType)
enum class EConquestYieldType : uint8
{
	Food,
	Production,
	Gold,
	Science,
	Culture,
	Faith,
	Happiness
};

USTRUCT(BlueprintType)
struct FConquestYieldSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Food = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Production = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Science = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Culture = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Faith = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Happiness = 0;

	void Reset()
	{
		Food = 0;
		Production = 0;
		Gold = 0;
		Science = 0;
		Culture = 0;
		Faith = 0;
		Happiness = 0;
	}

	FConquestYieldSet& operator+=(const FConquestYieldSet& Other)
	{
		Food += Other.Food;
		Production += Other.Production;
		Gold += Other.Gold;
		Science += Other.Science;
		Culture += Other.Culture;
		Faith += Other.Faith;
		Happiness += Other.Happiness;
		return *this;
	}

	FConquestYieldSet operator+(const FConquestYieldSet& Other) const
	{
		FConquestYieldSet Result = *this;
		Result += Other;
		return Result;
	}
};

USTRUCT(BlueprintType)
struct FCityProductionItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECityProductionType Type = ECityProductionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ProductionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Progress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cost = 0.0f;

	bool IsValid() const
	{
		return Type != ECityProductionType::None && !ProductionId.IsNone() && Cost > 0.0f;
	}

	void Clear()
	{
		Type = ECityProductionType::None;
		ProductionId = NAME_None;
		Progress = 0.0f;
		Cost = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FCityProductionProgressCacheEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECityProductionType Type = ECityProductionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ProductionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Progress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cost = 0.0f;

	bool Matches(ECityProductionType InType, FName InProductionId) const
	{
		return Type == InType && ProductionId == InProductionId;
	}

	bool IsValid() const
	{
		return Type != ECityProductionType::None && !ProductionId.IsNone() && Cost > 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FCityWorkedTileAssignment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "3"))
	int32 Citizens = 1;
};

USTRUCT(BlueprintType)
struct FCityOwnedTileCombatState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CombatStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 HealRatePerTurn = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefenderModifier = 1.0f;
};

USTRUCT(BlueprintType)
struct FCityState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CityId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OwnerPlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CityName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint CenterTile = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Population = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedStrength = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FoodStored = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedFoodRequiredForNextPopulation = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntPoint> OwnedTiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntPoint> WorkedTiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCityWorkedTileAssignment> WorkedTileAssignments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCityOwnedTileCombatState> OwnedTileCombatStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> ConstructedBuildingIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCityProductionItem CurrentProduction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCityProductionProgressCacheEntry> ProductionProgressCache;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexYield CachedYieldPerTurn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNeedsProductionChoice = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PendingBorderExpansions = 0;
};

USTRUCT(BlueprintType)
struct FHexTileGameplayState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OwnerPlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OwningCityId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsWorked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WorkedByCityId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexYield BaseYields;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexYield CachedFinalYields;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CombatStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 HealRatePerTurn = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefenderModifier = 1.0f;
};
