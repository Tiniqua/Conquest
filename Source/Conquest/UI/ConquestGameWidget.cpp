#include "ConquestGameWidget.h"

#include "ConquestHUD.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestTurnManager.h"

void UConquestGameWidget::SetText(UTextBlock* TextBlock, const FText& Text)
{
	if (TextBlock)
	{
		TextBlock->SetText(Text);
	}
}

void UConquestGameWidget::SetText(UTextBlock* TextBlock, const FString& Text)
{
	SetText(TextBlock, FText::FromString(Text));
}

void UConquestGameWidget::ClearText(UTextBlock* TextBlock)
{
	SetText(TextBlock, FText::GetEmpty());
}

void UConquestGameWidget::SetVisibility(UWidget* Widget, ESlateVisibility Visibility)
{
	if (Widget)
	{
		Widget->SetVisibility(Visibility);
	}
}

void UConquestGameWidget::SetYieldTexts(const FHexYield& Yield)
{
	SetText(TileFoodText, FString::Printf(TEXT("Food: %d"), Yield.Food));
	SetText(TileProductionText, FString::Printf(TEXT("Prod: %d"), Yield.Production));
	SetText(TileGoldText, FString::Printf(TEXT("Gold: %d"), Yield.Gold));
	SetText(TileScienceText, FString::Printf(TEXT("Sci: %d"), Yield.Science));
	SetText(TileCultureText, FString::Printf(TEXT("Culture: %d"), Yield.Culture));
	SetText(TileFaithText, FString::Printf(TEXT("Faith: %d"), Yield.Faith));
}

void UConquestGameWidget::ClearTileTexts()
{
	ClearText(TileCoordText);
	ClearText(TileTypeText);
	ClearText(TileFeaturesText);
	ClearText(TileResourceText);
	ClearText(TileImprovementText);
	ClearText(TileFreshWaterText);
	ClearText(TileRiverText);
	ClearText(TileHeightText);
	ClearText(TileFoodText);
	ClearText(TileProductionText);
	ClearText(TileGoldText);
	ClearText(TileScienceText);
	ClearText(TileCultureText);
	ClearText(TileFaithText);
}

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
	SetVisibility(HoveredTileInfoPanel, ESlateVisibility::Visible);
	SetVisibility(TileDetailsBox, ESlateVisibility::Visible);
	SetVisibility(TileYieldsBox, ESlateVisibility::Visible);

	SetText(TileCoordText, FString::Printf(TEXT("Tile [%d, %d]"), HoveredTileData.Q, HoveredTileData.R));
	SetText(TileTypeText, FString::Printf(TEXT("Type: %s"), *HoveredTileData.TileType));
	SetText(TileFeaturesText, FString::Printf(TEXT("Features: %s"), *HoveredTileData.Features));
	SetText(TileResourceText, FString::Printf(TEXT("Resource: %s"), *HoveredTileData.Resource));
	SetText(TileImprovementText, FString::Printf(TEXT("Improvement: %s"), *HoveredTileData.Improvement));
	SetText(TileFreshWaterText, FString::Printf(TEXT("Fresh Water: %s"), *HoveredTileData.FreshWater));
	SetText(TileRiverText, FString::Printf(TEXT("River: %s"), *HoveredTileData.River));
	SetText(TileHeightText, FString::Printf(TEXT("Height: %.2f"), HoveredTileData.Height));

	SetYieldTexts(HoveredTileData.Yield);
}

void UConquestGameWidget::ClearHoveredTileInfo()
{
	SetVisibility(HoveredTileInfoPanel, ESlateVisibility::Collapsed);
	ClearTileTexts();
}
