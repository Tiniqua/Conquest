#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Conquest/Core/ConquestModifierTypes.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "Conquest/World/Generation/HexYieldTypes.h"
#include "ConquestModifierManager.generated.h"

class AConquestGameState;
struct FCityState;
struct FConquestUnitState;

UCLASS(BlueprintType)
class CONQUEST_API UConquestModifierManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AConquestGameState* InGameState);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Modifiers")
	float CalculateModifiedValue(
		const FConquestModifierContext& Context,
		EConquestModifierAttribute Attribute,
		float BaseValue
	) const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Modifiers")
	float ApplyFinalRounding(EConquestModifierAttribute Attribute, float Value) const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Modifiers")
	int32 CalculateModifiedIntValue(
		const FConquestModifierContext& Context,
		EConquestModifierAttribute Attribute,
		float BaseValue
	) const;

	FHexYield ApplyYieldModifiers(
		const FConquestModifierContext& Context,
		const FHexYield& BaseYield
	) const;

	int32 CalculateProductionCost(
		const FCityState& City,
		ECityProductionType ProductionType,
		FName ProductionId,
		int32 BaseCost
	) const;

	int32 CalculateMovementCost(
		const FConquestUnitState& Unit,
		const FIntPoint& FromCoord,
		const FIntPoint& ToCoord,
		int32 BaseCost
	) const;

	int32 CalculatePhilosophyCost(int32 PlayerId, float BaseCost) const;

	void BuildUnitCombatModifiers(
		const FConquestUnitState& Unit,
		TArray<FConquestUnitCombatModifier>& OutCombatModifiers
	) const;

	void BuildExternalUnitCombatModifiers(
		const FConquestUnitState& TargetUnit,
		int32 SourcePlayerId,
		TArray<FConquestUnitCombatModifier>& OutCombatModifiers
	) const;

	float CalculateUnitCombatValue(
		const FConquestUnitState& Unit,
		EConquestUnitCombatModifierType ModifierType,
		int32 ExternalSourcePlayerId = INDEX_NONE
	) const;

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;
};
