#include "Conquest/UI/ConquestCityPanelWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"

#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/UI/ConquestChoiceButtonWidget.h"
#include "Conquest/UI/ConquestHUD.h"
#include "Conquest/Units/ConquestUnitTypes.h"

void UConquestCityPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindToGameState();

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UConquestCityPanelWidget::HandleCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &UConquestCityPanelWidget::HandleCloseClicked);
	}

	if (UnitsButton)
	{
		UnitsButton->OnClicked.RemoveDynamic(this, &UConquestCityPanelWidget::HandleUnitsButtonClicked);
		UnitsButton->OnClicked.AddDynamic(this, &UConquestCityPanelWidget::HandleUnitsButtonClicked);
	}

	if (BuildingsButton)
	{
		BuildingsButton->OnClicked.RemoveDynamic(this, &UConquestCityPanelWidget::HandleBuildingsButtonClicked);
		BuildingsButton->OnClicked.AddDynamic(this, &UConquestCityPanelWidget::HandleBuildingsButtonClicked);
	}

	Refresh();
}

void UConquestCityPanelWidget::NativeDestruct()
{
	UnbindFromGameState();
	Super::NativeDestruct();
}

void UConquestCityPanelWidget::SetCity(int32 InCityId)
{
	BindToGameState();
	CityId = InCityId;
	Refresh();
}

void UConquestCityPanelWidget::Refresh()
{
	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS || !GS->CityManager)
	{
		return;
	}

	const FCityState* City = GS->CityManager->GetCity(CityId);
	if (!City)
	{
		return;
	}

	if (CityNameText)
	{
		CityNameText->SetText(FText::FromName(City->CityName));
	}

	ApplyCivilisationTheme(*City, *GS);

	if (PopulationText)
	{
		const int32 TurnsToGrowth = GS->CityManager->EstimateTurnsToGrowth(CityId);
		const FText GrowthText = TurnsToGrowth == INDEX_NONE
			? NSLOCTEXT("Conquest", "CityGrowthStalled", "Growth: Stalled")
			: FText::Format(
				NSLOCTEXT("Conquest", "CityGrowthTurns", "Growth: {0} turns"),
				FText::AsNumber(TurnsToGrowth)
			);

		PopulationText->SetText(FText::Format(
			NSLOCTEXT("Conquest", "PopulationFormat", "Population: {0} | Food: {1}/{2} | {3} | Expansions: {4}"),
			FText::AsNumber(City->Population),
			FText::AsNumber(FMath::FloorToInt(City->FoodStored)),
			FText::AsNumber(City->CachedFoodRequiredForNextPopulation),
			GrowthText,
			FText::AsNumber(City->PendingBorderExpansions)
		));
	}

	if (YieldText)
	{
		YieldText->SetText(FText::FromString(City->CachedYieldPerTurn.ToCompactString()));
	}

	if (CurrentProductionText)
	{
		FText ProductionText =
			NSLOCTEXT("Conquest", "NoProduction", "Production: Choose something");

		if (City->CurrentProduction.IsValid())
		{
			const FConquestBuildingRow* BuildingRow = nullptr;
			const FConquestUnitRow* UnitRow = nullptr;

			if (
				City->CurrentProduction.Type == ECityProductionType::Building &&
				GS->ContentManager
			)
			{
				BuildingRow = GS->ContentManager->FindBuilding(
					City->CurrentProduction.ProductionId
				);
			}
			else if (
				City->CurrentProduction.Type == ECityProductionType::Unit &&
				GS->ContentManager
			)
			{
				UnitRow = GS->ContentManager->FindUnit(
					City->CurrentProduction.ProductionId
				);
			}

			if (BuildingRow)
			{
				ProductionText = FText::Format(
					NSLOCTEXT("Conquest", "ProductionFormat", "Building: {0} ({1}/{2})"),
					BuildingRow->DisplayName,
					FText::AsNumber(FMath::FloorToInt(City->CurrentProduction.Progress)),
					FText::AsNumber(FMath::FloorToInt(City->CurrentProduction.Cost))
				);
			}
			else if (UnitRow)
			{
				ProductionText = FText::Format(
					NSLOCTEXT("Conquest", "UnitProductionFormat", "Unit: {0} ({1}/{2})"),
					UnitRow->DisplayName,
					FText::AsNumber(FMath::FloorToInt(City->CurrentProduction.Progress)),
					FText::AsNumber(FMath::FloorToInt(City->CurrentProduction.Cost))
				);
			}
			else if (City->CurrentProduction.Type == ECityProductionType::Project)
			{
				ProductionText = FText::Format(
					NSLOCTEXT("Conquest", "ProjectProductionFormat", "Focus: {0}"),
					FText::FromName(City->CurrentProduction.ProductionId)
				);
			}
			else
			{
				ProductionText = FText::Format(
					NSLOCTEXT("Conquest", "UnknownProductionFormat", "Production: {0} ({1}/{2})"),
					FText::FromName(City->CurrentProduction.ProductionId),
					FText::AsNumber(FMath::FloorToInt(City->CurrentProduction.Progress)),
					FText::AsNumber(FMath::FloorToInt(City->CurrentProduction.Cost))
				);
			}
		}

		CurrentProductionText->SetText(ProductionText);
	}

	BuildBuildingButtons();
}

void UConquestCityPanelWidget::ApplyCivilisationTheme(const FCityState& City, const AConquestGameState& GS)
{
	UMaterialInterface* CivilisationThemeMaterial = nullptr;
	FLinearColor CivilisationThemeColor = FLinearColor::White;

	if (GS.HumanPlayer.PlayerId == City.OwnerPlayerId && GS.HumanCivilisation)
	{
		CivilisationThemeMaterial = GS.HumanCivilisation->CityLabelMaterial
			? GS.HumanCivilisation->CityLabelMaterial
			: GS.HumanCivilisation->BorderMaterial;
		CivilisationThemeColor = GS.HumanCivilisation->ThemeColor;
	}

	auto ApplyThemeToImage = [CivilisationThemeMaterial, CivilisationThemeColor](UImage* Image)
	{
		if (!Image)
		{
			return;
		}

		if (CivilisationThemeMaterial)
		{
			Image->SetBrushFromMaterial(CivilisationThemeMaterial);
		}

		Image->SetColorAndOpacity(CivilisationThemeColor);
	};

	ApplyThemeToImage(CityNameThemeMaterialImage);
	ApplyThemeToImage(ThemeMaterialImage);

	OnCivilisationThemeChanged(CivilisationThemeMaterial, CivilisationThemeColor);
}

void UConquestCityPanelWidget::ClosePanel()
{
	CityId = INDEX_NONE;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UConquestCityPanelWidget::SetProductionPanelTab(EConquestCityProductionPanelTab NewTab)
{
	ActiveProductionPanelTab = NewTab;
	BuildBuildingButtons();
}

void UConquestCityPanelWidget::BindToGameState()
{
	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS)
	{
		return;
	}

	GS->OnConquestStateChanged.RemoveDynamic(this, &UConquestCityPanelWidget::HandleConquestStateChanged);
	GS->OnConquestStateChanged.AddDynamic(this, &UConquestCityPanelWidget::HandleConquestStateChanged);

	if (GS->CityManager)
	{
		GS->CityManager->OnCityChanged.RemoveDynamic(this, &UConquestCityPanelWidget::HandleCityChanged);
		GS->CityManager->OnCityChanged.AddDynamic(this, &UConquestCityPanelWidget::HandleCityChanged);
		GS->CityManager->OnCityNeedsBorderExpansion.RemoveDynamic(this, &UConquestCityPanelWidget::HandleCityChanged);
		GS->CityManager->OnCityNeedsBorderExpansion.AddDynamic(this, &UConquestCityPanelWidget::HandleCityChanged);
	}
}

void UConquestCityPanelWidget::UnbindFromGameState()
{
	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS)
	{
		return;
	}

	GS->OnConquestStateChanged.RemoveDynamic(this, &UConquestCityPanelWidget::HandleConquestStateChanged);

	if (GS->CityManager)
	{
		GS->CityManager->OnCityChanged.RemoveDynamic(this, &UConquestCityPanelWidget::HandleCityChanged);
		GS->CityManager->OnCityNeedsBorderExpansion.RemoveDynamic(this, &UConquestCityPanelWidget::HandleCityChanged);
	}
}

void UConquestCityPanelWidget::HandleCityChanged(int32 ChangedCityId)
{
	if (CityId == INDEX_NONE || ChangedCityId != CityId || !IsVisible())
	{
		return;
	}

	Refresh();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PlayerController->GetHUD()))
		{
			ConquestHUD->BeginCityTileExpansionSelection(CityId);
		}
	}
}

void UConquestCityPanelWidget::HandleConquestStateChanged()
{
	if (CityId == INDEX_NONE || !IsVisible())
	{
		return;
	}

	Refresh();
}

void UConquestCityPanelWidget::HandleCloseClicked()
{
	APlayerController* PlayerController = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PlayerController
		? Cast<AConquestHUD>(PlayerController->GetHUD())
		: nullptr;

	if (ConquestHUD)
	{
		ConquestHUD->HideCityPanel();
		return;
	}

	ClosePanel();
}

void UConquestCityPanelWidget::HandleUnitsButtonClicked()
{
	SetProductionPanelTab(EConquestCityProductionPanelTab::Units);
}

void UConquestCityPanelWidget::HandleBuildingsButtonClicked()
{
	SetProductionPanelTab(EConquestCityProductionPanelTab::Buildings);
}

void UConquestCityPanelWidget::BuildBuildingButtons()
{
	if (!BuildingButtonBox)
	{
		return;
	}

	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS || !GS->CityManager || !GS->ContentManager)
	{
		return;
	}

	BuildingButtonBox->ClearChildren();

	if (!ChoiceButtonWidgetClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("ConquestCityPanelWidget: ChoiceButtonWidgetClass is not set.")
		);

		return;
	}

	if (ActiveProductionPanelTab == EConquestCityProductionPanelTab::Buildings)
	{
		BuildBuildingChoices(*GS);
		BuildProductionProjectChoices(*GS);
	}
	else
	{
		BuildUnitChoices(*GS);
	}
}

void UConquestCityPanelWidget::BuildBuildingChoices(AConquestGameState& GS)
{
	if (!BuildingButtonBox || !GS.CityManager || !GS.ContentManager || !ChoiceButtonWidgetClass)
	{
		return;
	}

	const TArray<FName> AvailableBuildingIds =
		GS.CityManager->GetAvailableProductionBuildingIdsForCity(CityId);

	for (const FName BuildingId : AvailableBuildingIds)
	{
		if (BuildingId.IsNone())
		{
			continue;
		}

		const FConquestBuildingRow* BuildingRow =
			GS.ContentManager->FindBuilding(BuildingId);

		if (!BuildingRow)
		{
			continue;
		}

		UConquestChoiceButtonWidget* ChoiceButton =
			CreateWidget<UConquestChoiceButtonWidget>(
				GetOwningPlayer(),
				ChoiceButtonWidgetClass
			);

		if (!ChoiceButton)
		{
			continue;
		}

		const int32 Turns =
			GS.CityManager->EstimateTurnsToBuildById(CityId, BuildingId);

		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::ProductionBuilding;
		ChoiceData.ChoiceId = BuildingId;
		ChoiceData.Payload = nullptr;
		ChoiceData.Title = BuildingRow->DisplayName;
		ChoiceData.Cost = BuildingRow->ProductionCost;
		ChoiceData.Turns = Turns;
		ChoiceData.bEnabled = true;
		ChoiceData.DetailText = BuildingRow->Description;

		if (Turns == INDEX_NONE)
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "ProductionChoiceNoTurns", "{0} Production"),
				FText::AsNumber(BuildingRow->ProductionCost)
			);
		}
		else
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "ProductionChoiceTurns", "{0} Production | {1} Turns"),
				FText::AsNumber(BuildingRow->ProductionCost),
				FText::AsNumber(Turns)
			);
		}

		ChoiceButton->SetupChoiceButton(ChoiceData);

		ChoiceButton->OnChoiceClicked.RemoveDynamic(
			this,
			&UConquestCityPanelWidget::HandleProductionChoiceClicked
		);

		ChoiceButton->OnChoiceClicked.AddDynamic(
			this,
			&UConquestCityPanelWidget::HandleProductionChoiceClicked
		);

		BuildingButtonBox->AddChild(ChoiceButton);
	}
}

void UConquestCityPanelWidget::BuildUnitChoices(AConquestGameState& GS)
{
	if (!BuildingButtonBox || !GS.CityManager || !GS.ContentManager || !ChoiceButtonWidgetClass)
	{
		return;
	}

	const TArray<FName> AvailableUnitIds =
		GS.CityManager->GetAvailableProductionUnitIdsForCity(CityId);

	for (const FName UnitId : AvailableUnitIds)
	{
		if (UnitId.IsNone())
		{
			continue;
		}

		const FConquestUnitRow* UnitRow = GS.ContentManager->FindUnit(UnitId);
		if (!UnitRow)
		{
			continue;
		}

		UConquestChoiceButtonWidget* ChoiceButton =
			CreateWidget<UConquestChoiceButtonWidget>(
				GetOwningPlayer(),
				ChoiceButtonWidgetClass
			);

		if (!ChoiceButton)
		{
			continue;
		}

		const int32 Turns =
			GS.CityManager->EstimateTurnsToTrainUnitById(CityId, UnitId);

		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::ProductionUnit;
		ChoiceData.ChoiceId = UnitId;
		ChoiceData.Payload = nullptr;
		ChoiceData.Title = UnitRow->DisplayName;
		ChoiceData.Cost = UnitRow->ProductionCost;
		ChoiceData.Turns = Turns;
		ChoiceData.bEnabled = true;
		ChoiceData.DetailText = UnitRow->Description;

		if (Turns == INDEX_NONE)
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "UnitProductionChoiceNoTurns", "Unit | {0} Production"),
				FText::AsNumber(UnitRow->ProductionCost)
			);
		}
		else
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "UnitProductionChoiceTurns", "Unit | {0} Production | {1} Turns"),
				FText::AsNumber(UnitRow->ProductionCost),
				FText::AsNumber(Turns)
			);
		}

		ChoiceButton->SetupChoiceButton(ChoiceData);

		ChoiceButton->OnChoiceClicked.RemoveDynamic(
			this,
			&UConquestCityPanelWidget::HandleProductionChoiceClicked
		);

		ChoiceButton->OnChoiceClicked.AddDynamic(
			this,
			&UConquestCityPanelWidget::HandleProductionChoiceClicked
		);

		BuildingButtonBox->AddChild(ChoiceButton);
	}
}

void UConquestCityPanelWidget::BuildProductionProjectChoices(AConquestGameState& GS)
{
	if (!BuildingButtonBox || !GS.CityManager || !ChoiceButtonWidgetClass)
	{
		return;
	}

	const TArray<FName> AvailableProjectIds =
		GS.CityManager->GetAvailableProductionProjectIdsForCity(CityId);

	for (const FName ProjectId : AvailableProjectIds)
	{
		if (ProjectId.IsNone())
		{
			continue;
		}

		UConquestChoiceButtonWidget* ChoiceButton =
			CreateWidget<UConquestChoiceButtonWidget>(
				GetOwningPlayer(),
				ChoiceButtonWidgetClass
			);

		if (!ChoiceButton)
		{
			continue;
		}

		FText Title = FText::FromName(ProjectId);
		FText DetailText = NSLOCTEXT("Conquest", "ProductionFocusDetail", "Converts this city's production into a focused yield each turn.");
		if (ProjectId == FName(TEXT("FoodFocus")))
		{
			Title = NSLOCTEXT("Conquest", "ProductionFocusFood", "Food Focus");
		}
		else if (ProjectId == FName(TEXT("GoldFocus")))
		{
			Title = NSLOCTEXT("Conquest", "ProductionFocusGold", "Gold Focus");
		}
		else if (ProjectId == FName(TEXT("CultureFocus")))
		{
			Title = NSLOCTEXT("Conquest", "ProductionFocusCulture", "Culture Focus");
		}
		else if (ProjectId == FName(TEXT("ScienceFocus")))
		{
			Title = NSLOCTEXT("Conquest", "ProductionFocusScience", "Science Focus");
		}

		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::ProductionProject;
		ChoiceData.ChoiceId = ProjectId;
		ChoiceData.Title = Title;
		ChoiceData.Cost = 0;
		ChoiceData.Turns = INDEX_NONE;
		ChoiceData.bEnabled = true;
		ChoiceData.DetailText = DetailText;
		ChoiceData.Subtitle = NSLOCTEXT("Conquest", "ProductionFocusSubtitle", "City focus | Never completes");

		ChoiceButton->SetupChoiceButton(ChoiceData);
		ChoiceButton->OnChoiceClicked.RemoveDynamic(
			this,
			&UConquestCityPanelWidget::HandleProductionChoiceClicked
		);
		ChoiceButton->OnChoiceClicked.AddDynamic(
			this,
			&UConquestCityPanelWidget::HandleProductionChoiceClicked
		);

		BuildingButtonBox->AddChild(ChoiceButton);
	}
}

void UConquestCityPanelWidget::HandleProductionChoiceClicked(
	const FConquestChoiceButtonData& ChoiceData
)
{
	if (
		ChoiceData.ChoiceType != EConquestChoiceType::ProductionBuilding &&
		ChoiceData.ChoiceType != EConquestChoiceType::ProductionUnit &&
		ChoiceData.ChoiceType != EConquestChoiceType::ProductionProject
	)
	{
		return;
	}

	if (ChoiceData.ChoiceId.IsNone())
	{
		return;
	}

	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS || !GS->CityManager)
	{
		return;
	}

	bool bSelectedProduction = false;
	if (ChoiceData.ChoiceType == EConquestChoiceType::ProductionBuilding)
	{
		bSelectedProduction = GS->CityManager->SetCityProductionBuildingById(
			CityId,
			ChoiceData.ChoiceId
		);
	}
	else if (ChoiceData.ChoiceType == EConquestChoiceType::ProductionUnit)
	{
		bSelectedProduction = GS->CityManager->SetCityProductionUnitById(
			CityId,
			ChoiceData.ChoiceId
		);
	}
	else if (ChoiceData.ChoiceType == EConquestChoiceType::ProductionProject)
	{
		bSelectedProduction = GS->CityManager->SetCityProductionProjectById(
			CityId,
			ChoiceData.ChoiceId
		);
	}

	Refresh();

	if (bSelectedProduction)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PC->GetHUD()))
			{
				ConquestHUD->HideCityPanel();
			}
		}
	}
}
