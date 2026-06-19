#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "ConquestCityManager.generated.h"

class AConquestGameState;
struct FConquestUnitState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityChanged, int32, CityId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCityFounded, int32, CityId, FIntPoint, Coord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityNeedsBorderExpansion, int32, CityId);

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

	UPROPERTY(BlueprintAssignable)
	FOnCityNeedsBorderExpansion OnCityNeedsBorderExpansion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FCityState> Cities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Cities")
	FName DefaultCityCentreBuildingId = FName(TEXT("CityCentre"));

	UFUNCTION(BlueprintCallable)
	bool FoundCity(int32 PlayerId, const FIntPoint& TileCoord, FName CityName);

	UFUNCTION(BlueprintCallable)
	int32 FindCityAtTile(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintCallable)
	void ProcessCitiesAtStartOfTurn(int32 PlayerId);

	UFUNCTION(BlueprintCallable)
	void ProcessUnitsAtStartOfTurn(int32 PlayerId);

	UFUNCTION(BlueprintCallable)
	bool SetCityProductionBuildingById(int32 CityId, FName BuildingId);

	UFUNCTION(BlueprintCallable)
	bool SetCityProductionUnitById(int32 CityId, FName UnitId);

	UFUNCTION(BlueprintCallable)
	bool SetCityProductionProjectById(int32 CityId, FName ProjectId);

	UFUNCTION(BlueprintCallable)
	TArray<FName> GetAvailableProductionBuildingIdsForCity(int32 CityId) const;

	UFUNCTION(BlueprintCallable)
	TArray<FName> GetAvailableProductionUnitIdsForCity(int32 CityId) const;

	UFUNCTION(BlueprintCallable)
	TArray<FName> GetAvailableProductionProjectIdsForCity(int32 CityId) const;

	UFUNCTION(BlueprintCallable)
	TArray<FIntPoint> GetExpansionCandidateTiles(int32 CityId) const;

	UFUNCTION(BlueprintCallable)
	bool ClaimExpansionTileForCity(int32 CityId, const FIntPoint& Coord);

	UFUNCTION(BlueprintCallable)
	TArray<FName> GetAvailableTileImprovementIdsForPlayer(int32 PlayerId, const FIntPoint& Coord) const;

	UFUNCTION(BlueprintCallable)
	bool PurchaseTileImprovementForPlayer(int32 PlayerId, const FIntPoint& Coord, FName ImprovementId);

	UFUNCTION(BlueprintCallable)
	bool RefreshCityYields(int32 CityId);

	UFUNCTION(BlueprintPure)
	int32 GetFoodRequiredForNextPopulation(const FCityState& City) const;
	
	UFUNCTION(BlueprintCallable)
	int32 EstimateTurnsToBuildById(int32 CityId, FName BuildingId) const;

	UFUNCTION(BlueprintCallable)
	int32 EstimateTurnsToTrainUnitById(int32 CityId, FName UnitId) const;

	UFUNCTION(BlueprintCallable)
	int32 EstimateTurnsToGrowth(int32 CityId) const;

	UFUNCTION(BlueprintCallable)
	FCityState GetCityCopy(int32 CityId) const;

	FCityState* GetCityMutable(int32 CityId);
	const FCityState* GetCity(int32 CityId) const;
	bool CityHasBuildingOrReplacement(const FCityState& City, FName BaseBuildingId) const;

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;

	int32 NextCityId = 1;

	static constexpr int32 MaxCityExpansionRange = 5;

	bool IsValidFoundCityTile(const FIntPoint& TileCoord) const;
	void GrantStartingBuildings(FCityState& City);
	bool ClaimTileForCity(FCityState& City, const FIntPoint& Coord);
	bool IsValidExpansionTileForCity(const FCityState& City, const FIntPoint& Coord) const;
	bool IsValidPopulationAssignmentTile(const FCityState& City, const FIntPoint& Coord) const;
	void AutoAssignWorkedTiles(FCityState& City);
	void SyncWorkedTilesFromAssignments(FCityState& City);
	int32 GetAssignedCitizensForTile(const FCityState& City, const FIntPoint& Coord) const;
	bool AssignCitizenToTile(FCityState& City, const FIntPoint& Coord);
	void RecalculateCityYields(FCityState& City);
	FHexYield GetProductionProjectYieldBonus(const FCityState& City) const;
	void RecalculateEmpireYields(int32 PlayerId);
	void RecalculateStrategicResourceEconomy(int32 PlayerId);
	void AccumulateStrategicResourceIncome(int32 PlayerId);
	void RecalculateUnitStats(FConquestUnitState& Unit) const;
	int32 CreateUnitFromProduction(const FCityState& City, FName UnitId);
	void SpawnUnitActorForState(const FConquestUnitState& UnitState);
	void UpdateOwnedTileVisuals(int32 PlayerId);
	void UpdateCityWorldLabel(const FCityState& City);
	FName ResolveCityName(int32 PlayerId, FName RequestedCityName) const;
	void ProcessCityGrowth(FCityState& City);
	void ProcessCityProduction(FCityState& City);
	float ScoreTileForExpansion(const FCityState& City, const FIntPoint& Coord) const;
};
