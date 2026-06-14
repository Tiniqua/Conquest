#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "ConquestCityManager.generated.h"

class AConquestGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityChanged, int32, CityId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityFounded, int32, CityId, FIntPoint, Coord);

UCLASS(BlueprintType)
class CONQUEST_API UConquestCityManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AConquestGameState* InGameState);

	UPROPERTY(BlueprintAssignable)
	FOnCityChanged OnCityChanged;

	UPROPERTY(BlueprintAssignable)
	FOnCityFounded OnCityFounded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FCityState> Cities;

	UFUNCTION(BlueprintCallable)
	bool FoundCity(int32 PlayerId, const FIntPoint& TileCoord, FName CityName);

	UFUNCTION(BlueprintCallable)
	int32 FindCityAtTile(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintCallable)
	void ProcessCitiesAtStartOfTurn(int32 PlayerId);

	UFUNCTION(BlueprintCallable)
	bool SetCityProductionBuildingById(int32 CityId, FName BuildingId);

	UFUNCTION(BlueprintCallable)
	TArray<FName> GetAvailableProductionBuildingIdsForCity(int32 CityId) const;
	
	UFUNCTION(BlueprintCallable)
	int32 EstimateTurnsToBuildById(int32 CityId, FName BuildingId) const;

	UFUNCTION(BlueprintCallable)
	FCityState GetCityCopy(int32 CityId) const;

	FCityState* GetCityMutable(int32 CityId);
	const FCityState* GetCity(int32 CityId) const;
	bool CityHasBuildingOrReplacement(const FCityState& City, FName BaseBuildingId) const;

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;

	int32 NextCityId = 1;

	bool IsValidFoundCityTile(const FIntPoint& TileCoord) const;
	void ClaimInitialTiles(FCityState& City);
	void ClaimTileForCity(FCityState& City, const FIntPoint& Coord);
	void AutoAssignWorkedTiles(FCityState& City);
	void RecalculateCityYields(FCityState& City);
	void ProcessCityGrowth(FCityState& City);
	void ProcessCityProduction(FCityState& City);
	void ExpandCityBorders(FCityState& City, int32 NumTiles);
	FIntPoint ChooseBestExpansionTile(const FCityState& City) const;
	float ScoreTileForExpansion(const FCityState& City, const FIntPoint& Coord) const;
};