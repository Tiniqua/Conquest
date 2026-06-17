#include "Conquest/UI/ConquestCityPanelWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/UI/ConquestChoiceButtonWidget.h"

void UConquestCityPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Refresh();
}

void UConquestCityPanelWidget::SetCity(int32 InCityId)
{
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

	if (PopulationText)
	{
		PopulationText->SetText(FText::Format(
			NSLOCTEXT("Conquest", "PopulationFormat", "Population: {0} | Food: {1}"),
			FText::AsNumber(City->Population),
			FText::AsNumber(FMath::FloorToInt(City->FoodStored))
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

			if (
				City->CurrentProduction.Type == ECityProductionType::Building &&
				GS->ContentManager
			)
			{
				BuildingRow = GS->ContentManager->FindBuilding(
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

	const TArray<FName> AvailableBuildingIds =
		GS->CityManager->GetAvailableProductionBuildingIdsForCity(CityId);

	for (const FName BuildingId : AvailableBuildingIds)
	{
		if (BuildingId.IsNone())
		{
			continue;
		}

		const FConquestBuildingRow* BuildingRow =
			GS->ContentManager->FindBuilding(BuildingId);

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
			GS->CityManager->EstimateTurnsToBuildById(CityId, BuildingId);

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

void UConquestCityPanelWidget::HandleProductionChoiceClicked(
	const FConquestChoiceButtonData& ChoiceData
)
{
	if (ChoiceData.ChoiceType != EConquestChoiceType::ProductionBuilding)
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

	GS->CityManager->SetCityProductionBuildingById(
		CityId,
		ChoiceData.ChoiceId
	);

	Refresh();
}
