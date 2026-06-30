#include "Conquest/UI/ConquestSettingsWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/UI/ConquestHUD.h"

void UConquestSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MusicEnabledCheckBox)
	{
		MusicEnabledCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UConquestSettingsWidget::HandleMusicEnabledChanged);
		MusicEnabledCheckBox->OnCheckStateChanged.AddDynamic(this, &UConquestSettingsWidget::HandleMusicEnabledChanged);
	}

	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetMinValue(0.0f);
		MusicVolumeSlider->SetMaxValue(2.0f);
		MusicVolumeSlider->OnValueChanged.RemoveDynamic(this, &UConquestSettingsWidget::HandleMusicVolumeSliderChanged);
		MusicVolumeSlider->OnValueChanged.AddDynamic(this, &UConquestSettingsWidget::HandleMusicVolumeSliderChanged);
	}

	if (MusicVolumeSpinBox)
	{
		MusicVolumeSpinBox->SetMinValue(0.0f);
		MusicVolumeSpinBox->SetMaxValue(2.0f);
		MusicVolumeSpinBox->OnValueChanged.RemoveDynamic(this, &UConquestSettingsWidget::HandleMusicVolumeSpinBoxChanged);
		MusicVolumeSpinBox->OnValueChanged.AddDynamic(this, &UConquestSettingsWidget::HandleMusicVolumeSpinBoxChanged);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UConquestSettingsWidget::HandleCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &UConquestSettingsWidget::HandleCloseClicked);
	}

	RefreshFromSettings();
}

void UConquestSettingsWidget::NativeDestruct()
{
	if (MusicEnabledCheckBox)
	{
		MusicEnabledCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UConquestSettingsWidget::HandleMusicEnabledChanged);
	}

	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.RemoveDynamic(this, &UConquestSettingsWidget::HandleMusicVolumeSliderChanged);
	}

	if (MusicVolumeSpinBox)
	{
		MusicVolumeSpinBox->OnValueChanged.RemoveDynamic(this, &UConquestSettingsWidget::HandleMusicVolumeSpinBoxChanged);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UConquestSettingsWidget::HandleCloseClicked);
	}

	Super::NativeDestruct();
}

void UConquestSettingsWidget::RefreshFromSettings()
{
	const AConquestPlayerController* ConquestPC = GetConquestPlayerController();
	if (!ConquestPC)
	{
		return;
	}

	bRefreshingSettingsControls = true;

	if (MusicEnabledCheckBox)
	{
		MusicEnabledCheckBox->SetIsChecked(ConquestPC->IsMusicEnabled());
	}

	const float MusicVolume = ConquestPC->GetMusicVolumeMultiplier();
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(MusicVolume);
	}

	if (MusicVolumeSpinBox)
	{
		MusicVolumeSpinBox->SetValue(MusicVolume);
	}

	bRefreshingSettingsControls = false;
}

void UConquestSettingsWidget::HandleMusicEnabledChanged(bool bIsChecked)
{
	if (bRefreshingSettingsControls)
	{
		return;
	}

	if (AConquestPlayerController* ConquestPC = GetConquestPlayerController())
	{
		ConquestPC->SetMusicEnabled(bIsChecked);
	}
}

void UConquestSettingsWidget::HandleMusicVolumeSliderChanged(float Value)
{
	ApplyMusicVolume(Value);
}

void UConquestSettingsWidget::HandleMusicVolumeSpinBoxChanged(float Value)
{
	ApplyMusicVolume(Value);
}

void UConquestSettingsWidget::HandleCloseClicked()
{
	if (AConquestPlayerController* ConquestPC = GetConquestPlayerController())
	{
		if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(ConquestPC->GetHUD()))
		{
			ConquestHUD->HideSettingsMenu();
			return;
		}
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

AConquestPlayerController* UConquestSettingsWidget::GetConquestPlayerController() const
{
	return Cast<AConquestPlayerController>(GetOwningPlayer());
}

void UConquestSettingsWidget::ApplyMusicVolume(float Value)
{
	if (bRefreshingSettingsControls)
	{
		return;
	}

	const float ClampedValue = FMath::Clamp(Value, 0.0f, 2.0f);
	if (AConquestPlayerController* ConquestPC = GetConquestPlayerController())
	{
		ConquestPC->SetMusicVolumeMultiplier(ClampedValue);
	}

	RefreshFromSettings();
}
