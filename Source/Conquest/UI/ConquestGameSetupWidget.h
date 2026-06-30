// ConquestGameSetupWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/World/Generation/HexMapTypePresets.h"
#include "Conquest/World/Generation/ConquestGameSetupTypes.h"
#include "ConquestGameSetupWidget.generated.h"

class UButton;
class UComboBoxString;
class USpinBox;
class UCheckBox;
class UTextBlock;
class UConquestHUDWidget;
class UConquestCivilisationData;

UCLASS()
class CONQUEST_API UConquestGameSetupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void InitializeGameSetupWidget(UConquestHUDWidget* InParentHUDWidget);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void RefreshMapPresetOptions();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void RefreshMapSizeOptions();

	UFUNCTION(BlueprintPure, Category = "Conquest|Game Setup")
	FConquestGameSetupSettings GetSelectedGameSetupSettings() const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup|Lobby")
	void RefreshLobbySlots();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup|Lobby")
	void RefreshCivilisationOptions();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup|Lobby")
	bool SetLobbySlotCivilisation(int32 SlotIndex, UConquestCivilisationData* Civilisation);

	UFUNCTION(BlueprintPure, Category = "Conquest|Game Setup|Lobby")
	TArray<FConquestLobbyPlayerSlot> GetLobbyPlayerSlots() const;

	UFUNCTION(BlueprintPure, Category = "Conquest|Game Setup|Lobby")
	TArray<UConquestCivilisationData*> GetAvailableCivilisations() const;

	UFUNCTION(BlueprintPure, Category = "Conquest|Game Setup|Lobby")
	FText GetLobbySlotDisplayText(int32 SlotIndex) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Game Setup|Lobby")
	void OnLobbySlotsChanged(const TArray<FConquestLobbyPlayerSlot>& PlayerSlots);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Game Setup|Lobby")
	void OnCivilisationOptionsChanged();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> MapPresetComboBox = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> MapSizeComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapSizeTooltipText = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UConquestCivilisationData>> AvailableCivilisations;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot0ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot1ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot2ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot3ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot4ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot5ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot6ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot7ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot8ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot9ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot10ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> CivSlot11ComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivInfoText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivInfoNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivInfoLeaderText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivInfoAbilityText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivInfoUniqueUnitsText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivInfoUniqueBuildingsText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> GenerateResourcesCheckBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> BonusResourceDensitySpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> LuxuryResourceDensitySpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> StrategicResourceDensitySpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> BonusResourceCountSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> LuxuryResourceCountSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> StrategicResourceCountSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> ResourceSpacingSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> GenerateRiversCheckBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> RiverDensitySpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> MaxRiverCountSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> MinRiverLengthSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> MaxRiverLengthSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> RiverSpacingSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> LakeFrequencySpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> LakeCountSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> LakeSpacingSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> LakeMinSizeSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> LakeMaxSizeSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> MountainAmountSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> UseTemperatureBiasCheckBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> TemperatureBiasStrengthSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> PolarFalloffPowerSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> TemperatureNoiseStrengthSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> RandomSeedSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RandomSeedButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReadyButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadyStatusText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadyButtonText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> ParentHUDWidget = nullptr;

	UPROPERTY(Transient)
	TMap<FString, EHexMapTypePreset> OptionToPreset;

	UPROPERTY(Transient)
	TMap<FString, EConquestMapSizePreset> OptionToMapSizePreset;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UConquestCivilisationData>> OptionToCivilisation;

	UPROPERTY(Transient)
	TArray<FConquestLobbyPlayerSlot> LobbyPlayerSlots;

	UPROPERTY(Transient)
	TObjectPtr<UConquestCivilisationData> PreviewCivilisation = nullptr;

	UPROPERTY(Transient)
	EHexMapTypePreset SelectedMapPreset = EHexMapTypePreset::Continents;

	UPROPERTY(Transient)
	EConquestMapSizePreset SelectedMapSizePreset = EConquestMapSizePreset::Standard;

	UFUNCTION()
	void HandleMapPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleMapSizeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot0SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot1SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot2SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot3SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot4SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot5SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot6SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot7SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot8SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot9SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot10SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCivSlot11SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandlePlayButtonClicked();

	UFUNCTION()
	void HandleBackButtonClicked();

	UFUNCTION()
	void HandleReadyButtonClicked();

	UFUNCTION()
	void HandleConquestStateChanged();

	UFUNCTION()
	void HandleAssignedPlayerIdChanged(int32 NewPlayerId);

	UFUNCTION()
	void HandleRandomSeedButtonClicked();

	UFUNCTION()
	void HandleRandomSeedValueChanged(float NewValue);

	UFUNCTION()
	void HandleGameSetupFloatValueChanged(float NewValue);

	UFUNCTION()
	void HandleGameSetupCheckStateChanged(bool bIsChecked);

	int32 GenerateRandomSeed() const;

private:
	void ApplyDefaultAdvancedValues();
	void ApplyGameSetupSettingsToControls(const FConquestGameSetupSettings& SetupSettings);
	void RefreshHostOnlyControlState();
	void PushGameSetupSettingsToServerIfHost();
	bool IsLocalPlayerLobbyHost() const;
	void SetSelectedRandomSeed(int32 NewSeed, bool bUpdateSpinBox);
	void ConfigureRandomSeedSpinBox();
	void UpdateMapSizeTooltip();
	void BindCivilisationComboBoxes();
	void RefreshCivilisationComboBox(UComboBoxString* ComboBox, int32 SlotIndex);
	void HandleCivilisationSelectionChanged(int32 SlotIndex, const FString& SelectedItem);
	bool CanLocalPlayerEditSlot(int32 SlotIndex) const;
	void PushLobbySlotsToServerIfAuthority();
	void RefreshReadyStatus();
	TArray<UComboBoxString*> GetCivilisationComboBoxes() const;
	FString GetCivilisationOptionName(const UConquestCivilisationData* Civilisation, int32 OptionIndex) const;
	void RefreshCivilisationInfo(const UConquestCivilisationData* Civilisation);
	const UConquestCivilisationData* GetDefaultPreviewCivilisation() const;

	UPROPERTY(Transient)
	int32 SelectedRandomSeed = 1337;

	UPROPERTY(Transient)
	bool bHasAppliedDefaultAdvancedValues = false;

	UPROPERTY(Transient)
	bool bUpdatingRandomSeedSpinBox = false;

	UPROPERTY(Transient)
	bool bApplyingGameSetupSettings = false;

	UPROPERTY(Transient)
	bool bUpdatingCivilisationComboBoxes = false;

	UPROPERTY(Transient)
	bool bForceRebuildLobbySlots = false;
};
