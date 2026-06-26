#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "Conquest/World/Generation/HexYieldTypes.h"
#include "ConquestYieldManager.generated.h"

class AConquestGameState;
class UConquestBuildingData;
struct FHexTileData;

UCLASS(BlueprintType)
class CONQUEST_API UConquestYieldManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AConquestGameState* InGameState);

	FHexYield CalculateTileYield(const FHexTileData& Tile) const;
	FHexYield CalculateCityBuildingYields(const FCityState& City) const;
	void ApplyCityBuildingYieldsToCenterTile(const FCityState& City) const;
	FHexYield CalculateCityTotalYieldsBeforeUnhappyPenalty(const FCityState& City) const;
	FHexYield CalculateCityTotalYields(const FCityState& City) const;
	FHexYield CalculateEmpireYieldPerTurnBeforeUnhappyPenalty(int32 PlayerId) const;
	FHexYield CalculateEmpireYieldPerTurn(int32 PlayerId) const;
	int32 CalculateEmpireHappiness(int32 PlayerId) const;

	UFUNCTION(BlueprintCallable)
	FHexYield RecalculateEmpireYieldPerTurn(int32 PlayerId) const;

	UFUNCTION(BlueprintCallable)
	int32 RecalculateEmpireHappiness(int32 PlayerId) const;

	UFUNCTION(BlueprintCallable)
	void CollectGlobalYieldIncome(int32 PlayerId) const;

	UFUNCTION(BlueprintPure)
	bool IsEmpireUnhappy(int32 PlayerId) const;

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;

	FHexYield ApplyUnhappyYieldPenalty(const FHexYield& Yield, int32 PlayerId) const;
};
