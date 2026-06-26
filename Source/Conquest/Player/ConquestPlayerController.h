#pragma once

#include "CoreMinimal.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "GameFramework/PlayerController.h"
#include "ConquestPlayerController.generated.h"

class FLifetimeProperty;
class UConquestCivilisationData;

UCLASS()
class CONQUEST_API AConquestPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AConquestPlayerController();

	UFUNCTION(BlueprintPure, Category="Conquest|Multiplayer")
	int32 GetAssignedPlayerId() const { return AssignedPlayerId; }

	UFUNCTION(BlueprintCallable, Category="Conquest|Multiplayer")
	void SetAssignedPlayerId(int32 NewPlayerId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Turn")
	void RequestEndTurn();

	UFUNCTION(BlueprintCallable, Category="Conquest|Game")
	void RequestReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category="Conquest|Game Setup")
	void RequestRegenerateFirstTurnMap();

	UFUNCTION(BlueprintCallable, Category="Conquest|Research")
	void RequestSetCurrentResearch(FName TechId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Philosophy")
	void RequestAdoptPhilosophy(FName PhilosophyId);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	void RequestSetCityProduction(int32 CityId, ECityProductionType ProductionType, FName ProductionId);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	void RequestClaimExpansionTile(int32 CityId, FIntPoint Coord);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	void RequestPurchaseTileImprovement(FIntPoint Coord, FName ImprovementId);

	UFUNCTION(BlueprintCallable, Category="Conquest|City")
	void RequestFoundStartingCity(FIntPoint Coord, FName CityName);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	void RequestMoveUnit(int32 UnitInstanceId, FIntPoint TargetCoord);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	void RequestUnitAction(int32 UnitInstanceId, FName ActionId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	void RequestApplyUnitAugment(int32 UnitInstanceId, FName AugmentId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	void RequestAttackUnit(int32 AttackerUnitInstanceId, int32 DefenderUnitInstanceId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	void RequestAttackCity(int32 AttackerUnitInstanceId, int32 DefenderCityId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Unit")
	void RequestAttackTile(int32 AttackerUnitInstanceId, FIntPoint TargetCoord);

	UFUNCTION(BlueprintCallable, Category="Conquest|Lobby")
	void RequestSetLobbySlotCivilisation(int32 SlotIndex, UConquestCivilisationData* Civilisation);

	UFUNCTION(BlueprintCallable, Category="Conquest|Lobby")
	void RequestSetLobbyReady(bool bReady);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Conquest|Multiplayer")
	int32 AssignedPlayerId = 0;

	UFUNCTION(Server, Reliable)
	void ServerRequestEndTurn();

	UFUNCTION(Server, Reliable)
	void ServerRequestReturnToMainMenu();

	UFUNCTION(Server, Reliable)
	void ServerRequestRegenerateFirstTurnMap();

	UFUNCTION(Server, Reliable)
	void ServerRequestSetCurrentResearch(FName TechId);

	UFUNCTION(Server, Reliable)
	void ServerRequestAdoptPhilosophy(FName PhilosophyId);

	UFUNCTION(Server, Reliable)
	void ServerRequestSetCityProduction(int32 CityId, ECityProductionType ProductionType, FName ProductionId);

	UFUNCTION(Server, Reliable)
	void ServerRequestClaimExpansionTile(int32 CityId, FIntPoint Coord);

	UFUNCTION(Server, Reliable)
	void ServerRequestPurchaseTileImprovement(FIntPoint Coord, FName ImprovementId);

	UFUNCTION(Server, Reliable)
	void ServerRequestFoundStartingCity(FIntPoint Coord, FName CityName);

	UFUNCTION(Server, Reliable)
	void ServerRequestMoveUnit(int32 UnitInstanceId, FIntPoint TargetCoord);

	UFUNCTION(Server, Reliable)
	void ServerRequestUnitAction(int32 UnitInstanceId, FName ActionId);

	UFUNCTION(Server, Reliable)
	void ServerRequestApplyUnitAugment(int32 UnitInstanceId, FName AugmentId);

	UFUNCTION(Server, Reliable)
	void ServerRequestAttackUnit(int32 AttackerUnitInstanceId, int32 DefenderUnitInstanceId);

	UFUNCTION(Server, Reliable)
	void ServerRequestAttackCity(int32 AttackerUnitInstanceId, int32 DefenderCityId);

	UFUNCTION(Server, Reliable)
	void ServerRequestAttackTile(int32 AttackerUnitInstanceId, FIntPoint TargetCoord);

	UFUNCTION(Server, Reliable)
	void ServerRequestSetLobbySlotCivilisation(int32 SlotIndex, UConquestCivilisationData* Civilisation);

	UFUNCTION(Server, Reliable)
	void ServerRequestSetLobbyReady(bool bReady);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
