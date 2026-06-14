#include "ConquestGameWidget.h"

#include "ConquestHUD.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestTurnManager.h"

void UConquestGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EndTurnButton)
	{
		EndTurnButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleEndTurnClicked);
	}

	if (ResearchButton)
	{
		ResearchButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleResearchClicked);
	}
	
	ClearHoveredTileInfo();
	RefreshTurnInfo();
}

void UConquestGameWidget::HandleEndTurnClicked()
{
	if (AConquestGameMode* ConquestGM = GetWorld() ? GetWorld()->GetAuthGameMode<AConquestGameMode>() : nullptr)
	{
		ConquestGM->EndCurrentTurn();
	}

	RefreshTurnInfo();
}

void UConquestGameWidget::HandleResearchClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PC->GetHUD()))
	{
		ConquestHUD->ShowResearchPanel();
	}
}

void UConquestGameWidget::RefreshTurnInfo()
{
	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS || !ConquestGS->TurnManager || !TurnText)
	{
		return;
	}

	TurnText->SetText(FText::Format(
		NSLOCTEXT("Conquest", "TurnTextFormat", "Turn {0}"),
		FText::AsNumber(ConquestGS->TurnManager->CurrentTurn)
	));
}

void UConquestGameWidget::UpdateHoveredTileInfo(const FHoveredHexTileWidgetData& HoveredTileData)
{
	if (HoveredTileInfoPanel)
	{
		HoveredTileInfoPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (TileCoordText)
	{
		TileCoordText->SetText(FText::FromString(
			FString::Printf(TEXT("Tile [%d, %d]"), HoveredTileData.Q, HoveredTileData.R)
		));
	}

	if (TileTypeText)
	{
		TileTypeText->SetText(FText::FromString(
			FString::Printf(TEXT("Type: %s"), *HoveredTileData.TileType)
		));
	}

	if (TileFeaturesText)
	{
		TileFeaturesText->SetText(FText::FromString(
			FString::Printf(TEXT("Features: %s"), *HoveredTileData.Features)
		));
	}

	if (TileResourceText)
	{
		TileResourceText->SetText(FText::FromString(
			FString::Printf(TEXT("Resource: %s"), *HoveredTileData.Resource)
		));
	}

	if (TileImprovementText)
	{
		TileImprovementText->SetText(FText::FromString(
			FString::Printf(TEXT("Improvement: %s"), *HoveredTileData.Improvement)
		));
	}

	if (TileFreshWaterText)
	{
		TileFreshWaterText->SetText(FText::FromString(
			FString::Printf(TEXT("Fresh Water: %s"), *HoveredTileData.FreshWater)
		));
	}

	if (TileRiverText)
	{
		TileRiverText->SetText(FText::FromString(
			FString::Printf(TEXT("River: %s"), *HoveredTileData.River)
		));
	}

	if (TileHeightText)
	{
		TileHeightText->SetText(FText::FromString(
			FString::Printf(TEXT("Height: %.2f"), HoveredTileData.Height)
		));
	}

	if (TileFoodText)
	{
		TileFoodText->SetText(FText::FromString(
			FString::Printf(TEXT("Food: %d"), HoveredTileData.Yield.Food)
		));
	}

	if (TileProductionText)
	{
		TileProductionText->SetText(FText::FromString(
			FString::Printf(TEXT("Prod: %d"), HoveredTileData.Yield.Production)
		));
	}

	if (TileGoldText)
	{
		TileGoldText->SetText(FText::FromString(
			FString::Printf(TEXT("Gold: %d"), HoveredTileData.Yield.Gold)
		));
	}

	if (TileScienceText)
	{
		TileScienceText->SetText(FText::FromString(
			FString::Printf(TEXT("Sci: %d"), HoveredTileData.Yield.Science)
		));
	}

	if (TileCultureText)
	{
		TileCultureText->SetText(FText::FromString(
			FString::Printf(TEXT("Culture: %d"), HoveredTileData.Yield.Culture)
		));
	}

	if (TileFaithText)
	{
		TileFaithText->SetText(FText::FromString(
			FString::Printf(TEXT("Faith: %d"), HoveredTileData.Yield.Faith)
		));
	}
}

void UConquestGameWidget::ClearHoveredTileInfo()
{
	if (HoveredTileInfoPanel)
	{
		HoveredTileInfoPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TileCoordText)
	{
		TileCoordText->SetText(FText::GetEmpty());
	}

	if (TileTypeText)
	{
		TileTypeText->SetText(FText::GetEmpty());
	}

	if (TileFeaturesText)
	{
		TileFeaturesText->SetText(FText::GetEmpty());
	}

	if (TileResourceText)
	{
		TileResourceText->SetText(FText::GetEmpty());
	}

	if (TileImprovementText)
	{
		TileImprovementText->SetText(FText::GetEmpty());
	}

	if (TileFreshWaterText)
	{
		TileFreshWaterText->SetText(FText::GetEmpty());
	}

	if (TileRiverText)
	{
		TileRiverText->SetText(FText::GetEmpty());
	}

	if (TileHeightText)
	{
		TileHeightText->SetText(FText::GetEmpty());
	}

	if (HoveredTileInfoPanel)
	{
		HoveredTileInfoPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (TileDetailsBox)
	{
		TileDetailsBox->SetVisibility(ESlateVisibility::Visible);
	}

	if (TileYieldsBox)
	{
		TileYieldsBox->SetVisibility(ESlateVisibility::Visible);
	}

	if (TileFoodText)
	{
		TileFoodText->SetText(FText::GetEmpty());
	}

	if (TileProductionText)
	{
		TileProductionText->SetText(FText::GetEmpty());
	}

	if (TileGoldText)
	{
		TileGoldText->SetText(FText::GetEmpty());
	}

	if (TileScienceText)
	{
		TileScienceText->SetText(FText::GetEmpty());
	}

	if (TileCultureText)
	{
		TileCultureText->SetText(FText::GetEmpty());
	}

	if (TileFaithText)
	{
		TileFaithText->SetText(FText::GetEmpty());
	}
}