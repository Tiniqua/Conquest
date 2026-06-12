#include "ConquestGameSetupWidget.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "ConquestHUDWidget.h"

void UConquestGameSetupWidget::InitializeGameSetupWidget(UConquestHUDWidget* InParentHUDWidget)
{
	ParentHUDWidget = InParentHUDWidget;

	RefreshMapPresetOptions();
}

void UConquestGameSetupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MapPresetComboBox)
	{
		MapPresetComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleMapPresetSelectionChanged);
		MapPresetComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleMapPresetSelectionChanged);
	}

	if (PlayButton)
	{
		PlayButton->OnClicked.RemoveDynamic(this, &UConquestGameSetupWidget::HandlePlayButtonClicked);
		PlayButton->OnClicked.AddDynamic(this, &UConquestGameSetupWidget::HandlePlayButtonClicked);
	}

	RefreshMapPresetOptions();
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

void UConquestGameSetupWidget::HandleMapPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (const EHexMapTypePreset* FoundPreset = OptionToPreset.Find(SelectedItem))
	{
		SelectedMapPreset = *FoundPreset;
	}
}

void UConquestGameSetupWidget::HandlePlayButtonClicked()
{
	if (ParentHUDWidget)
	{
		ParentHUDWidget->RequestStartGame(SelectedMapPreset);
	}
}