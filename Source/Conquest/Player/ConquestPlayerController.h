#pragma once

#include "CoreMinimal.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "Conquest/World/Generation/ConquestGameSetupTypes.h"
#include "GameFramework/PlayerController.h"
#include "ConquestPlayerController.generated.h"

class UAudioComponent;
class FLifetimeProperty;
class UConquestCivilisationData;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConquestAssignedPlayerIdChanged, int32, AssignedPlayerId);

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

	UPROPERTY(BlueprintAssignable, Category="Conquest|Multiplayer")
	FOnConquestAssignedPlayerIdChanged OnAssignedPlayerIdChanged;

	UFUNCTION(BlueprintCallable, Category="Conquest|Turn")
	void RequestEndTurn();

	UFUNCTION(BlueprintCallable, Category="Conquest|Game")
	void RequestReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category="Conquest|Game Setup")
	void RequestLeaveGameSetup();

	UFUNCTION(BlueprintCallable, Category="Conquest|Settings")
	void ToggleSettingsMenu();

	UFUNCTION(BlueprintCallable, Category="Conquest|Settings|Audio")
	void SetMusicEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Conquest|Settings|Audio")
	bool IsMusicEnabled() const { return bMusicEnabled; }

	UFUNCTION(BlueprintCallable, Category="Conquest|Settings|Audio")
	void SetMusicVolumeMultiplier(float NewVolumeMultiplier);

	UFUNCTION(BlueprintPure, Category="Conquest|Settings|Audio")
	float GetMusicVolumeMultiplier() const { return MusicVolumeMultiplier; }

	UFUNCTION(BlueprintCallable, Category="Conquest|Audio")
	void InitializeBackgroundMusic(const TArray<USoundBase*>& InBackgroundMusicTracks);

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

	UFUNCTION(BlueprintCallable, Category="Conquest|Game Setup")
	void RequestSetGameSetupSettings(const FConquestGameSetupSettings& SetupSettings);

	UFUNCTION(BlueprintCallable, Category="Conquest|Cheats")
	void RequestCheatImproveAllResources();

	UFUNCTION(BlueprintCallable, Category="Conquest|Cheats")
	void RequestCheatGrantTech(FName TechId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Cheats")
	void RequestCheatGrantPhilosophy(FName PhilosophyId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Cheats")
	void RequestCheatSpawnUnit(FName UnitId, FIntPoint TileCoord);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(ReplicatedUsing=OnRep_AssignedPlayerId, BlueprintReadOnly, Category="Conquest|Multiplayer")
	int32 AssignedPlayerId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Audio")
	TObjectPtr<UAudioComponent> BackgroundMusicAudioComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> BackgroundMusicTracks;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentBackgroundMusicTrack = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Conquest|Settings|Audio")
	bool bMusicEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category="Conquest|Settings|Audio")
	float MusicVolumeMultiplier = 1.0f;

	UFUNCTION()
	void OnRep_AssignedPlayerId();

	UFUNCTION(Server, Reliable)
	void ServerRequestEndTurn();

	UFUNCTION(Server, Reliable)
	void ServerRequestReturnToMainMenu();

	UFUNCTION(Server, Reliable)
	void ServerRequestLeaveGameSetup();

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

	UFUNCTION(Server, Reliable)
	void ServerRequestSetGameSetupSettings(const FConquestGameSetupSettings& SetupSettings);

	UFUNCTION(Server, Reliable)
	void ServerRequestCheatImproveAllResources();

	UFUNCTION(Server, Reliable)
	void ServerRequestCheatGrantTech(FName TechId);

	UFUNCTION(Server, Reliable)
	void ServerRequestCheatGrantPhilosophy(FName PhilosophyId);

	UFUNCTION(Server, Reliable)
	void ServerRequestCheatSpawnUnit(FName UnitId, FIntPoint TileCoord);

	UFUNCTION(Client, Reliable)
	void ClientInitializeBackgroundMusic(const TArray<USoundBase*>& InBackgroundMusicTracks);

	UFUNCTION()
	void HandleBackgroundMusicFinished();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void LoadUserSettings();
	void SaveUserSettings() const;
	void ApplyMusicSettings();
	void PlayNextBackgroundMusicTrack();
	USoundBase* ChooseNextBackgroundMusicTrack() const;
};
