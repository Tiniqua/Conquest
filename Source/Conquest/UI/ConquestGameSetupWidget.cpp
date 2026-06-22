// ConquestGameSetupWidget.cpp

#include "ConquestGameSetupWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "ConquestHUDWidget.h"
#include "Misc/Guid.h"

void UConquestGameSetupWidget::InitializeGameSetupWidget(UConquestHUDWidget* InParentHUDWidget)
{
	ParentHUDWidget = InParentHUDWidget;

	RefreshMapPresetOptions();
	RefreshMapSizeOptions();
	RefreshCivilisationOptions();
	RefreshLobbySlots();
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

	if (RandomSeedButton)
	{
		RandomSeedButton->OnClicked.RemoveDynamic(this, &UConquestGameSetupWidget::HandleRandomSeedButtonClicked);
		RandomSeedButton->OnClicked.AddDynamic(this, &UConquestGameSetupWidget::HandleRandomSeedButtonClicked);
	}

	BindCivilisationComboBoxes();
	ConfigureRandomSeedSpinBox();

	RefreshMapPresetOptions();
	RefreshMapSizeOptions();
	RefreshCivilisationOptions();
	RefreshLobbySlots();
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
	if (bHasAppliedDefaultAdvancedValues)
	{
		ConfigureRandomSeedSpinBox();
		SetSelectedRandomSeed(SelectedRandomSeed, true);
		return;
	}

	bHasAppliedDefaultAdvancedValues = true;

	// Match your current defaults from HexGridSettings.h.
	ConfigureRandomSeedSpinBox();
	SetSelectedRandomSeed(1337, true);

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

	if (BonusResourceCountSpinBox)
	{
		BonusResourceCountSpinBox->SetValue(0.0f);
	}

	if (LuxuryResourceCountSpinBox)
	{
		LuxuryResourceCountSpinBox->SetValue(0.0f);
	}

	if (StrategicResourceCountSpinBox)
	{
		StrategicResourceCountSpinBox->SetValue(0.0f);
	}

	if (ResourceSpacingSpinBox)
	{
		ResourceSpacingSpinBox->SetValue(2.0f);
	}

	if (GenerateRiversCheckBox)
	{
		GenerateRiversCheckBox->SetIsChecked(true);
	}

	if (RiverDensitySpinBox)
	{
		RiverDensitySpinBox->SetValue(0.12f);
	}

	if (MaxRiverCountSpinBox)
	{
		MaxRiverCountSpinBox->SetValue(40.0f);
	}

	if (MinRiverLengthSpinBox)
	{
		MinRiverLengthSpinBox->SetValue(4.0f);
	}

	if (MaxRiverLengthSpinBox)
	{
		MaxRiverLengthSpinBox->SetValue(12.0f);
	}

	if (RiverSpacingSpinBox)
	{
		RiverSpacingSpinBox->SetValue(3.0f);
	}

	FHexMapShapeSettings DefaultShape;
	FHexMapTypePreset SelectedPresetDefinition;
	if (FHexMapTypePresets::GetPreset(SelectedMapPreset, SelectedPresetDefinition))
	{
		DefaultShape = SelectedPresetDefinition.Shape;
	}

	if (LakeFrequencySpinBox)
	{
		LakeFrequencySpinBox->SetValue(DefaultShape.LakeFrequency);
	}

	if (LakeCountSpinBox)
	{
		LakeCountSpinBox->SetValue(static_cast<float>(DefaultShape.LakeCount));
	}

	if (LakeSpacingSpinBox)
	{
		LakeSpacingSpinBox->SetValue(static_cast<float>(DefaultShape.LakeSpacing));
	}

	if (LakeMinSizeSpinBox)
	{
		LakeMinSizeSpinBox->SetValue(static_cast<float>(DefaultShape.LakeMinSize));
	}

	if (LakeMaxSizeSpinBox)
	{
		LakeMaxSizeSpinBox->SetValue(static_cast<float>(DefaultShape.LakeMaxSize));
	}

	if (MountainAmountSpinBox)
	{
		MountainAmountSpinBox->SetValue(1.0f);
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

		FHexMapTypePreset Preset;
		if (FHexMapTypePresets::GetPreset(SelectedMapPreset, Preset))
		{
			if (LakeFrequencySpinBox)
			{
				LakeFrequencySpinBox->SetValue(Preset.Shape.LakeFrequency);
			}
			if (LakeCountSpinBox)
			{
				LakeCountSpinBox->SetValue(static_cast<float>(Preset.Shape.LakeCount));
			}
			if (LakeSpacingSpinBox)
			{
				LakeSpacingSpinBox->SetValue(static_cast<float>(Preset.Shape.LakeSpacing));
			}
			if (LakeMinSizeSpinBox)
			{
				LakeMinSizeSpinBox->SetValue(static_cast<float>(Preset.Shape.LakeMinSize));
			}
			if (LakeMaxSizeSpinBox)
			{
				LakeMaxSizeSpinBox->SetValue(static_cast<float>(Preset.Shape.LakeMaxSize));
			}
		}
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
			TEXT("%s: %dx%d tiles, %d players"),
			*Preset.DisplayName.ToString(),
			Preset.Width,
			Preset.Height,
			Preset.PlayerCount
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

	Settings.PlayerSlots = LobbyPlayerSlots;
	Settings.SizeSettings.HexRadius = 100.0f;

	Settings.RandomSeed = SelectedRandomSeed;

	FHexMapTypePreset Preset;
	if (FHexMapTypePresets::GetPreset(SelectedMapPreset, Preset))
	{
		Settings.MapShapeSettings = Preset.Shape;
		Settings.bUseCustomMapShapeSettings = true;
	}

	if (GenerateResourcesCheckBox)
	{
		Settings.ResourceGenerationSettings.bGenerateResources = GenerateResourcesCheckBox->IsChecked();
	}
	if (BonusResourceDensitySpinBox)
	{
		Settings.ResourceGenerationSettings.AutoBonusDensity = BonusResourceDensitySpinBox->GetValue();
	}
	if (LuxuryResourceDensitySpinBox)
	{
		Settings.ResourceGenerationSettings.AutoLuxuryDensity = LuxuryResourceDensitySpinBox->GetValue();
	}
	if (StrategicResourceDensitySpinBox)
	{
		Settings.ResourceGenerationSettings.AutoStrategicDensity = StrategicResourceDensitySpinBox->GetValue();
	}
	if (BonusResourceCountSpinBox)
	{
		Settings.ResourceGenerationSettings.BonusResourceCount = FMath::Max(0, FMath::RoundToInt(BonusResourceCountSpinBox->GetValue()));
	}
	if (LuxuryResourceCountSpinBox)
	{
		Settings.ResourceGenerationSettings.LuxuryResourceCount = FMath::Max(0, FMath::RoundToInt(LuxuryResourceCountSpinBox->GetValue()));
	}
	if (StrategicResourceCountSpinBox)
	{
		Settings.ResourceGenerationSettings.StrategicResourceCount = FMath::Max(0, FMath::RoundToInt(StrategicResourceCountSpinBox->GetValue()));
	}
	if (ResourceSpacingSpinBox)
	{
		Settings.ResourceGenerationSettings.ResourceMinSpacing = FMath::Max(0, FMath::RoundToInt(ResourceSpacingSpinBox->GetValue()));
	}

	if (UseTemperatureBiasCheckBox)
	{
		Settings.TemperatureSettings.bUseTemperatureBias = UseTemperatureBiasCheckBox->IsChecked();
	}
	if (TemperatureBiasStrengthSpinBox)
	{
		Settings.TemperatureSettings.TemperatureBiasStrength = TemperatureBiasStrengthSpinBox->GetValue();
		Settings.MapShapeSettings.TemperatureBiasStrength = TemperatureBiasStrengthSpinBox->GetValue();
	}
	if (PolarFalloffPowerSpinBox)
	{
		Settings.TemperatureSettings.PolarFalloffPower = FMath::Max(0.1f, PolarFalloffPowerSpinBox->GetValue());
		Settings.MapShapeSettings.PolarFalloffPower = Settings.TemperatureSettings.PolarFalloffPower;
	}
	if (TemperatureNoiseStrengthSpinBox)
	{
		Settings.TemperatureSettings.TemperatureNoiseStrength = FMath::Max(0.0f, TemperatureNoiseStrengthSpinBox->GetValue());
	}

	if (GenerateRiversCheckBox)
	{
		Settings.RiverSettings.bGenerateRivers = GenerateRiversCheckBox->IsChecked();
	}
	if (RiverDensitySpinBox)
	{
		Settings.RiverSettings.RiverDensityPer100Tiles = FMath::Max(0.0f, RiverDensitySpinBox->GetValue());
	}
	if (MaxRiverCountSpinBox)
	{
		Settings.RiverSettings.MaxRiverCount = FMath::Max(0, FMath::RoundToInt(MaxRiverCountSpinBox->GetValue()));
	}
	if (MinRiverLengthSpinBox)
	{
		Settings.RiverSettings.MinRiverLength = FMath::Max(1, FMath::RoundToInt(MinRiverLengthSpinBox->GetValue()));
	}
	if (MaxRiverLengthSpinBox)
	{
		Settings.RiverSettings.MaxRiverLength = FMath::Max(Settings.RiverSettings.MinRiverLength, FMath::RoundToInt(MaxRiverLengthSpinBox->GetValue()));
	}
	if (RiverSpacingSpinBox)
	{
		Settings.RiverSettings.RiverAvoidanceRadius = FMath::Max(0, FMath::RoundToInt(RiverSpacingSpinBox->GetValue()));
	}

	if (LakeFrequencySpinBox)
	{
		Settings.MapShapeSettings.LakeFrequency = FMath::Clamp(LakeFrequencySpinBox->GetValue(), 0.0f, 1.0f);
	}
	if (LakeCountSpinBox)
	{
		Settings.MapShapeSettings.LakeCount = FMath::Max(0, FMath::RoundToInt(LakeCountSpinBox->GetValue()));
	}
	if (LakeSpacingSpinBox)
	{
		Settings.MapShapeSettings.LakeSpacing = FMath::Max(0, FMath::RoundToInt(LakeSpacingSpinBox->GetValue()));
	}
	if (LakeMinSizeSpinBox)
	{
		Settings.MapShapeSettings.LakeMinSize = FMath::Max(1, FMath::RoundToInt(LakeMinSizeSpinBox->GetValue()));
	}
	if (LakeMaxSizeSpinBox)
	{
		Settings.MapShapeSettings.LakeMaxSize = FMath::Max(Settings.MapShapeSettings.LakeMinSize, FMath::RoundToInt(LakeMaxSizeSpinBox->GetValue()));
	}
	if (MountainAmountSpinBox)
	{
		Settings.MountainWeightScale = FMath::Max(0.0f, MountainAmountSpinBox->GetValue());
	}

	return Settings;
}

int32 UConquestGameSetupWidget::GenerateRandomSeed() const
{
	const FGuid Guid = FGuid::NewGuid();
	const uint32 MixedSeed = Guid.A ^ Guid.B ^ Guid.C ^ Guid.D;
	return FMath::Max(1, static_cast<int32>(MixedSeed & 0x7FFFFFFF));
}

void UConquestGameSetupWidget::HandleRandomSeedButtonClicked()
{
	SetSelectedRandomSeed(GenerateRandomSeed(), true);
}

void UConquestGameSetupWidget::HandleRandomSeedValueChanged(float NewValue)
{
	if (bUpdatingRandomSeedSpinBox)
	{
		return;
	}

	SelectedRandomSeed = FMath::Clamp(
		FMath::RoundToInt(NewValue),
		1,
		2147483646
	);
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
		RefreshLobbySlots();
	}
}

void UConquestGameSetupWidget::SetSelectedRandomSeed(int32 NewSeed, bool bUpdateSpinBox)
{
	SelectedRandomSeed = FMath::Clamp(NewSeed, 1, 2147483646);

	if (bUpdateSpinBox && RandomSeedSpinBox)
	{
		ConfigureRandomSeedSpinBox();
		bUpdatingRandomSeedSpinBox = true;
		RandomSeedSpinBox->SetValue(static_cast<float>(SelectedRandomSeed));
		bUpdatingRandomSeedSpinBox = false;
	}
}

void UConquestGameSetupWidget::ConfigureRandomSeedSpinBox()
{
	if (!RandomSeedSpinBox)
	{
		return;
	}

	RandomSeedSpinBox->SetMinValue(1.0f);
	RandomSeedSpinBox->SetMaxValue(2147483646.0f);
	RandomSeedSpinBox->OnValueChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleRandomSeedValueChanged);
	RandomSeedSpinBox->OnValueChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleRandomSeedValueChanged);
}

void UConquestGameSetupWidget::RefreshLobbySlots()
{
	FConquestMapSizePresetDefinition SizePreset;
	const int32 PlayerCount = FConquestMapSizePresets::GetPreset(SelectedMapSizePreset, SizePreset)
		? FMath::Max(1, SizePreset.PlayerCount)
		: 1;

	TArray<FConquestLobbyPlayerSlot> PreviousSlots = LobbyPlayerSlots;
	LobbyPlayerSlots.Reset(PlayerCount);

	for (int32 SlotIndex = 0; SlotIndex < PlayerCount; ++SlotIndex)
	{
		FConquestLobbyPlayerSlot LobbySlot;
		LobbySlot.SlotIndex = SlotIndex;
		LobbySlot.PlayerId = SlotIndex;
		LobbySlot.SlotType = SlotIndex == 0 ? EConquestLobbySlotType::Human : EConquestLobbySlotType::AI;
		LobbySlot.bIsHost = SlotIndex == 0;

		if (const FConquestLobbyPlayerSlot* PreviousSlot = PreviousSlots.FindByPredicate(
			[SlotIndex](const FConquestLobbyPlayerSlot& Candidate)
			{
				return Candidate.SlotIndex == SlotIndex;
			}))
		{
			LobbySlot.Civilisation = PreviousSlot->Civilisation;
		}

		if (!LobbySlot.Civilisation && AvailableCivilisations.Num() > 0)
		{
			LobbySlot.Civilisation = AvailableCivilisations[SlotIndex % AvailableCivilisations.Num()];
		}

		LobbyPlayerSlots.Add(LobbySlot);
	}

	const TArray<UComboBoxString*> ComboBoxes = GetCivilisationComboBoxes();
	for (int32 SlotIndex = 0; SlotIndex < ComboBoxes.Num(); ++SlotIndex)
	{
		RefreshCivilisationComboBox(ComboBoxes[SlotIndex], SlotIndex);
	}

	OnLobbySlotsChanged(LobbyPlayerSlots);
}

void UConquestGameSetupWidget::RefreshCivilisationOptions()
{
	OptionToCivilisation.Reset();
	AvailableCivilisations.Reset();

	if (UWorld* World = GetWorld())
	{
		if (const AConquestGameMode* ConquestGameMode = World->GetAuthGameMode<AConquestGameMode>())
		{
			const TArray<UConquestCivilisationData*> GameModeCivilisations = ConquestGameMode->GetAvailableCivilisations();
			for (UConquestCivilisationData* Civilisation : GameModeCivilisations)
			{
				if (Civilisation)
				{
					AvailableCivilisations.Add(Civilisation);
				}
			}
		}
	}

	for (int32 CivIndex = 0; CivIndex < AvailableCivilisations.Num(); ++CivIndex)
	{
		if (UConquestCivilisationData* Civilisation = AvailableCivilisations[CivIndex])
		{
			OptionToCivilisation.Add(GetCivilisationOptionName(Civilisation, CivIndex), Civilisation);
		}
	}

	const TArray<UComboBoxString*> ComboBoxes = GetCivilisationComboBoxes();
	for (int32 SlotIndex = 0; SlotIndex < ComboBoxes.Num(); ++SlotIndex)
	{
		RefreshCivilisationComboBox(ComboBoxes[SlotIndex], SlotIndex);
	}

	OnCivilisationOptionsChanged();
}

bool UConquestGameSetupWidget::SetLobbySlotCivilisation(int32 SlotIndex, UConquestCivilisationData* Civilisation)
{
	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	LobbyPlayerSlots[SlotIndex].Civilisation = Civilisation;
	const TArray<UComboBoxString*> ComboBoxes = GetCivilisationComboBoxes();
	RefreshCivilisationComboBox(ComboBoxes.IsValidIndex(SlotIndex) ? ComboBoxes[SlotIndex] : nullptr, SlotIndex);
	return true;
}

TArray<FConquestLobbyPlayerSlot> UConquestGameSetupWidget::GetLobbyPlayerSlots() const
{
	return LobbyPlayerSlots;
}

TArray<UConquestCivilisationData*> UConquestGameSetupWidget::GetAvailableCivilisations() const
{
	TArray<UConquestCivilisationData*> Result;
	Result.Reserve(AvailableCivilisations.Num());

	for (UConquestCivilisationData* Civilisation : AvailableCivilisations)
	{
		if (Civilisation)
		{
			Result.Add(Civilisation);
		}
	}

	return Result;
}

void UConquestGameSetupWidget::BindCivilisationComboBoxes()
{
	if (CivSlot0ComboBox)
	{
		CivSlot0ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot0SelectionChanged);
		CivSlot0ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot0SelectionChanged);
	}
	if (CivSlot1ComboBox)
	{
		CivSlot1ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot1SelectionChanged);
		CivSlot1ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot1SelectionChanged);
	}
	if (CivSlot2ComboBox)
	{
		CivSlot2ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot2SelectionChanged);
		CivSlot2ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot2SelectionChanged);
	}
	if (CivSlot3ComboBox)
	{
		CivSlot3ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot3SelectionChanged);
		CivSlot3ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot3SelectionChanged);
	}
	if (CivSlot4ComboBox)
	{
		CivSlot4ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot4SelectionChanged);
		CivSlot4ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot4SelectionChanged);
	}
	if (CivSlot5ComboBox)
	{
		CivSlot5ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot5SelectionChanged);
		CivSlot5ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot5SelectionChanged);
	}
	if (CivSlot6ComboBox)
	{
		CivSlot6ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot6SelectionChanged);
		CivSlot6ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot6SelectionChanged);
	}
	if (CivSlot7ComboBox)
	{
		CivSlot7ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot7SelectionChanged);
		CivSlot7ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot7SelectionChanged);
	}
	if (CivSlot8ComboBox)
	{
		CivSlot8ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot8SelectionChanged);
		CivSlot8ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot8SelectionChanged);
	}
	if (CivSlot9ComboBox)
	{
		CivSlot9ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot9SelectionChanged);
		CivSlot9ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot9SelectionChanged);
	}
	if (CivSlot10ComboBox)
	{
		CivSlot10ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot10SelectionChanged);
		CivSlot10ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot10SelectionChanged);
	}
	if (CivSlot11ComboBox)
	{
		CivSlot11ComboBox->OnSelectionChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleCivSlot11SelectionChanged);
		CivSlot11ComboBox->OnSelectionChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleCivSlot11SelectionChanged);
	}
}

void UConquestGameSetupWidget::RefreshCivilisationComboBox(UComboBoxString* ComboBox, int32 SlotIndex)
{
	if (!ComboBox)
	{
		return;
	}

	ComboBox->ClearOptions();
	ComboBox->SetIsEnabled(LobbyPlayerSlots.IsValidIndex(SlotIndex));

	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	for (int32 CivIndex = 0; CivIndex < AvailableCivilisations.Num(); ++CivIndex)
	{
		if (UConquestCivilisationData* Civilisation = AvailableCivilisations[CivIndex])
		{
			ComboBox->AddOption(GetCivilisationOptionName(Civilisation, CivIndex));
		}
	}

	if (const UConquestCivilisationData* SelectedCivilisation = LobbyPlayerSlots[SlotIndex].Civilisation)
	{
		for (int32 CivIndex = 0; CivIndex < AvailableCivilisations.Num(); ++CivIndex)
		{
			if (AvailableCivilisations[CivIndex] == SelectedCivilisation)
			{
				ComboBox->SetSelectedOption(GetCivilisationOptionName(SelectedCivilisation, CivIndex));
				return;
			}
		}
	}
}

void UConquestGameSetupWidget::HandleCivilisationSelectionChanged(int32 SlotIndex, const FString& SelectedItem)
{
	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	if (TObjectPtr<UConquestCivilisationData>* Civilisation = OptionToCivilisation.Find(SelectedItem))
	{
		LobbyPlayerSlots[SlotIndex].Civilisation = Civilisation->Get();
	}
}

TArray<UComboBoxString*> UConquestGameSetupWidget::GetCivilisationComboBoxes() const
{
	TArray<UComboBoxString*> ComboBoxes;
	ComboBoxes.Reserve(12);
	ComboBoxes.Add(CivSlot0ComboBox.Get());
	ComboBoxes.Add(CivSlot1ComboBox.Get());
	ComboBoxes.Add(CivSlot2ComboBox.Get());
	ComboBoxes.Add(CivSlot3ComboBox.Get());
	ComboBoxes.Add(CivSlot4ComboBox.Get());
	ComboBoxes.Add(CivSlot5ComboBox.Get());
	ComboBoxes.Add(CivSlot6ComboBox.Get());
	ComboBoxes.Add(CivSlot7ComboBox.Get());
	ComboBoxes.Add(CivSlot8ComboBox.Get());
	ComboBoxes.Add(CivSlot9ComboBox.Get());
	ComboBoxes.Add(CivSlot10ComboBox.Get());
	ComboBoxes.Add(CivSlot11ComboBox.Get());
	return ComboBoxes;
}

FString UConquestGameSetupWidget::GetCivilisationOptionName(const UConquestCivilisationData* Civilisation, int32 OptionIndex) const
{
	if (!Civilisation)
	{
		return FString();
	}

	const FString DisplayName = Civilisation->CivilisationName.IsEmpty()
		? Civilisation->GetName()
		: Civilisation->CivilisationName.ToString();

	return FString::Printf(TEXT("%s [%d]"), *DisplayName, OptionIndex + 1);
}

void UConquestGameSetupWidget::HandleCivSlot0SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(0, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot1SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(1, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot2SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(2, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot3SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(3, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot4SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(4, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot5SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(5, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot6SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(6, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot7SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(7, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot8SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(8, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot9SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(9, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot10SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(10, SelectedItem);
}

void UConquestGameSetupWidget::HandleCivSlot11SelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	HandleCivilisationSelectionChanged(11, SelectedItem);
}
