#pragma once

#include "CoreMinimal.h"
#include "Conquest/World/Generation/HexYieldTypes.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "ConquestPlayerEmpireState.generated.h"

USTRUCT(BlueprintType)
struct FConquestStrategicResourceStockpile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ResourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Stored = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Cap = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PerTurn = 0;
};

USTRUCT(BlueprintType)
struct FConquestUnitAugmentState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AugmentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConquestUnitAugmentStat ModifiedStat = EConquestUnitAugmentStat::Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FlatBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MultiplierBonus = 0.0f;
};

USTRUCT(BlueprintType)
struct FConquestUnitState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UnitInstanceId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OwnerPlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName UnitId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SourceCityId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint TileCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentHealth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentMovementPoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsFortified = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSleeping = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestUnitAugmentState> Augments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestUnitCombatModifier> CombatModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedAttackRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedMaxHealth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedHealthRegenPerTurn = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedMovementPoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedGoldMaintenancePerTurn = 0;
};

USTRUCT(BlueprintType)
struct FConquestPlayerEmpireState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> CityIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexYield StoredYields;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexYield CachedYieldPerTurn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestStrategicResourceStockpile> StrategicResources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestUnitState> Units;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NextUnitInstanceId = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BaseHappiness = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedHappiness = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedLuxuryHappiness = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedBuildingHappiness = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedCityHappinessCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CachedPopulationHappinessCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> ResearchedTechIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CurrentResearchId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentResearchProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AdoptedPhilosophyTenets = 0;

	bool HasResearched(FName TechId) const
	{
		return !TechId.IsNone() && ResearchedTechIds.Contains(TechId);
	}

	FConquestStrategicResourceStockpile* FindStrategicResource(FName ResourceId)
	{
		return StrategicResources.FindByPredicate([ResourceId](const FConquestStrategicResourceStockpile& Stockpile)
		{
			return Stockpile.ResourceId == ResourceId;
		});
	}

	const FConquestStrategicResourceStockpile* FindStrategicResource(FName ResourceId) const
	{
		return StrategicResources.FindByPredicate([ResourceId](const FConquestStrategicResourceStockpile& Stockpile)
		{
			return Stockpile.ResourceId == ResourceId;
		});
	}
};

namespace ConquestUnitCombat
{
	CONQUEST_API float GetHealthCombatMultiplier(const FConquestUnitState& Unit);
	CONQUEST_API float GetCombatValue(const FConquestUnitState& Unit, EConquestUnitCombatModifierType ModifierType);
	CONQUEST_API int32 CalculateDeterministicDamage(float AttackerValue, float DefenderValue, float EqualStrengthDamage);
	CONQUEST_API FConquestCombatPreviewData CalculatePreview(
		const FConquestUnitState& Attacker,
		const FConquestUnitState& Defender,
		int32 AttackDistance
	);
}

namespace ConquestHappiness
{
	CONQUEST_API int32 GetCityCost();
	CONQUEST_API int32 GetPopulationCost();
	CONQUEST_API int32 GetSevereUnhappyThreshold();
	CONQUEST_API bool IsUnhappy(int32 Happiness);
	CONQUEST_API bool IsSeverelyUnhappy(int32 Happiness);
	CONQUEST_API float GetPenaltyMultiplier(int32 Happiness);
	CONQUEST_API FText GetPenaltyText(int32 Happiness);
}
