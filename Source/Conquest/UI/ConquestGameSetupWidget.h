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

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> MapPresetComboBox = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> MapSizeComboBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapSizeTooltipText = nullptr;

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

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> ParentHUDWidget = nullptr;

	UPROPERTY(Transient)
	TMap<FString, EHexMapTypePreset> OptionToPreset;

	UPROPERTY(Transient)
	TMap<FString, EConquestMapSizePreset> OptionToMapSizePreset;

	UPROPERTY(Transient)
	EHexMapTypePreset SelectedMapPreset = EHexMapTypePreset::Continents;

	UPROPERTY(Transient)
	EConquestMapSizePreset SelectedMapSizePreset = EConquestMapSizePreset::Standard;

	UFUNCTION()
	void HandleMapPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleMapSizeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandlePlayButtonClicked();

	UFUNCTION()
	void HandleRandomSeedButtonClicked();

	UFUNCTION()
	void HandleRandomSeedValueChanged(float NewValue);

	int32 GenerateRandomSeed() const;

private:
	void ApplyDefaultAdvancedValues();
	void SetSelectedRandomSeed(int32 NewSeed, bool bUpdateSpinBox);
	void ConfigureRandomSeedSpinBox();
	void UpdateMapSizeTooltip();

	UPROPERTY(Transient)
	int32 SelectedRandomSeed = 1337;

	UPROPERTY(Transient)
	bool bHasAppliedDefaultAdvancedValues = false;

	UPROPERTY(Transient)
	bool bUpdatingRandomSeedSpinBox = false;
};
