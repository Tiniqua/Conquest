// ConquestGameSetupWidget.cpp

#include "ConquestGameSetupWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "ConquestHUDWidget.h"

void UConquestGameSetupWidget::InitializeGameSetupWidget(UConquestHUDWidget* InParentHUDWidget)
{
	ParentHUDWidget = InParentHUDWidget;

	RefreshMapPresetOptions();
	RefreshMapSizeOptions();
	ApplyDefaultAdvancedValues();
	UpdateMapSizeTooltip();
}

void UConquestGameSetupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MapPresetComboBox)
	{
		MapPresetComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleMapPresetSelectionChanged);
		MapPresetComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleMapPresetSelectionChanged);
	}

	if (MapSizeComboBox)
	{
		MapSizeComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleMapSizeSelectionChanged);
		MapSizeComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleMapSizeSelectionChanged);
	}

	if (PlayButton)
	{
		PlayButton->OnClicked.RemoveDynamic(this, &UConquestGameSetupWidget::HandlePlayButtonClicked);
		PlayButton->OnClicked.AddDynamic(this, &UConquestGameSetupWidget::HandlePlayButtonClicked);
	}

	RefreshMapPresetOptions();
	RefreshMapSizeOptions();
	ApplyDefaultAdvancedValues();
	UpdateMapSizeTooltip();
}

void UConquestGameSetupWidget::RefreshMapPresetOptions()
{
	OptionToPreset.Reset();

	if (!MapPresetComboBox)
	{
		return;
	}

	MapPresetComboBox->ClearOptions();

	const TArray<FHexMapTypePreset> Presets = FHexMapTypePresets::GetAllPresets();

	for (const FHexMapTypePreset& Preset : Presets)
	{
		const FString OptionName = Preset.DisplayName.IsEmpty()
			? Preset.PresetId.ToString()
			: Preset.DisplayName.ToString();

		MapPresetComboBox->AddOption(OptionName);
		OptionToPreset.Add(OptionName, Preset.PresetType);
	}

	FHexMapTypePreset DefaultPreset;
	if (FHexMapTypePresets::GetPreset(EHexMapTypePreset::Continents, DefaultPreset))
	{
		const FString DefaultOptionName = DefaultPreset.DisplayName.IsEmpty()
			? DefaultPreset.PresetId.ToString()
			: DefaultPreset.DisplayName.ToString();

		if (MapPresetComboBox->FindOptionIndex(DefaultOptionName) != INDEX_NONE)
		{
			MapPresetComboBox->SetSelectedOption(DefaultOptionName);
			SelectedMapPreset = EHexMapTypePreset::Continents;
			return;
		}
	}

	if (Presets.Num() > 0)
	{
		const FString FirstOptionName = Presets[0].DisplayName.IsEmpty()
			? Presets[0].PresetId.ToString()
			: Presets[0].DisplayName.ToString();

		MapPresetComboBox->SetSelectedOption(FirstOptionName);
		SelectedMapPreset = Presets[0].PresetType;
	}
}

void UConquestGameSetupWidget::RefreshMapSizeOptions()
{
	OptionToMapSizePreset.Reset();

	if (!MapSizeComboBox)
	{
		return;
	}

	MapSizeComboBox->ClearOptions();

	const TArray<FConquestMapSizePresetDefinition> SizePresets = FConquestMapSizePresets::GetAllPresets();

	for (const FConquestMapSizePresetDefinition& Preset : SizePresets)
	{
		const FString OptionName = Preset.DisplayName.ToString();

		MapSizeComboBox->AddOption(OptionName);
		OptionToMapSizePreset.Add(OptionName, Preset.Preset);
	}

	FConquestMapSizePresetDefinition DefaultPreset;
	if (FConquestMapSizePresets::GetPreset(EConquestMapSizePreset::Standard, DefaultPreset))
	{
		const FString DefaultOptionName = DefaultPreset.DisplayName.ToString();

		if (MapSizeComboBox->FindOptionIndex(DefaultOptionName) != INDEX_NONE)
		{
			MapSizeComboBox->SetSelectedOption(DefaultOptionName);
			SelectedMapSizePreset = EConquestMapSizePreset::Standard;
			UpdateMapSizeTooltip();
			return;
		}
	}

	if (SizePresets.Num() > 0)
	{
		const FString FirstOptionName = SizePresets[0].DisplayName.ToString();

		MapSizeComboBox->SetSelectedOption(FirstOptionName);
		SelectedMapSizePreset = SizePresets[0].Preset;
		UpdateMapSizeTooltip();
	}
}

void UConquestGameSetupWidget::ApplyDefaultAdvancedValues()
{
	// Match your current defaults from HexGridSettings.h.
	if (GenerateRiversCheckBox)
	{
		GenerateRiversCheckBox->SetIsChecked(true);
	}

	if (RiverCountSpinBox)
	{
		RiverCountSpinBox->SetValue(12.0f);
	}

	if (MinRiverLengthSpinBox)
	{
		MinRiverLengthSpinBox->SetValue(10.0f);
	}

	if (MaxRiverLengthSpinBox)
	{
		MaxRiverLengthSpinBox->SetValue(20.0f);
	}

	if (RiverAvoidanceRadiusSpinBox)
	{
		RiverAvoidanceRadiusSpinBox->SetValue(5.0f);
	}

	if (RiverStartChanceSpinBox)
	{
		RiverStartChanceSpinBox->SetValue(1.0f);
	}

	if (GenerateResourcesCheckBox)
	{
		GenerateResourcesCheckBox->SetIsChecked(true);
	}

	if (BonusResourceDensitySpinBox)
	{
		BonusResourceDensitySpinBox->SetValue(0.08f);
	}

	if (LuxuryResourceDensitySpinBox)
	{
		LuxuryResourceDensitySpinBox->SetValue(0.05f);
	}

	if (StrategicResourceDensitySpinBox)
	{
		StrategicResourceDensitySpinBox->SetValue(0.04f);
	}

	if (UseTemperatureBiasCheckBox)
	{
		UseTemperatureBiasCheckBox->SetIsChecked(true);
	}

	if (TemperatureBiasStrengthSpinBox)
	{
		TemperatureBiasStrengthSpinBox->SetValue(2.0f);
	}

	if (PolarFalloffPowerSpinBox)
	{
		PolarFalloffPowerSpinBox->SetValue(1.0f);
	}

	if (TemperatureNoiseStrengthSpinBox)
	{
		TemperatureNoiseStrengthSpinBox->SetValue(0.12f);
	}
}

void UConquestGameSetupWidget::HandleMapPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (const EHexMapTypePreset* FoundPreset = OptionToPreset.Find(SelectedItem))
	{
		SelectedMapPreset = *FoundPreset;
	}
}

void UConquestGameSetupWidget::UpdateMapSizeTooltip()
{
	FConquestMapSizePresetDefinition Preset;
	if (!FConquestMapSizePresets::GetPreset(SelectedMapSizePreset, Preset))
	{
		return;
	}

	const FText TooltipText = FText::FromString(
		FString::Printf(
			TEXT("%s: %dx%d tiles"),
			*Preset.DisplayName.ToString(),
			Preset.Width,
			Preset.Height
		)
	);

	if (MapSizeTooltipText)
	{
		MapSizeTooltipText->SetText(TooltipText);
	}

	if (MapSizeComboBox)
	{
		MapSizeComboBox->SetToolTipText(TooltipText);
	}
}

FConquestGameSetupSettings UConquestGameSetupWidget::GetSelectedGameSetupSettings() const
{
	FConquestGameSetupSettings Settings;

	Settings.MapTypePreset = SelectedMapPreset;
	Settings.MapSizePreset = SelectedMapSizePreset;

	FConquestMapSizePresetDefinition SizePreset;
	if (FConquestMapSizePresets::GetPreset(SelectedMapSizePreset, SizePreset))
	{
		Settings.SizeSettings.GridWidth = SizePreset.Width;
		Settings.SizeSettings.GridHeight = SizePreset.Height;
	}

	Settings.SizeSettings.HexRadius = 100.0f;

	Settings.RiverGenerationSettings.bGenerateRivers = GenerateRiversCheckBox
		? GenerateRiversCheckBox->IsChecked()
		: true;

	Settings.RiverGenerationSettings.RiverCount = RiverCountSpinBox
		? FMath::RoundToInt(RiverCountSpinBox->GetValue())
		: 12;

	Settings.RiverGenerationSettings.MinRiverLength = MinRiverLengthSpinBox
		? FMath::RoundToInt(MinRiverLengthSpinBox->GetValue())
		: 10;

	Settings.RiverGenerationSettings.MaxRiverLength = MaxRiverLengthSpinBox
		? FMath::RoundToInt(MaxRiverLengthSpinBox->GetValue())
		: 20;

	Settings.RiverGenerationSettings.RiverAvoidanceRadius = RiverAvoidanceRadiusSpinBox
		? FMath::RoundToInt(RiverAvoidanceRadiusSpinBox->GetValue())
		: 5;

	Settings.RiverGenerationSettings.RiverStartChance = RiverStartChanceSpinBox
		? FMath::Clamp(RiverStartChanceSpinBox->GetValue(), 0.0f, 1.0f)
		: 1.0f;

	Settings.RiverGenerationSettings.MaxRiverLength = FMath::Max(
		Settings.RiverGenerationSettings.MinRiverLength,
		Settings.RiverGenerationSettings.MaxRiverLength
	);

	Settings.ResourceGenerationSettings.bGenerateResources = GenerateResourcesCheckBox
		? GenerateResourcesCheckBox->IsChecked()
		: true;

	Settings.ResourceGenerationSettings.AutoBonusDensity = BonusResourceDensitySpinBox
		? FMath::Clamp(BonusResourceDensitySpinBox->GetValue(), 0.0f, 1.0f)
		: 0.08f;

	Settings.ResourceGenerationSettings.AutoLuxuryDensity = LuxuryResourceDensitySpinBox
		? FMath::Clamp(LuxuryResourceDensitySpinBox->GetValue(), 0.0f, 1.0f)
		: 0.05f;

	Settings.ResourceGenerationSettings.AutoStrategicDensity = StrategicResourceDensitySpinBox
		? FMath::Clamp(StrategicResourceDensitySpinBox->GetValue(), 0.0f, 1.0f)
		: 0.04f;

	Settings.TemperatureSettings.bUseTemperatureBias = UseTemperatureBiasCheckBox
		? UseTemperatureBiasCheckBox->IsChecked()
		: true;

	Settings.TemperatureSettings.TemperatureBiasStrength = TemperatureBiasStrengthSpinBox
		? FMath::Max(0.0f, TemperatureBiasStrengthSpinBox->GetValue())
		: 2.0f;

	Settings.TemperatureSettings.PolarFalloffPower = PolarFalloffPowerSpinBox
		? FMath::Max(0.1f, PolarFalloffPowerSpinBox->GetValue())
		: 1.0f;

	Settings.TemperatureSettings.TemperatureNoiseStrength = TemperatureNoiseStrengthSpinBox
		? FMath::Max(0.0f, TemperatureNoiseStrengthSpinBox->GetValue())
		: 0.12f;

	return Settings;
}

void UConquestGameSetupWidget::HandlePlayButtonClicked()
{
	if (ParentHUDWidget)
	{
		ParentHUDWidget->RequestStartGame(GetSelectedGameSetupSettings());
	}
}

void UConquestGameSetupWidget::HandleMapSizeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (const EConquestMapSizePreset* FoundPreset = OptionToMapSizePreset.Find(SelectedItem))
	{
		SelectedMapSizePreset = *FoundPreset;
		UpdateMapSizeTooltip();

		if (RiverCountSpinBox)
		{
			RiverCountSpinBox->SetValue(GetRecommendedRiverCountForMapSize(SelectedMapSizePreset));
		}
	}
}

int32 UConquestGameSetupWidget::GetRecommendedRiverCountForMapSize(EConquestMapSizePreset MapSizePreset) const
{
	switch (MapSizePreset)
	{
	case EConquestMapSizePreset::Tiny:
		return 6;

	case EConquestMapSizePreset::Small:
		return 9;

	case EConquestMapSizePreset::Standard:
		return 12;

	case EConquestMapSizePreset::Large:
		return 24;

	case EConquestMapSizePreset::Huge:
		return 40;

	default:
		return 12;
	}
}