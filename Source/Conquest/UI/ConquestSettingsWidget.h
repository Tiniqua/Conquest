#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestSettingsWidget.generated.h"

class AConquestPlayerController;
class UButton;
class UCheckBox;
class USlider;
class USpinBox;

UCLASS()
class CONQUEST_API UConquestSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Settings")
	void RefreshFromSettings();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> MusicEnabledCheckBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MusicVolumeSlider = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> MusicVolumeSpinBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton = nullptr;

	UFUNCTION()
	void HandleMusicEnabledChanged(bool bIsChecked);

	UFUNCTION()
	void HandleMusicVolumeSliderChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeSpinBoxChanged(float Value);

	UFUNCTION()
	void HandleCloseClicked();

private:
	bool bRefreshingSettingsControls = false;

	AConquestPlayerController* GetConquestPlayerController() const;
	void ApplyMusicVolume(float Value);
};
