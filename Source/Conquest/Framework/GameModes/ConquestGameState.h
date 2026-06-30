#pragma once

#include "CoreMinimal.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Units/ConquestUnitActor.h"
#include "GameFramework/GameStateBase.h"
#include "Conquest/Core/ConquestModifierTypes.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/World/Generation/ModularHexGridActor.h"
#include "Conquest/World/Generation/ConquestGameSetupTypes.h"
#include "ConquestGameState.generated.h"

class UConquestContentManager;
class UConquestTurnManager;
class UConquestCityManager;
class UConquestYieldManager;
class UConquestTechManager;
class UConquestModifierManager;
class UConquestPhilosophyManager;
class UConquestBuildingData;
class UConquestTechData;
class FHexGridModel;
class FLifetimeProperty;
struct FConquestGameSetupSettings;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConquestStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnConquestUnitMoved, int32, UnitInstanceId, int32, PlayerId, FIntPoint, FromCoord, FIntPoint, ToCoord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnConquestUnitAction, int32, UnitInstanceId, int32, PlayerId, FName, ActionId);

enum class EConquestStateVisualDirtyFlags : uint8
{
	None = 0,
	Cities = 1,
	Units = 2,
	TileImprovements = 4,
	All = 7
};

ENUM_CLASS_FLAGS(EConquestStateVisualDirtyFlags);

UENUM(BlueprintType)
enum class EConquestEndTurnBlockType : uint8
{
	None,
	GameNotReady,
	WrongPhase,
	Research,
	Philosophy,
	CityProduction,
	UnitOrders,
	CityGrowth
};

USTRUCT(BlueprintType)
struct FConquestEndTurnBlocker
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConquestEndTurnBlockType Type = EConquestEndTurnBlockType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CityId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UnitInstanceId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FConquestPlayerStartRegion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Center = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RegionRadius = 5;
};

USTRUCT(BlueprintType)
struct FConquestReplicatedTileImprovement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ImprovementId = NAME_None;
};

USTRUCT(BlueprintType)
struct FConquestReplicatedGameState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentTurn = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConquestTurnPhase CurrentPhase = EConquestTurnPhase::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConquestTurnMode TurnMode = EConquestTurnMode::Simultaneous;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestPlayerEmpireState> PlayerEmpires;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCityState> Cities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestLobbyPlayerSlot> LobbyPlayerSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FConquestGameSetupSettings GameSetupSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> ReadyPlayerIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UConquestCivilisationData>> AvailableCivilisations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestPlayerStartRegion> PlayerStartRegions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FConquestReplicatedTileImprovement> TileImprovements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bGameEnded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WinningPlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 VisualDirtyMask = 0x07;
};

UCLASS()
class CONQUEST_API AConquestGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AConquestGameState();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable)
	FOnConquestStateChanged OnConquestStateChanged;

	UPROPERTY(BlueprintAssignable)
	FOnConquestUnitMoved OnConquestUnitMoved;

	UPROPERTY(BlueprintAssignable)
	FOnConquestUnitAction OnConquestUnitAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestTurnManager> TurnManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestCityManager> CityManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestYieldManager> YieldManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestTechManager> TechManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestModifierManager> ModifierManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestPhilosophyManager> PhilosophyManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestContentManager> ContentManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> BuildingTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> UnitTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> UnitAugmentTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> TechTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> PhilosophyTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Philosophy", meta=(ClampMin="0"))
	int32 BasePhilosophyCultureCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Philosophy", meta=(ClampMin="0.0"))
	float PhilosophyCostIncreasePercentPerAdoption = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Modifiers")
	EConquestModifierRoundingMode DefaultModifierRoundingMode = EConquestModifierRoundingMode::Nearest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Modifiers", meta=(ClampMin="0.0001"))
	float DefaultModifierRoundingIncrement = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Modifiers")
	TArray<FConquestModifierRoundingRule> ModifierRoundingRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Players")
	TObjectPtr<UConquestCivilisationData> HumanCivilisation = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Players")
	TMap<int32, TObjectPtr<UConquestCivilisationData>> PlayerCivilisations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Players")
	TArray<FConquestLobbyPlayerSlot> LobbyPlayerSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Game Setup")
	FConquestGameSetupSettings GameSetupSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Players")
	TArray<TObjectPtr<UConquestCivilisationData>> AvailableCivilisations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Players")
	FConquestPlayerEmpireState HumanPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Players")
	TArray<FConquestPlayerEmpireState> PlayerEmpires;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Players")
	TArray<FConquestPlayerStartRegion> PlayerStartRegions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Game")
	bool bGameEnded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conquest|Game")
	int32 WinningPlayerId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedConquestState, BlueprintReadOnly, Category="Conquest|Replication")
	FConquestReplicatedGameState ReplicatedConquestState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Units")
	TSubclassOf<AConquestUnitActor> UnitActorClass;

	UPROPERTY(BlueprintReadOnly, Category="Conquest|Units")
	TMap<int32, TObjectPtr<AConquestUnitActor>> UnitActorsByInstanceId;

	UPROPERTY(BlueprintReadOnly, Category="Conquest|Units")
	int32 SelectedUnitInstanceId = INDEX_NONE;

	UFUNCTION(BlueprintCallable)
	void BroadcastStateChanged();

	void BroadcastStateChangedWithVisuals(EConquestStateVisualDirtyFlags VisualDirtyFlags);
	void BroadcastStateChangedWithoutReplication();
	void BeginStateChangeBatch();
	void EndStateChangeBatch(EConquestStateVisualDirtyFlags FallbackVisualDirtyFlags = EConquestStateVisualDirtyFlags::All);
	void MarkVisualsDirty(EConquestStateVisualDirtyFlags VisualDirtyFlags);

	UFUNCTION(BlueprintCallable, Category="Conquest|Replication")
	void PushReplicatedState();

	void ApplyReplicatedTileImprovements();

	UFUNCTION()
	void OnRep_ReplicatedConquestState();

	UFUNCTION(BlueprintCallable)
	FConquestPlayerEmpireState& GetHumanPlayerMutable();

	UFUNCTION(BlueprintCallable)
	const FConquestPlayerEmpireState& GetHumanPlayer() const;

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	FConquestPlayerEmpireState& GetPlayerEmpireMutable(int32 PlayerId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	const FConquestPlayerEmpireState& GetPlayerEmpire(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category="Conquest|Players")
	int32 GetLocalPlayerId() const;

	UFUNCTION(BlueprintPure, Category="Conquest|Players")
	bool GetStartRegionForPlayer(int32 PlayerId, FConquestPlayerStartRegion& OutStartRegion) const;

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	void EnsurePlayerEmpire(int32 PlayerId);

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	void ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings);

	UFUNCTION(BlueprintCallable, Category="Conquest|Game Setup")
	void SetGameSetupSettings(const FConquestGameSetupSettings& SetupSettings);

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	void SetAvailableCivilisations(const TArray<UConquestCivilisationData*>& InAvailableCivilisations);

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	void SetLobbyPlayerSlots(const TArray<FConquestLobbyPlayerSlot>& InLobbyPlayerSlots);

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	bool SetLobbySlotCivilisationForPlayer(int32 RequestingPlayerId, int32 SlotIndex, UConquestCivilisationData* Civilisation);

	UFUNCTION(BlueprintCallable, Category="Conquest|Players")
	void SetLobbyPlayerReady(int32 PlayerId, bool bReady);

	UFUNCTION(BlueprintPure, Category="Conquest|Players")
	int32 GetReadyHumanPlayerCount() const;

	UFUNCTION(BlueprintPure, Category="Conquest|Players")
	int32 GetRequiredReadyHumanPlayerCount() const;

	UFUNCTION(BlueprintPure, Category="Conquest|Turn")
	bool IsPlayerWaitingForOtherPlayers(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category="Conquest|Players")
	UConquestCivilisationData* GetCivilisationForPlayer(int32 PlayerId) const;

	UFUNCTION(BlueprintCallable, Category="Conquest|Turn")
	bool GetEndTurnBlockerForPlayer(int32 PlayerId, FConquestEndTurnBlocker& OutBlocker) const;

	UFUNCTION(BlueprintCallable, Category="Conquest|Turn")
	bool CanPlayerEndTurnFromState(int32 PlayerId, UPARAM(ref) FText& OutBlockReason) const;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotifyUnitMoved(int32 UnitInstanceId, int32 PlayerId, FIntPoint FromCoord, FIntPoint ToCoord, int32 CurrentMovementPoints);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotifyUnitAction(int32 UnitInstanceId, int32 PlayerId, FName ActionId);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReturnToMainMenu();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSyncCityVisualState(const FCityState& CityState, bool bRebuildBorders, bool bRebuildProceduralPlaceholders);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSyncTileHealthBar(int32 CityId, FCityOwnedTileCombatState TileCombatState);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRemoveTileHealthBar(FIntPoint Coord);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSyncTileImprovement(FIntPoint Coord, FName ImprovementId);
	
	UPROPERTY(ReplicatedUsing=OnRep_ActiveGridActor, BlueprintReadWrite, Category="Conquest|Map")
	TObjectPtr<AModularHexGridActor> ActiveGridActor;

	UFUNCTION()
	void OnRep_ActiveGridActor();

	void RebuildCityVisualsFromReplicatedState();
	void RebuildUnitVisualsFromReplicatedState();
	void ClearUnitVisuals();
	void ApplyCityVisualState(const FCityState& CityState, bool bRebuildBorders, bool bRebuildProceduralPlaceholders);
	void RebuildCivilisationBordersFromLocalState();
	void ApplyTileHealthBarVisual(int32 CityId, const FCityOwnedTileCombatState& TileCombatState);

	FHexGridModel* GetHexGridModelMutable();
	const FHexGridModel* GetHexGridModel() const;

private:
	int32 StateChangeBatchDepth = 0;
	bool bStateChangeBatchPending = false;
	bool bHasPendingVisualDirtyFlags = false;
	bool bHasAppliedReplicatedConquestState = false;
	EConquestStateVisualDirtyFlags PendingVisualDirtyFlags = EConquestStateVisualDirtyFlags::None;
	EConquestStateVisualDirtyFlags VisualDirtyFlagsForNextPush = EConquestStateVisualDirtyFlags::All;

	void FlushStateChanged(EConquestStateVisualDirtyFlags FallbackVisualDirtyFlags);
	void RebuildDirtyVisuals(EConquestStateVisualDirtyFlags VisualDirtyFlags);
	void ApplyReplicatedUnitMoveToLocalState(int32 UnitInstanceId, int32 PlayerId, FIntPoint ToCoord, int32 CurrentMovementPoints);
};
