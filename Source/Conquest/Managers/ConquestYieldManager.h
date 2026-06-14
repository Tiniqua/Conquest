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
	FHexYield CalculateCityTotalYields(const FCityState& City) const;

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;
};