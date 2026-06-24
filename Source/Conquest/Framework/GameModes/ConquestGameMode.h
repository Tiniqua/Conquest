#pragma once

#include "CoreMinimal.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "GameFramework/GameModeBase.h"
#include "ConquestGameMode.generated.h"

class UConquestCivilisationData;

UCLASS()
class CONQUEST_API AConquestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AConquestGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conquest|Game Setup")
	TArray<TObjectPtr<UConquestCivilisationData>> AvailableCivilisations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Multiplayer")
	TArray<int32> ReadyPlayerIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Multiplayer")
	TArray<int32> ConnectedHumanPlayerIds;

	UFUNCTION(BlueprintPure, Category = "Conquest|Game Setup")
	TArray<UConquestCivilisationData*> GetAvailableCivilisations() const;

	UFUNCTION(BlueprintCallable)
	void StartSinglePlayerGame();

	UFUNCTION(BlueprintCallable)
	void EndCurrentTurn();

	UFUNCTION(BlueprintCallable, Category="Conquest|Turn")
	bool EndTurnForPlayer(int32 PlayerId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Research")
	bool SetCurrentResearchForPlayer(int32 PlayerId, FName TechId);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	bool SetCityProductionForPlayer(int32 PlayerId, int32 CityId, ECityProductionType ProductionType, FName ProductionId);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	bool ClaimExpansionTileForPlayer(int32 PlayerId, int32 CityId, FIntPoint Coord);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	bool PurchaseTileImprovementForPlayer(int32 PlayerId, FIntPoint Coord, FName ImprovementId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	bool MoveUnitForPlayer(int32 PlayerId, int32 UnitInstanceId, FIntPoint TargetCoord);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	bool ApplyUnitActionForPlayer(int32 PlayerId, int32 UnitInstanceId, FName ActionId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	bool ApplyUnitAugmentForPlayer(int32 PlayerId, int32 UnitInstanceId, FName AugmentId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	bool AttackUnitForPlayer(int32 PlayerId, int32 AttackerUnitInstanceId, int32 DefenderUnitInstanceId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	bool AttackCityForPlayer(int32 PlayerId, int32 AttackerUnitInstanceId, int32 DefenderCityId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Game")
	void ResetGameToMainMenu();

	UFUNCTION(BlueprintCallable, Category="Conquest|Game Setup")
	bool RegenerateFirstTurnMapForPlayer(int32 PlayerId);

	UFUNCTION(BlueprintCallable)
	bool CanEndCurrentTurn(UPARAM(ref) FText& OutBlockReason) const;

	UFUNCTION(BlueprintCallable)
	bool FoundStartingCity(const FIntPoint& TileCoord, FName CityName);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	bool FoundStartingCityForPlayer(int32 PlayerId, const FIntPoint& TileCoord, FName CityName);

	bool SetLobbySlotCivilisationForPlayer(int32 PlayerId, int32 SlotIndex, UConquestCivilisationData* Civilisation);
	void SetLobbyReadyForPlayer(int32 PlayerId, bool bReady);
	bool AreAllLobbyHumanPlayersReady() const;

private:
	int32 NextAssignedPlayerId = 0;

	static constexpr int32 StartingRegionRadius = 5;

	void SyncAvailableCivilisationsToGameState();
	void AssignPlayerToLobbySlot(int32 PlayerId, const FString& PlayerName);
	void InitializeEmpiresFromLobby();
	void AssignPlayerStartRegions();
	bool IsValidStartRegionCenter(const AConquestGameState& ConquestGS, const FIntPoint& Coord) const;
	float ScoreStartRegionCandidate(
		const AConquestGameState& ConquestGS,
		const FIntPoint& Coord,
		const TArray<FConquestPlayerStartRegion>& ExistingRegions,
		const TMap<FIntPoint, int32>& LandRegionSizeByCoord,
		int32 LargestLandRegionSize,
		int32 IdealSpacing,
		int32 MaxPreferredSpacing
	) const;
	bool IsTileInPlayerStartRegion(const AConquestGameState& ConquestGS, int32 PlayerId, const FIntPoint& Coord) const;
	bool HaveAllHumanPlayersFoundedStartingCities(const AConquestGameState& ConquestGS) const;
	bool AreAllHumanPlayersReady(const AConquestGameState& ConquestGS) const;
	TArray<int32> GetHumanPlayerIds(const AConquestGameState& ConquestGS) const;
	bool DoesPlayerOwnCity(const AConquestGameState& ConquestGS, int32 PlayerId, int32 CityId) const;
	bool CanPlayerEndTurn(const AConquestGameState& ConquestGS, int32 PlayerId, FText& OutBlockReason) const;
	bool CheckCityOwnershipVictory(AConquestGameState& ConquestGS);
	int32 GetParticipatingPlayerCount(const AConquestGameState& ConquestGS) const;
};
