#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/World/Generation/HexMapTypePresets.h"
#include "ConquestGameSetupWidget.generated.h"

class UButton;
class UComboBoxString;
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

	UFUNCTION(BlueprintPure, Category = "Conquest|Game Setup")
	EHexMapTypePreset GetSelectedMapPreset() const { return SelectedMapPreset; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> MapPresetComboBox = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> ParentHUDWidget = nullptr;

	UPROPERTY(Transient)
	TMap<FString, EHexMapTypePreset> OptionToPreset;

	UPROPERTY(Transient)
	EHexMapTypePreset SelectedMapPreset = EHexMapTypePreset::Continents;

	UFUNCTION()
	void HandleMapPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandlePlayButtonClicked();
};