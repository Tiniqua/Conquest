// ConquestGameSetupWidget.cpp

#include "ConquestGameSetupWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestModifierTypes.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "ConquestHUDWidget.h"
#include "Misc/Guid.h"

namespace
{
	FString ConquestSetupEnumValueToString(const UEnum* Enum, int64 Value)
	{
		if (!Enum)
		{
			return TEXT("Unknown");
		}

		FString Name = Enum->GetDisplayNameTextByValue(Value).ToString();
		if (Name.IsEmpty())
		{
			Name = Enum->GetNameStringByValue(Value);
		}
		return Name;
	}

	FString ConquestSetupFormatSignedInt(int32 Value)
	{
		return Value > 0
			? FString::Printf(TEXT("+%d"), Value)
			: FString::Printf(TEXT("%d"), Value);
	}

	FString ConquestSetupFormatSignedFloat(float Value)
	{
		return Value > 0.0f
			? FString::Printf(TEXT("+%.1f"), Value)
			: FString::Printf(TEXT("%.1f"), Value);
	}

	FString ConquestSetupFormatPercent(float Value)
	{
		return FString::Printf(TEXT("%+.0f%%"), Value * 100.0f);
	}

	void ConquestSetupAppendLine(TArray<FString>& Lines, const FString& Line)
	{
		if (!Line.IsEmpty())
		{
			Lines.Add(Line);
		}
	}

	FString ConquestSetupFormatYield(const FHexYield& Yield)
	{
		TArray<FString> Parts;
		if (Yield.Food != 0)
		{
			Parts.Add(FString::Printf(TEXT("%s Food"), *ConquestSetupFormatSignedInt(Yield.Food)));
		}
		if (Yield.Production != 0)
		{
			Parts.Add(FString::Printf(TEXT("%s Production"), *ConquestSetupFormatSignedInt(Yield.Production)));
		}
		if (Yield.Gold != 0)
		{
			Parts.Add(FString::Printf(TEXT("%s Gold"), *ConquestSetupFormatSignedInt(Yield.Gold)));
		}
		if (Yield.Science != 0)
		{
			Parts.Add(FString::Printf(TEXT("%s Science"), *ConquestSetupFormatSignedInt(Yield.Science)));
		}
		if (Yield.Culture != 0)
		{
			Parts.Add(FString::Printf(TEXT("%s Culture"), *ConquestSetupFormatSignedInt(Yield.Culture)));
		}
		return FString::Join(Parts, TEXT(", "));
	}

	FString ConquestSetupFormatModifier(const FConquestModifierDefinition& Modifier)
	{
		if (!Modifier.Description.IsEmpty())
		{
			return Modifier.Description.ToString();
		}

		const FString OperationName = ConquestSetupEnumValueToString(
			StaticEnum<EConquestModifierOperation>(),
			static_cast<int64>(Modifier.Operation)
		);
		const FString AttributeName = ConquestSetupEnumValueToString(
			StaticEnum<EConquestModifierAttribute>(),
			static_cast<int64>(Modifier.Attribute)
		);
		const FString ScopeName = ConquestSetupEnumValueToString(
			StaticEnum<EConquestModifierTargetScope>(),
			static_cast<int64>(Modifier.TargetScope)
		);

		FString ValueText;
		switch (Modifier.Operation)
		{
		case EConquestModifierOperation::AddPercent:
			ValueText = ConquestSetupFormatPercent(Modifier.Value);
			break;
		case EConquestModifierOperation::Multiply:
			ValueText = FString::Printf(TEXT("x%.2f"), Modifier.Value);
			break;
		default:
			ValueText = ConquestSetupFormatSignedFloat(Modifier.Value);
			break;
		}

		FString Result = FString::Printf(TEXT("%s %s to %s"), *ValueText, *AttributeName, *ScopeName);
		if (Modifier.CountSource != EConquestModifierCountSource::None)
		{
			const FString CountSourceName = ConquestSetupEnumValueToString(
				StaticEnum<EConquestModifierCountSource>(),
				static_cast<int64>(Modifier.CountSource)
			);
			Result += FString::Printf(TEXT(" per %d %s"), FMath::Max(1, Modifier.CountDivisor), *CountSourceName);
		}
		if (Modifier.Conditions.Num() > 0)
		{
			Result += FString::Printf(TEXT(" (%d condition%s)"), Modifier.Conditions.Num(), Modifier.Conditions.Num() == 1 ? TEXT("") : TEXT("s"));
		}
		if (!OperationName.IsEmpty() && Modifier.Operation == EConquestModifierOperation::Override)
		{
			Result += FString::Printf(TEXT(" [%s]"), *OperationName);
		}
		return Result;
	}

	FString ConquestSetupFormatUnitStats(const FConquestUnitRow& Unit)
	{
		TArray<FString> Parts;
		Parts.Add(FString::Printf(
			TEXT("Cost %d | Str %d | Range %d | HP %d | Regen %d | Move %d | Upkeep %d"),
			Unit.ProductionCost,
			Unit.Strength,
			Unit.AttackRange,
			Unit.MaxHealth,
			Unit.HealthRegenPerTurn,
			Unit.MovementPoints,
			Unit.GoldMaintenancePerTurn
		));
		if (!Unit.RequiredTechId.IsNone())
		{
			Parts.Add(FString::Printf(TEXT("Requires %s"), *Unit.RequiredTechId.ToString()));
		}
		if (!Unit.UpgradesToUnitId.IsNone())
		{
			Parts.Add(FString::Printf(TEXT("Upgrades to %s (%d Gold)"), *Unit.UpgradesToUnitId.ToString(), Unit.UpgradeGoldCost));
		}
		if (!Unit.UpgradeUnlockedByTechId.IsNone())
		{
			Parts.Add(FString::Printf(TEXT("Upgrade tech %s"), *Unit.UpgradeUnlockedByTechId.ToString()));
		}
		if (Unit.bCanFoundCity)
		{
			Parts.Add(TEXT("Can found cities"));
		}
		if (Unit.AllowedAugmentIds.Num() > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d allowed augment%s"), Unit.AllowedAugmentIds.Num(), Unit.AllowedAugmentIds.Num() == 1 ? TEXT("") : TEXT("s")));
		}
		if (Unit.Modifiers.Num() > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d modifier%s"), Unit.Modifiers.Num(), Unit.Modifiers.Num() == 1 ? TEXT("") : TEXT("s")));
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	FString ConquestSetupFormatUnitDiff(const FConquestUnitRow& Unit, const FConquestUnitRow& BaseUnit)
	{
		TArray<FString> Parts;
		if (Unit.ProductionCost != BaseUnit.ProductionCost)
		{
			Parts.Add(FString::Printf(TEXT("%s Cost"), *ConquestSetupFormatSignedInt(Unit.ProductionCost - BaseUnit.ProductionCost)));
		}
		if (Unit.Strength != BaseUnit.Strength)
		{
			Parts.Add(FString::Printf(TEXT("%s Atk"), *ConquestSetupFormatSignedInt(Unit.Strength - BaseUnit.Strength)));
		}
		if (Unit.AttackRange != BaseUnit.AttackRange)
		{
			Parts.Add(FString::Printf(TEXT("%s Range"), *ConquestSetupFormatSignedInt(Unit.AttackRange - BaseUnit.AttackRange)));
		}
		if (Unit.MaxHealth != BaseUnit.MaxHealth)
		{
			Parts.Add(FString::Printf(TEXT("%s HP"), *ConquestSetupFormatSignedInt(Unit.MaxHealth - BaseUnit.MaxHealth)));
		}
		if (Unit.HealthRegenPerTurn != BaseUnit.HealthRegenPerTurn)
		{
			Parts.Add(FString::Printf(TEXT("%s Regen"), *ConquestSetupFormatSignedInt(Unit.HealthRegenPerTurn - BaseUnit.HealthRegenPerTurn)));
		}
		if (Unit.MovementPoints != BaseUnit.MovementPoints)
		{
			Parts.Add(FString::Printf(TEXT("%s Move"), *ConquestSetupFormatSignedInt(Unit.MovementPoints - BaseUnit.MovementPoints)));
		}
		if (Unit.GoldMaintenancePerTurn != BaseUnit.GoldMaintenancePerTurn)
		{
			Parts.Add(FString::Printf(TEXT("%s Upkeep"), *ConquestSetupFormatSignedInt(Unit.GoldMaintenancePerTurn - BaseUnit.GoldMaintenancePerTurn)));
		}
		if (Unit.bCanFoundCity != BaseUnit.bCanFoundCity)
		{
			Parts.Add(Unit.bCanFoundCity ? TEXT("Can found cities") : TEXT("Cannot found cities"));
		}
		if (Unit.RequiredTechId != BaseUnit.RequiredTechId)
		{
			Parts.Add(FString::Printf(TEXT("Requires %s"), Unit.RequiredTechId.IsNone() ? TEXT("no tech") : *Unit.RequiredTechId.ToString()));
		}
		if (Unit.UpgradesToUnitId != BaseUnit.UpgradesToUnitId)
		{
			Parts.Add(FString::Printf(TEXT("Upgrades to %s"), Unit.UpgradesToUnitId.IsNone() ? TEXT("nothing") : *Unit.UpgradesToUnitId.ToString()));
		}
		if (Unit.AllowedAugmentIds.Num() != BaseUnit.AllowedAugmentIds.Num())
		{
			Parts.Add(FString::Printf(TEXT("%+d allowed augments"), Unit.AllowedAugmentIds.Num() - BaseUnit.AllowedAugmentIds.Num()));
		}
		if (Unit.Modifiers.Num() != BaseUnit.Modifiers.Num())
		{
			Parts.Add(FString::Printf(TEXT("%+d modifier entries"), Unit.Modifiers.Num() - BaseUnit.Modifiers.Num()));
		}
		return Parts.Num() > 0 ? FString::Join(Parts, TEXT(", ")) : TEXT("No stat changes");
	}

	FString ConquestSetupFormatBuildingStats(const FConquestBuildingRow& Building)
	{
		TArray<FString> Parts;
		Parts.Add(FString::Printf(TEXT("Cost %d"), Building.ProductionCost));
		Parts.Add(ConquestSetupEnumValueToString(
			StaticEnum<EConquestProductionCategory>(),
			static_cast<int64>(Building.ProductionCategory)
		));
		if (!Building.RequiredTechId.IsNone())
		{
			Parts.Add(FString::Printf(TEXT("Requires %s"), *Building.RequiredTechId.ToString()));
		}
		const FString YieldText = ConquestSetupFormatYield(Building.FlatCityYieldBonus);
		if (!YieldText.IsEmpty())
		{
			Parts.Add(YieldText);
		}
		if (Building.HappinessBonus != 0)
		{
			Parts.Add(FString::Printf(TEXT("%s Happiness"), *ConquestSetupFormatSignedInt(Building.HappinessBonus)));
		}
		if (Building.StrategicResourceCapBonuses.Num() > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d resource cap bonus%s"), Building.StrategicResourceCapBonuses.Num(), Building.StrategicResourceCapBonuses.Num() == 1 ? TEXT("") : TEXT("es")));
		}
		if (Building.UnlockedTileImprovementIds.Num() > 0)
		{
			Parts.Add(FString::Printf(TEXT("Unlocks %d improvement%s"), Building.UnlockedTileImprovementIds.Num(), Building.UnlockedTileImprovementIds.Num() == 1 ? TEXT("") : TEXT("s")));
		}
		if (Building.Modifiers.Num() > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d modifier%s"), Building.Modifiers.Num(), Building.Modifiers.Num() == 1 ? TEXT("") : TEXT("s")));
		}
		if (Building.bGrantedOnCityFounding)
		{
			Parts.Add(TEXT("Granted on founding"));
		}
		if (Building.bIsWorldWonder)
		{
			Parts.Add(TEXT("World Wonder"));
		}
		if (Building.bIsNationalWonder)
		{
			Parts.Add(TEXT("National Wonder"));
		}
		if (Building.bIsProject)
		{
			Parts.Add(TEXT("Project"));
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	FString ConquestSetupFormatBuildingDiff(const FConquestBuildingRow& Building, const FConquestBuildingRow& BaseBuilding)
	{
		TArray<FString> Parts;
		if (Building.ProductionCost != BaseBuilding.ProductionCost)
		{
			Parts.Add(FString::Printf(TEXT("%s Cost"), *ConquestSetupFormatSignedInt(Building.ProductionCost - BaseBuilding.ProductionCost)));
		}

		FHexYield YieldDiff;
		YieldDiff.Food = Building.FlatCityYieldBonus.Food - BaseBuilding.FlatCityYieldBonus.Food;
		YieldDiff.Production = Building.FlatCityYieldBonus.Production - BaseBuilding.FlatCityYieldBonus.Production;
		YieldDiff.Gold = Building.FlatCityYieldBonus.Gold - BaseBuilding.FlatCityYieldBonus.Gold;
		YieldDiff.Science = Building.FlatCityYieldBonus.Science - BaseBuilding.FlatCityYieldBonus.Science;
		YieldDiff.Culture = Building.FlatCityYieldBonus.Culture - BaseBuilding.FlatCityYieldBonus.Culture;
		const FString YieldDiffText = ConquestSetupFormatYield(YieldDiff);
		if (!YieldDiffText.IsEmpty())
		{
			Parts.Add(YieldDiffText);
		}
		if (Building.HappinessBonus != BaseBuilding.HappinessBonus)
		{
			Parts.Add(FString::Printf(TEXT("%s Happiness"), *ConquestSetupFormatSignedInt(Building.HappinessBonus - BaseBuilding.HappinessBonus)));
		}
		if (Building.RequiredTechId != BaseBuilding.RequiredTechId)
		{
			Parts.Add(FString::Printf(TEXT("Requires %s"), Building.RequiredTechId.IsNone() ? TEXT("no tech") : *Building.RequiredTechId.ToString()));
		}
		if (Building.ProductionCategory != BaseBuilding.ProductionCategory)
		{
			Parts.Add(FString::Printf(
				TEXT("Category %s"),
				*ConquestSetupEnumValueToString(StaticEnum<EConquestProductionCategory>(), static_cast<int64>(Building.ProductionCategory))
			));
		}
		if (Building.Modifiers.Num() != BaseBuilding.Modifiers.Num())
		{
			Parts.Add(FString::Printf(TEXT("%+d modifier entries"), Building.Modifiers.Num() - BaseBuilding.Modifiers.Num()));
		}
		if (Building.UnlockedTileImprovementIds.Num() != BaseBuilding.UnlockedTileImprovementIds.Num())
		{
			Parts.Add(FString::Printf(TEXT("%+d unlocked improvements"), Building.UnlockedTileImprovementIds.Num() - BaseBuilding.UnlockedTileImprovementIds.Num()));
		}
		return Parts.Num() > 0 ? FString::Join(Parts, TEXT(", ")) : TEXT("No stat changes");
	}

	FString ConquestSetupGetUnitName(const FConquestUnitRow* Unit, FName FallbackId)
	{
		return Unit && !Unit->DisplayName.IsEmpty()
			? Unit->DisplayName.ToString()
			: FallbackId.ToString();
	}

	FString ConquestSetupGetBuildingName(const FConquestBuildingRow* Building, FName FallbackId)
	{
		return Building && !Building->DisplayName.IsEmpty()
			? Building->DisplayName.ToString()
			: FallbackId.ToString();
	}
}

void UConquestGameSetupWidget::InitializeGameSetupWidget(UConquestHUDWidget* InParentHUDWidget)
{
	ParentHUDWidget = InParentHUDWidget;

	RefreshMapPresetOptions();
	RefreshMapSizeOptions();
	RefreshCivilisationOptions();
	RefreshLobbySlots();
	ApplyDefaultAdvancedValues();
	if (const AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		ApplyGameSetupSettingsToControls(ConquestGS->GameSetupSettings);
	}
	UpdateMapSizeTooltip();
	RefreshHostOnlyControlState();
	RefreshCivilisationInfo(nullptr);
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

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UConquestGameSetupWidget::HandleBackButtonClicked);
		BackButton->OnClicked.AddDynamic(this, &UConquestGameSetupWidget::HandleBackButtonClicked);
	}

	if (ReadyButton)
	{
		ReadyButton->OnClicked.RemoveDynamic(this, &UConquestGameSetupWidget::HandleReadyButtonClicked);
		ReadyButton->OnClicked.AddDynamic(this, &UConquestGameSetupWidget::HandleReadyButtonClicked);
	}

	if (RandomSeedButton)
	{
		RandomSeedButton->OnClicked.RemoveDynamic(this, &UConquestGameSetupWidget::HandleRandomSeedButtonClicked);
		RandomSeedButton->OnClicked.AddDynamic(this, &UConquestGameSetupWidget::HandleRandomSeedButtonClicked);
	}

	BindCivilisationComboBoxes();
	ConfigureRandomSeedSpinBox();

	for (USpinBox* SpinBox : {
		BonusResourceDensitySpinBox.Get(),
		LuxuryResourceDensitySpinBox.Get(),
		StrategicResourceDensitySpinBox.Get(),
		BonusResourceCountSpinBox.Get(),
		LuxuryResourceCountSpinBox.Get(),
		StrategicResourceCountSpinBox.Get(),
		ResourceSpacingSpinBox.Get(),
		RiverDensitySpinBox.Get(),
		MaxRiverCountSpinBox.Get(),
		MinRiverLengthSpinBox.Get(),
		MaxRiverLengthSpinBox.Get(),
		RiverSpacingSpinBox.Get(),
		LakeFrequencySpinBox.Get(),
		LakeCountSpinBox.Get(),
		LakeSpacingSpinBox.Get(),
		LakeMinSizeSpinBox.Get(),
		LakeMaxSizeSpinBox.Get(),
		MountainAmountSpinBox.Get(),
		TemperatureBiasStrengthSpinBox.Get(),
		PolarFalloffPowerSpinBox.Get(),
		TemperatureNoiseStrengthSpinBox.Get()
	})
	{
		if (SpinBox)
		{
			SpinBox->OnValueChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleGameSetupFloatValueChanged);
			SpinBox->OnValueChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleGameSetupFloatValueChanged);
		}
	}

	for (UCheckBox* CheckBox : {
		GenerateResourcesCheckBox.Get(),
		GenerateRiversCheckBox.Get(),
		UseTemperatureBiasCheckBox.Get()
	})
	{
		if (CheckBox)
		{
			CheckBox->OnCheckStateChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleGameSetupCheckStateChanged);
			CheckBox->OnCheckStateChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleGameSetupCheckStateChanged);
		}
	}

	bApplyingGameSetupSettings = true;
	RefreshMapPresetOptions();
	RefreshMapSizeOptions();
	RefreshCivilisationOptions();
	RefreshLobbySlots();
	ApplyDefaultAdvancedValues();
	bApplyingGameSetupSettings = false;
	if (const AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		ApplyGameSetupSettingsToControls(ConquestGS->GameSetupSettings);
	}
	UpdateMapSizeTooltip();
	RefreshHostOnlyControlState();
	PushGameSetupSettingsToServerIfHost();
	RefreshReadyStatus();
	RefreshCivilisationInfo(nullptr);

	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		ConquestGS->OnConquestStateChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleConquestStateChanged);
		ConquestGS->OnConquestStateChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleConquestStateChanged);
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->OnAssignedPlayerIdChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleAssignedPlayerIdChanged);
		ConquestPC->OnAssignedPlayerIdChanged.AddDynamic(this, &UConquestGameSetupWidget::HandleAssignedPlayerIdChanged);
	}
}

void UConquestGameSetupWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UConquestGameSetupWidget::HandleBackButtonClicked);
	}

	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		ConquestGS->OnConquestStateChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleConquestStateChanged);
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->OnAssignedPlayerIdChanged.RemoveDynamic(this, &UConquestGameSetupWidget::HandleAssignedPlayerIdChanged);
	}

	Super::NativeDestruct();
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
	(void)SelectionType;

	if (const EHexMapTypePreset* FoundPreset = OptionToPreset.Find(SelectedItem))
	{
		SelectedMapPreset = *FoundPreset;

		FHexMapTypePreset Preset;
		if (FHexMapTypePresets::GetPreset(SelectedMapPreset, Preset))
		{
			const bool bWasApplyingGameSetupSettings = bApplyingGameSetupSettings;
			bApplyingGameSetupSettings = true;

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

			bApplyingGameSetupSettings = bWasApplyingGameSetupSettings;
		}
	}

	if (!bApplyingGameSetupSettings)
	{
		PushGameSetupSettingsToServerIfHost();
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

void UConquestGameSetupWidget::ApplyGameSetupSettingsToControls(const FConquestGameSetupSettings& SetupSettings)
{
	bApplyingGameSetupSettings = true;

	SelectedMapPreset = SetupSettings.MapTypePreset;
	SelectedMapSizePreset = SetupSettings.MapSizePreset;

	if (MapPresetComboBox)
	{
		for (const TPair<FString, EHexMapTypePreset>& Option : OptionToPreset)
		{
			if (Option.Value == SelectedMapPreset)
			{
				MapPresetComboBox->SetSelectedOption(Option.Key);
				break;
			}
		}
	}

	if (MapSizeComboBox)
	{
		for (const TPair<FString, EConquestMapSizePreset>& Option : OptionToMapSizePreset)
		{
			if (Option.Value == SelectedMapSizePreset)
			{
				MapSizeComboBox->SetSelectedOption(Option.Key);
				break;
			}
		}
	}

	SetSelectedRandomSeed(SetupSettings.RandomSeed, true);

	if (GenerateResourcesCheckBox)
	{
		GenerateResourcesCheckBox->SetIsChecked(SetupSettings.ResourceGenerationSettings.bGenerateResources);
	}
	if (BonusResourceDensitySpinBox)
	{
		BonusResourceDensitySpinBox->SetValue(SetupSettings.ResourceGenerationSettings.AutoBonusDensity);
	}
	if (LuxuryResourceDensitySpinBox)
	{
		LuxuryResourceDensitySpinBox->SetValue(SetupSettings.ResourceGenerationSettings.AutoLuxuryDensity);
	}
	if (StrategicResourceDensitySpinBox)
	{
		StrategicResourceDensitySpinBox->SetValue(SetupSettings.ResourceGenerationSettings.AutoStrategicDensity);
	}
	if (BonusResourceCountSpinBox)
	{
		BonusResourceCountSpinBox->SetValue(static_cast<float>(SetupSettings.ResourceGenerationSettings.BonusResourceCount));
	}
	if (LuxuryResourceCountSpinBox)
	{
		LuxuryResourceCountSpinBox->SetValue(static_cast<float>(SetupSettings.ResourceGenerationSettings.LuxuryResourceCount));
	}
	if (StrategicResourceCountSpinBox)
	{
		StrategicResourceCountSpinBox->SetValue(static_cast<float>(SetupSettings.ResourceGenerationSettings.StrategicResourceCount));
	}
	if (ResourceSpacingSpinBox)
	{
		ResourceSpacingSpinBox->SetValue(static_cast<float>(SetupSettings.ResourceGenerationSettings.ResourceMinSpacing));
	}

	if (GenerateRiversCheckBox)
	{
		GenerateRiversCheckBox->SetIsChecked(SetupSettings.RiverSettings.bGenerateRivers);
	}
	if (RiverDensitySpinBox)
	{
		RiverDensitySpinBox->SetValue(SetupSettings.RiverSettings.RiverDensityPer100Tiles);
	}
	if (MaxRiverCountSpinBox)
	{
		MaxRiverCountSpinBox->SetValue(static_cast<float>(SetupSettings.RiverSettings.MaxRiverCount));
	}
	if (MinRiverLengthSpinBox)
	{
		MinRiverLengthSpinBox->SetValue(static_cast<float>(SetupSettings.RiverSettings.MinRiverLength));
	}
	if (MaxRiverLengthSpinBox)
	{
		MaxRiverLengthSpinBox->SetValue(static_cast<float>(SetupSettings.RiverSettings.MaxRiverLength));
	}
	if (RiverSpacingSpinBox)
	{
		RiverSpacingSpinBox->SetValue(static_cast<float>(SetupSettings.RiverSettings.RiverAvoidanceRadius));
	}

	if (LakeFrequencySpinBox)
	{
		LakeFrequencySpinBox->SetValue(SetupSettings.MapShapeSettings.LakeFrequency);
	}
	if (LakeCountSpinBox)
	{
		LakeCountSpinBox->SetValue(static_cast<float>(SetupSettings.MapShapeSettings.LakeCount));
	}
	if (LakeSpacingSpinBox)
	{
		LakeSpacingSpinBox->SetValue(static_cast<float>(SetupSettings.MapShapeSettings.LakeSpacing));
	}
	if (LakeMinSizeSpinBox)
	{
		LakeMinSizeSpinBox->SetValue(static_cast<float>(SetupSettings.MapShapeSettings.LakeMinSize));
	}
	if (LakeMaxSizeSpinBox)
	{
		LakeMaxSizeSpinBox->SetValue(static_cast<float>(SetupSettings.MapShapeSettings.LakeMaxSize));
	}
	if (MountainAmountSpinBox)
	{
		MountainAmountSpinBox->SetValue(SetupSettings.MountainWeightScale);
	}

	if (UseTemperatureBiasCheckBox)
	{
		UseTemperatureBiasCheckBox->SetIsChecked(SetupSettings.TemperatureSettings.bUseTemperatureBias);
	}
	if (TemperatureBiasStrengthSpinBox)
	{
		TemperatureBiasStrengthSpinBox->SetValue(SetupSettings.TemperatureSettings.TemperatureBiasStrength);
	}
	if (PolarFalloffPowerSpinBox)
	{
		PolarFalloffPowerSpinBox->SetValue(SetupSettings.TemperatureSettings.PolarFalloffPower);
	}
	if (TemperatureNoiseStrengthSpinBox)
	{
		TemperatureNoiseStrengthSpinBox->SetValue(SetupSettings.TemperatureSettings.TemperatureNoiseStrength);
	}

	UpdateMapSizeTooltip();
	bApplyingGameSetupSettings = false;
}

bool UConquestGameSetupWidget::IsLocalPlayerLobbyHost() const
{
	const AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer());
	return ConquestPC && ConquestPC->HasAuthority() && ConquestPC->GetAssignedPlayerId() == 0;
}

void UConquestGameSetupWidget::RefreshHostOnlyControlState()
{
	const bool bIsHost = IsLocalPlayerLobbyHost();

	if (MapPresetComboBox)
	{
		MapPresetComboBox->SetIsEnabled(bIsHost);
	}
	if (MapSizeComboBox)
	{
		MapSizeComboBox->SetIsEnabled(bIsHost);
	}
	if (RandomSeedSpinBox)
	{
		RandomSeedSpinBox->SetIsEnabled(bIsHost);
	}
	if (RandomSeedButton)
	{
		RandomSeedButton->SetIsEnabled(bIsHost);
	}

	for (USpinBox* SpinBox : {
		BonusResourceDensitySpinBox.Get(),
		LuxuryResourceDensitySpinBox.Get(),
		StrategicResourceDensitySpinBox.Get(),
		BonusResourceCountSpinBox.Get(),
		LuxuryResourceCountSpinBox.Get(),
		StrategicResourceCountSpinBox.Get(),
		ResourceSpacingSpinBox.Get(),
		RiverDensitySpinBox.Get(),
		MaxRiverCountSpinBox.Get(),
		MinRiverLengthSpinBox.Get(),
		MaxRiverLengthSpinBox.Get(),
		RiverSpacingSpinBox.Get(),
		LakeFrequencySpinBox.Get(),
		LakeCountSpinBox.Get(),
		LakeSpacingSpinBox.Get(),
		LakeMinSizeSpinBox.Get(),
		LakeMaxSizeSpinBox.Get(),
		MountainAmountSpinBox.Get(),
		TemperatureBiasStrengthSpinBox.Get(),
		PolarFalloffPowerSpinBox.Get(),
		TemperatureNoiseStrengthSpinBox.Get()
	})
	{
		if (SpinBox)
		{
			SpinBox->SetIsEnabled(bIsHost);
		}
	}

	for (UCheckBox* CheckBox : {
		GenerateResourcesCheckBox.Get(),
		GenerateRiversCheckBox.Get(),
		UseTemperatureBiasCheckBox.Get()
	})
	{
		if (CheckBox)
		{
			CheckBox->SetIsEnabled(bIsHost);
		}
	}
}

void UConquestGameSetupWidget::PushGameSetupSettingsToServerIfHost()
{
	if (!IsLocalPlayerLobbyHost())
	{
		return;
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestSetGameSetupSettings(GetSelectedGameSetupSettings());
	}
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
	PushGameSetupSettingsToServerIfHost();
}

void UConquestGameSetupWidget::HandleRandomSeedValueChanged(float NewValue)
{
	if (bUpdatingRandomSeedSpinBox || bApplyingGameSetupSettings)
	{
		return;
	}

	SelectedRandomSeed = FMath::Clamp(
		FMath::RoundToInt(NewValue),
		1,
		2147483646
	);
	PushGameSetupSettingsToServerIfHost();
}

void UConquestGameSetupWidget::HandlePlayButtonClicked()
{
	const AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (
		!IsLocalPlayerLobbyHost() ||
		!ConquestGS ||
		ConquestGS->GetRequiredReadyHumanPlayerCount() <= 0 ||
		ConquestGS->GetReadyHumanPlayerCount() < ConquestGS->GetRequiredReadyHumanPlayerCount()
	)
	{
		return;
	}

	if (ParentHUDWidget)
	{
		ParentHUDWidget->RequestStartGame(GetSelectedGameSetupSettings());
	}
}

void UConquestGameSetupWidget::HandleBackButtonClicked()
{
	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestLeaveGameSetup();
	}
}

void UConquestGameSetupWidget::HandleMapSizeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	(void)SelectionType;

	if (const EConquestMapSizePreset* FoundPreset = OptionToMapSizePreset.Find(SelectedItem))
	{
		SelectedMapSizePreset = *FoundPreset;
		UpdateMapSizeTooltip();
		if (IsLocalPlayerLobbyHost())
		{
			bForceRebuildLobbySlots = true;
			RefreshLobbySlots();
		}
	}

	if (!bApplyingGameSetupSettings)
	{
		PushGameSetupSettingsToServerIfHost();
	}
}

void UConquestGameSetupWidget::HandleGameSetupFloatValueChanged(float NewValue)
{
	(void)NewValue;

	if (!bApplyingGameSetupSettings)
	{
		PushGameSetupSettingsToServerIfHost();
	}
}

void UConquestGameSetupWidget::HandleGameSetupCheckStateChanged(bool bIsChecked)
{
	(void)bIsChecked;

	if (!bApplyingGameSetupSettings)
	{
		PushGameSetupSettingsToServerIfHost();
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
	if (!bForceRebuildLobbySlots)
	{
		if (const AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
		{
			if (ConquestGS->LobbyPlayerSlots.Num() > 0)
			{
				LobbyPlayerSlots = ConquestGS->LobbyPlayerSlots;
				const TArray<UComboBoxString*> ComboBoxes = GetCivilisationComboBoxes();
				for (int32 SlotIndex = 0; SlotIndex < ComboBoxes.Num(); ++SlotIndex)
				{
					RefreshCivilisationComboBox(ComboBoxes[SlotIndex], SlotIndex);
				}
				OnLobbySlotsChanged(LobbyPlayerSlots);
				RefreshReadyStatus();
				RefreshCivilisationInfo(PreviewCivilisation);
				return;
			}
		}
	}
	bForceRebuildLobbySlots = false;

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
		LobbySlot.PlayerId = INDEX_NONE;
		LobbySlot.SlotType = EConquestLobbySlotType::AI;
		LobbySlot.bIsHost = SlotIndex == 0;

		if (const FConquestLobbyPlayerSlot* PreviousSlot = PreviousSlots.FindByPredicate(
			[SlotIndex](const FConquestLobbyPlayerSlot& Candidate)
			{
				return Candidate.SlotIndex == SlotIndex;
			}))
		{
			LobbySlot.Civilisation = PreviousSlot->Civilisation;
			LobbySlot.PlayerId = PreviousSlot->PlayerId;
			LobbySlot.SlotType = PreviousSlot->SlotType;
			LobbySlot.bIsHost = PreviousSlot->bIsHost;
			LobbySlot.PlayerName = PreviousSlot->PlayerName;
			LobbySlot.bIsReady = PreviousSlot->bIsReady;
		}

		if (SlotIndex == 0 && LobbySlot.PlayerId == INDEX_NONE)
		{
			LobbySlot.PlayerId = 0;
			LobbySlot.SlotType = EConquestLobbySlotType::Human;
			LobbySlot.bIsHost = true;
			LobbySlot.PlayerName = TEXT("Host");
		}

		LobbyPlayerSlots.Add(LobbySlot);
	}

	const TArray<UComboBoxString*> ComboBoxes = GetCivilisationComboBoxes();
	for (int32 SlotIndex = 0; SlotIndex < ComboBoxes.Num(); ++SlotIndex)
	{
		RefreshCivilisationComboBox(ComboBoxes[SlotIndex], SlotIndex);
	}

	OnLobbySlotsChanged(LobbyPlayerSlots);
	PushLobbySlotsToServerIfAuthority();
	RefreshReadyStatus();
	RefreshCivilisationInfo(PreviewCivilisation);
}

void UConquestGameSetupWidget::RefreshCivilisationOptions()
{
	OptionToCivilisation.Reset();
	AvailableCivilisations.Reset();

	if (UWorld* World = GetWorld())
	{
		if (const AConquestGameState* ConquestGS = World->GetGameState<AConquestGameState>();
			ConquestGS && ConquestGS->AvailableCivilisations.Num() > 0)
		{
			for (UConquestCivilisationData* Civilisation : ConquestGS->AvailableCivilisations)
			{
				if (Civilisation)
				{
					AvailableCivilisations.Add(Civilisation);
				}
			}
		}
		else if (const AConquestGameMode* ConquestGameMode = World->GetAuthGameMode<AConquestGameMode>())
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
	if (
		PreviewCivilisation &&
		!AvailableCivilisations.ContainsByPredicate([this](const TObjectPtr<UConquestCivilisationData>& Civilisation)
		{
			return Civilisation.Get() == PreviewCivilisation.Get();
		})
	)
	{
		PreviewCivilisation = nullptr;
	}
	RefreshCivilisationInfo(PreviewCivilisation);
}

bool UConquestGameSetupWidget::SetLobbySlotCivilisation(int32 SlotIndex, UConquestCivilisationData* Civilisation)
{
	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	LobbyPlayerSlots[SlotIndex].Civilisation = Civilisation;
	PreviewCivilisation = Civilisation;
	RefreshCivilisationInfo(Civilisation);
	const TArray<UComboBoxString*> ComboBoxes = GetCivilisationComboBoxes();
	RefreshCivilisationComboBox(ComboBoxes.IsValidIndex(SlotIndex) ? ComboBoxes[SlotIndex] : nullptr, SlotIndex);
	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestSetLobbySlotCivilisation(SlotIndex, Civilisation);
	}
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

FText UConquestGameSetupWidget::GetLobbySlotDisplayText(int32 SlotIndex) const
{
	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return FText::GetEmpty();
	}

	const FConquestLobbyPlayerSlot& LobbySlot = LobbyPlayerSlots[SlotIndex];
	FString PlayerLabel;
	if (LobbySlot.SlotType == EConquestLobbySlotType::Human)
	{
		PlayerLabel = LobbySlot.PlayerName.IsEmpty()
			? FString::Printf(TEXT("Player %d"), LobbySlot.PlayerId + 1)
			: LobbySlot.PlayerName;
		if (LobbySlot.bIsHost)
		{
			PlayerLabel += TEXT(" [HOST]");
		}
	}
	else if (LobbySlot.SlotType == EConquestLobbySlotType::AI)
	{
		PlayerLabel = TEXT("AI");
	}
	else
	{
		PlayerLabel = TEXT("Closed");
	}

	const FString CivilisationLabel = LobbySlot.Civilisation
		? (
			LobbySlot.Civilisation->CivilisationName.IsEmpty()
				? LobbySlot.Civilisation->GetName()
				: LobbySlot.Civilisation->CivilisationName.ToString()
		)
		: TEXT("No civilisation");

	return FText::FromString(FString::Printf(TEXT("%s - %s"), *PlayerLabel, *CivilisationLabel));
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

	bUpdatingCivilisationComboBoxes = true;
	ComboBox->ClearOptions();
	ComboBox->SetIsEnabled(LobbyPlayerSlots.IsValidIndex(SlotIndex) && CanLocalPlayerEditSlot(SlotIndex));

	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		bUpdatingCivilisationComboBoxes = false;
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
				bUpdatingCivilisationComboBoxes = false;
				return;
			}
		}
	}

	bUpdatingCivilisationComboBoxes = false;
}

void UConquestGameSetupWidget::HandleCivilisationSelectionChanged(int32 SlotIndex, const FString& SelectedItem)
{
	if (bUpdatingCivilisationComboBoxes)
	{
		return;
	}

	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	if (TObjectPtr<UConquestCivilisationData>* Civilisation = OptionToCivilisation.Find(SelectedItem))
	{
		LobbyPlayerSlots[SlotIndex].Civilisation = Civilisation->Get();
		PreviewCivilisation = Civilisation->Get();
		RefreshCivilisationInfo(Civilisation->Get());
		if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
		{
			ConquestPC->RequestSetLobbySlotCivilisation(SlotIndex, Civilisation->Get());
		}
	}
}

const UConquestCivilisationData* UConquestGameSetupWidget::GetDefaultPreviewCivilisation() const
{
	return nullptr;
}

void UConquestGameSetupWidget::RefreshCivilisationInfo(const UConquestCivilisationData* Civilisation)
{
	if (!Civilisation)
	{
		if (CivInfoNameText)
		{
			CivInfoNameText->SetText(FText::GetEmpty());
		}
		if (CivInfoLeaderText)
		{
			CivInfoLeaderText->SetText(FText::GetEmpty());
		}
		if (CivInfoAbilityText)
		{
			CivInfoAbilityText->SetText(FText::GetEmpty());
		}
		if (CivInfoUniqueUnitsText)
		{
			CivInfoUniqueUnitsText->SetText(FText::GetEmpty());
		}
		if (CivInfoUniqueBuildingsText)
		{
			CivInfoUniqueBuildingsText->SetText(FText::GetEmpty());
		}
		if (CivInfoText)
		{
			CivInfoText->SetText(FText::GetEmpty());
		}
		return;
	}

	const FString CivName = Civilisation
		? (Civilisation->CivilisationName.IsEmpty() ? Civilisation->GetName() : Civilisation->CivilisationName.ToString())
		: TEXT("No civilisation selected");
	const FString LeaderName = Civilisation && !Civilisation->LeaderName.IsEmpty()
		? Civilisation->LeaderName.ToString()
		: TEXT("Unknown leader");

	TArray<FString> AbilityLines;
	TArray<FString> UnitLines;
	TArray<FString> BuildingLines;

	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	const UConquestContentManager* ContentManager = ConquestGS ? ConquestGS->ContentManager : nullptr;

	if (Civilisation)
	{
		if (!Civilisation->AbilityDescription.IsEmpty())
		{
			AbilityLines.Add(Civilisation->AbilityDescription.ToString());
		}

		for (const FConquestModifierDefinition& Modifier : Civilisation->Modifiers)
		{
			ConquestSetupAppendLine(AbilityLines, ConquestSetupFormatModifier(Modifier));
		}

		for (const FConquestContentOverride& Override : Civilisation->UnitOverrides)
		{
			const FName ReplacementId = !Override.ReplacementId.IsNone() ? Override.ReplacementId : NAME_None;
			const FName BaseId = !Override.ReplacesId.IsNone() ? Override.ReplacesId : NAME_None;
			const FConquestUnitRow* Unit = ContentManager ? ContentManager->FindUnit(ReplacementId) : nullptr;
			const FName RowReplacementId = Unit && !Unit->ReplacesUnitId.IsNone() ? Unit->ReplacesUnitId : BaseId;
			const FConquestUnitRow* BaseUnit = ContentManager ? ContentManager->FindUnit(RowReplacementId) : nullptr;

			FString Line = FString::Printf(
				TEXT("%s - %s"),
				*ConquestSetupGetUnitName(Unit, ReplacementId),
				Unit ? *ConquestSetupFormatUnitStats(*Unit) : TEXT("Missing unit row")
			);
			if (Unit && BaseUnit)
			{
				Line += FString::Printf(
					TEXT("\n  Replaces - %s (%s)"),
					*ConquestSetupGetUnitName(BaseUnit, RowReplacementId),
					*ConquestSetupFormatUnitDiff(*Unit, *BaseUnit)
				);
			}
			else if (!RowReplacementId.IsNone())
			{
				Line += FString::Printf(TEXT("\n  Replaces - %s"), *RowReplacementId.ToString());
			}
			if (Unit && Unit->Modifiers.Num() > 0)
			{
				TArray<FString> ModifierLines;
				for (const FConquestModifierDefinition& Modifier : Unit->Modifiers)
				{
					ModifierLines.Add(ConquestSetupFormatModifier(Modifier));
				}
				Line += FString::Printf(TEXT("\n  Bonuses - %s"), *FString::Join(ModifierLines, TEXT("; ")));
			}
			UnitLines.Add(Line);
		}

		for (const FConquestContentOverride& Override : Civilisation->BuildingOverrides)
		{
			const FName ReplacementId = !Override.ReplacementId.IsNone() ? Override.ReplacementId : NAME_None;
			const FName BaseId = !Override.ReplacesId.IsNone() ? Override.ReplacesId : NAME_None;
			const FConquestBuildingRow* Building = ContentManager ? ContentManager->FindBuilding(ReplacementId) : nullptr;
			const FName RowReplacementId = Building && !Building->ReplacesBuildingId.IsNone() ? Building->ReplacesBuildingId : BaseId;
			const FConquestBuildingRow* BaseBuilding = ContentManager ? ContentManager->FindBuilding(RowReplacementId) : nullptr;

			FString Line = FString::Printf(
				TEXT("%s - %s"),
				*ConquestSetupGetBuildingName(Building, ReplacementId),
				Building ? *ConquestSetupFormatBuildingStats(*Building) : TEXT("Missing building row")
			);
			if (Building && BaseBuilding)
			{
				Line += FString::Printf(
					TEXT("\n  Replaces - %s (%s)"),
					*ConquestSetupGetBuildingName(BaseBuilding, RowReplacementId),
					*ConquestSetupFormatBuildingDiff(*Building, *BaseBuilding)
				);
			}
			else if (!RowReplacementId.IsNone())
			{
				Line += FString::Printf(TEXT("\n  Replaces - %s"), *RowReplacementId.ToString());
			}
			if (Building && Building->Modifiers.Num() > 0)
			{
				TArray<FString> ModifierLines;
				for (const FConquestModifierDefinition& Modifier : Building->Modifiers)
				{
					ModifierLines.Add(ConquestSetupFormatModifier(Modifier));
				}
				Line += FString::Printf(TEXT("\n  Bonuses - %s"), *FString::Join(ModifierLines, TEXT("; ")));
			}
			BuildingLines.Add(Line);
		}
	}

	const FString AbilityText = AbilityLines.Num() > 0 ? FString::Join(AbilityLines, TEXT("\n")) : TEXT("No listed civilisation ability");
	const FString UnitsText = UnitLines.Num() > 0 ? FString::Join(UnitLines, TEXT("\n\n")) : TEXT("No unique units");
	const FString BuildingsText = BuildingLines.Num() > 0 ? FString::Join(BuildingLines, TEXT("\n\n")) : TEXT("No unique buildings");

	if (CivInfoNameText)
	{
		CivInfoNameText->SetText(FText::FromString(CivName));
	}
	if (CivInfoLeaderText)
	{
		CivInfoLeaderText->SetText(FText::FromString(LeaderName));
	}
	if (CivInfoAbilityText)
	{
		CivInfoAbilityText->SetText(FText::FromString(AbilityText));
	}
	if (CivInfoUniqueUnitsText)
	{
		CivInfoUniqueUnitsText->SetText(FText::FromString(UnitsText));
	}
	if (CivInfoUniqueBuildingsText)
	{
		CivInfoUniqueBuildingsText->SetText(FText::FromString(BuildingsText));
	}
	if (CivInfoText)
	{
		CivInfoText->SetText(FText::FromString(FString::Printf(
			TEXT("%s\n%s\n\n%s\n\nUnique Units\n%s\n\nUnique Buildings\n%s"),
			*CivName,
			*LeaderName,
			*AbilityText,
			*UnitsText,
			*BuildingsText
		)));
	}
}

bool UConquestGameSetupWidget::CanLocalPlayerEditSlot(int32 SlotIndex) const
{
	if (!LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer());
	const int32 LocalPlayerId = ConquestPC ? ConquestPC->GetAssignedPlayerId() : 0;
	const FConquestLobbyPlayerSlot& LobbySlot = LobbyPlayerSlots[SlotIndex];
	const bool bLocalPlayerIsHost = LocalPlayerId == 0;

	return
		(LobbySlot.SlotType == EConquestLobbySlotType::Human && LobbySlot.PlayerId == LocalPlayerId) ||
		(
			bLocalPlayerIsHost &&
			(
				LobbySlot.SlotType != EConquestLobbySlotType::Human ||
				LobbySlot.PlayerId == LocalPlayerId
			)
		);
}

void UConquestGameSetupWidget::PushLobbySlotsToServerIfAuthority()
{
	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		if (ConquestGS->HasAuthority())
		{
			ConquestGS->SetLobbyPlayerSlots(LobbyPlayerSlots);
		}
	}
}

void UConquestGameSetupWidget::RefreshReadyStatus()
{
	const AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	const int32 ReadyCount = ConquestGS ? ConquestGS->GetReadyHumanPlayerCount() : 0;
	const int32 RequiredCount = ConquestGS ? ConquestGS->GetRequiredReadyHumanPlayerCount() : 0;

	const FText ReadyText = FText::Format(
		NSLOCTEXT("Conquest", "LobbyReadyStatusFormat", "Ready {0}/{1}"),
		FText::AsNumber(ReadyCount),
		FText::AsNumber(RequiredCount)
	);

	if (ReadyStatusText)
	{
		ReadyStatusText->SetText(ReadyText);
	}

	if (ReadyButtonText)
	{
		ReadyButtonText->SetText(ReadyText);
	}

	if (ReadyButton)
	{
		const AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer());
		const int32 LocalPlayerId = ConquestPC ? ConquestPC->GetAssignedPlayerId() : INDEX_NONE;
		const FConquestLobbyPlayerSlot* LocalSlot = LobbyPlayerSlots.FindByPredicate(
			[LocalPlayerId](const FConquestLobbyPlayerSlot& CandidateSlot)
			{
				return CandidateSlot.SlotType == EConquestLobbySlotType::Human && CandidateSlot.PlayerId == LocalPlayerId;
			}
		);

		ReadyButton->SetIsEnabled(LocalSlot && LocalSlot->Civilisation && !LocalSlot->bIsReady);
	}

	if (PlayButton)
	{
		PlayButton->SetIsEnabled(
			IsLocalPlayerLobbyHost() &&
			RequiredCount > 0 &&
			ReadyCount >= RequiredCount
		);
	}
}

void UConquestGameSetupWidget::HandleReadyButtonClicked()
{
	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestSetLobbyReady(true);
	}
}

void UConquestGameSetupWidget::HandleConquestStateChanged()
{
	if (const AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		if (
			ConquestGS->TurnManager &&
			ConquestGS->TurnManager->CurrentPhase != EConquestTurnPhase::None &&
			ConquestGS->TurnManager->CurrentPhase != EConquestTurnPhase::GameSetup
		)
		{
			if (ParentHUDWidget)
			{
				ParentHUDWidget->ShowGame();
			}
			return;
		}

		ApplyGameSetupSettingsToControls(ConquestGS->GameSetupSettings);
	}

	RefreshHostOnlyControlState();
	RefreshCivilisationOptions();
	RefreshLobbySlots();
	RefreshReadyStatus();
}

void UConquestGameSetupWidget::HandleAssignedPlayerIdChanged(int32 NewPlayerId)
{
	(void)NewPlayerId;

	RefreshCivilisationOptions();
	RefreshLobbySlots();
	RefreshHostOnlyControlState();
	RefreshReadyStatus();
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
