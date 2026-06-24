#pragma once

#include "CoreMinimal.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/World/Generation/ConquestGameSetupTypes.h"
#include "GameFramework/HUD.h"
#include "Conquest/World/Generation/HexMapTypePresets.h"
#include "ConquestHUD.generated.h"

class UConquestResearchPanelWidget;
class UConquestCityPanelWidget;
class UConquestHUDWidget;
class UConquestMainMenuWidget;
class UConquestGameSetupWidget;
class UConquestGameWidget;
class AModularHexGridActor;
struct FConquestUnitState;

UCLASS()
class CONQUEST_API AConquestHUD : public AHUD
{
	GENERATED_BODY()

public:
	AConquestHUD();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowGameSetup();

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowGame();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void RequestStartGame(const FConquestGameSetupSettings& SetupSettings);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void StartGameWithMapPreset(EHexMapTypePreset MapPreset);

	UFUNCTION(BlueprintPure, Category = "Conquest|HUD")
	UConquestGameWidget* GetGameWidget() const;

	UFUNCTION(BlueprintPure, Category = "Conquest|HUD")
	UConquestGameWidget* GetActiveGameWidget() const;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestCityPanelWidget> CityPanelWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestResearchPanelWidget> ResearchPanelWidgetClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UConquestGameWidget> GameWidget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UConquestCityPanelWidget> CityPanelWidget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UConquestResearchPanelWidget> ResearchPanelWidget;

	UFUNCTION(BlueprintCallable)
	void ShowCityPanel(int32 CityId);

	UFUNCTION(BlueprintCallable)
	void HideCityPanel();

	UFUNCTION(BlueprintCallable)
	void ShowResearchPanel();

	UFUNCTION(BlueprintCallable)
	void HideResearchPanel();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Starting City")
	void BeginStartingRegionSelection();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Starting City")
	bool SelectStartingCandidateTile(int32 Q, int32 R);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Starting City")
	bool ConfirmSelectedStartingCity();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Starting City")
	void CancelSelectedStartingCity();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Expansion")
	void BeginCityTileExpansionSelection(int32 CityId);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Expansion")
	bool SelectExpansionCandidateTile(int32 Q, int32 R);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Expansion")
	bool ConfirmSelectedExpansionTile();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Expansion")
	void CancelSelectedExpansionTile();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Expansion")
	void ClearCityTileExpansionSelection();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Improvement")
	bool ShowTileImprovementChoicesForTile(int32 Q, int32 R);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Improvement")
	bool PurchaseSelectedTileImprovement(FName ImprovementId);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Improvement")
	void ClearTileImprovementChoices();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool SelectUnitAtTile(int32 Q, int32 R);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void ClearUnitSelection();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool EnterSelectedUnitMoveMode();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool TryMoveSelectedUnitToTile(int32 Q, int32 R);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool EnterSelectedUnitAttackMode();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool TryAttackSelectedUnitAtTile(int32 Q, int32 R);

	UFUNCTION(BlueprintPure, Category = "Conquest|Unit")
	bool IsSelectedUnitMovementTile(int32 Q, int32 R) const;

	UFUNCTION(BlueprintPure, Category = "Conquest|Unit")
	bool IsSelectedUnitAttackTile(int32 Q, int32 R) const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Combat Preview")
	bool UpdateSelectedUnitCombatPreviewForTile(int32 Q, int32 R);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Combat Preview")
	void ClearCombatPreview();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool ShowAugmentChoicesForSelectedUnit();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool PurchaseSelectedUnitAugment(FName AugmentId);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void ClearUnitAugmentChoices();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool FortifySelectedUnit();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool DoNothingSelectedUnit();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool SleepSelectedUnit();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool DisbandSelectedUnit();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	bool SettleSelectedUnit();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void ClearUnitMovementHighlights();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void ClearUnitAttackHighlights();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Camera")
	void FocusCameraOnTile(FIntPoint Coord, bool bPreserveCurrentHeight);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Turn")
	void TriggerEndTurnShortcut();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestGameSetupWidget> GameSetupWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestGameWidget> GameWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|World")
	TSubclassOf<AModularHexGridActor> HexGridActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|World")
	FTransform HexGridSpawnTransform = FTransform::Identity;

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> HUDWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AModularHexGridActor> SpawnedHexGridActor = nullptr;

	int32 ExpansionSelectionCityId = INDEX_NONE;
	FIntPoint PendingExpansionTileCoord = FIntPoint(INT32_MIN, INT32_MIN);
	FIntPoint PendingImprovementTileCoord = FIntPoint(INT32_MIN, INT32_MIN);
	int32 HiddenCityWorldLabelId = INDEX_NONE;
	TMap<FIntPoint, int32> CurrentUnitMovementRemainingByTile;
	TSet<FIntPoint> CurrentUnitAttackTiles;
	int32 PendingAugmentUnitInstanceId = INDEX_NONE;
	FIntPoint PendingStartingCityCoord = FIntPoint(INT32_MIN, INT32_MIN);
	bool bLocalFogInitialized = false;
	int32 LocalFogPlayerId = INDEX_NONE;
	bool bStartingCameraFocused = false;
	FIntPoint LastStartingCameraFocusCoord = FIntPoint(INT32_MIN, INT32_MIN);

	void ConfigureMenuInputMode();
	void ConfigureGameInputMode();
	void SetCityWorldLabelHiddenForPanel(int32 CityId);
	void RestoreHiddenCityWorldLabel();
	void RefreshSelectedUnitWidget(const FConquestUnitState& UnitState);
	void ApplyLocalFogOfWar();
	bool GetLocalStartingRegion(FConquestPlayerStartRegion& OutStartRegion) const;
	TArray<FIntPoint> GetValidStartingRegionTiles(const FConquestPlayerStartRegion& StartRegion) const;

	UFUNCTION()
	void HandleConquestStateChangedForHUD();

	UFUNCTION()
	void HandleUnitMovedForHUD(int32 UnitInstanceId, int32 PlayerId, FIntPoint FromCoord, FIntPoint ToCoord);
};
