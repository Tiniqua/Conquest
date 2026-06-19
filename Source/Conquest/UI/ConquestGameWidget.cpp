#include "ConquestGameWidget.h"

#include "ConquestHUD.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "Conquest/UI/ConquestChoiceButtonWidget.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Tech/ConquestTechTypes.h"

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

FText UConquestGameWidget::FormatStoredYieldText(const FText& Label, int32 Stored, int32 PerTurn)
{
	return FText::Format(
		NSLOCTEXT("Conquest", "TopBarStoredYieldFormat", "{0}: {1} (+{2})"),
		Label,
		FText::AsNumber(Stored),
		FText::AsNumber(PerTurn)
	);
}

FText UConquestGameWidget::FormatPerTurnYieldText(const FText& Label, int32 PerTurn)
{
	return FText::Format(
		NSLOCTEXT("Conquest", "TopBarPerTurnYieldFormat", "{0}: +{1}"),
		Label,
		FText::AsNumber(PerTurn)
	);
}

void UConquestGameWidget::SetTopBarYieldTexts(const FConquestTopBarYieldData& YieldData)
{
	SetVisibility(TopBarYieldPanel, ESlateVisibility::Visible);
	SetVisibility(
		TopBarLocalYieldPanel,
		YieldData.bShowSelectedCityLocalYields ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
	);

	if (YieldData.bShowSelectedCityLocalYields)
	{
		SetText(
			TopBarFoodText,
			FormatPerTurnYieldText(NSLOCTEXT("Conquest", "YieldFoodShort", "Food"), YieldData.SelectedCityYieldPerTurn.Food)
		);
		SetText(
			TopBarProductionText,
			FormatPerTurnYieldText(NSLOCTEXT("Conquest", "YieldProductionShort", "Prod"), YieldData.SelectedCityYieldPerTurn.Production)
		);
	}
	else
	{
		ClearText(TopBarFoodText);
		ClearText(TopBarProductionText);
	}

	SetText(
		TopBarGoldText,
		FormatStoredYieldText(NSLOCTEXT("Conquest", "YieldGoldShort", "Gold"), YieldData.EmpireStoredYields.Gold, YieldData.EmpireYieldPerTurn.Gold)
	);
	SetText(
		TopBarScienceText,
		FormatPerTurnYieldText(NSLOCTEXT("Conquest", "YieldScienceShort", "Sci"), YieldData.EmpireYieldPerTurn.Science)
	);
	SetText(
		TopBarCultureText,
		FormatStoredYieldText(NSLOCTEXT("Conquest", "YieldCultureShort", "Culture"), YieldData.EmpireStoredYields.Culture, YieldData.EmpireYieldPerTurn.Culture)
	);
	SetText(
		TopBarFaithText,
		FormatStoredYieldText(NSLOCTEXT("Conquest", "YieldFaithShort", "Faith"), YieldData.EmpireStoredYields.Faith, YieldData.EmpireYieldPerTurn.Faith)
	);
	SetText(
		TopBarHappinessText,
		YieldData.bIsUnhappy
			? FText::Format(
				NSLOCTEXT("Conquest", "TopBarUnhappyFormat", "Happy: {0} (-25% Unhappy)"),
				FText::AsNumber(YieldData.Happiness)
			)
			: FText::Format(
				NSLOCTEXT("Conquest", "TopBarHappinessFormat", "Happy: {0}"),
				FText::AsNumber(YieldData.Happiness)
			)
	);
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
		EndTurnButton->OnClicked.RemoveDynamic(this, &UConquestGameWidget::HandleEndTurnClicked);
		EndTurnButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleEndTurnClicked);
	}

	if (ResearchButton)
	{
		ResearchButton->OnClicked.RemoveDynamic(this, &UConquestGameWidget::HandleResearchClicked);
		ResearchButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleResearchClicked);
	}

	if (TileExpansionConfirmButton)
	{
		TileExpansionConfirmButton->OnClicked.RemoveDynamic(this, &UConquestGameWidget::HandleTileExpansionConfirmClicked);
		TileExpansionConfirmButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleTileExpansionConfirmClicked);
	}

	if (TileExpansionCancelButton)
	{
		TileExpansionCancelButton->OnClicked.RemoveDynamic(this, &UConquestGameWidget::HandleTileExpansionCancelClicked);
		TileExpansionCancelButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleTileExpansionCancelClicked);
	}

	if (TileImprovementCloseButton)
	{
		TileImprovementCloseButton->OnClicked.RemoveDynamic(this, &UConquestGameWidget::HandleTileImprovementCloseClicked);
		TileImprovementCloseButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleTileImprovementCloseClicked);
	}

	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		ConquestGS->OnConquestStateChanged.RemoveDynamic(this, &UConquestGameWidget::HandleConquestStateChanged);
		ConquestGS->OnConquestStateChanged.AddDynamic(this, &UConquestGameWidget::HandleConquestStateChanged);

		if (ConquestGS->TurnManager)
		{
			ConquestGS->TurnManager->OnTurnChanged.RemoveDynamic(this, &UConquestGameWidget::HandleTurnChanged);
			ConquestGS->TurnManager->OnTurnChanged.AddDynamic(this, &UConquestGameWidget::HandleTurnChanged);
		}

		if (ConquestGS->TechManager)
		{
			ConquestGS->TechManager->OnResearchChanged.RemoveDynamic(this, &UConquestGameWidget::HandleResearchChanged);
			ConquestGS->TechManager->OnResearchChanged.AddDynamic(this, &UConquestGameWidget::HandleResearchChanged);
		}
	}
	
	ClearHoveredTileInfo();
	ClearTileExpansionConfirmation();
	ClearTileImprovementChoices();
	RefreshTurnInfo();
	RefreshTopBarYieldInfo();
	RefreshResearchInfo();
}

void UConquestGameWidget::NativeDestruct()
{
	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		ConquestGS->OnConquestStateChanged.RemoveDynamic(this, &UConquestGameWidget::HandleConquestStateChanged);

		if (ConquestGS->TurnManager)
		{
			ConquestGS->TurnManager->OnTurnChanged.RemoveDynamic(this, &UConquestGameWidget::HandleTurnChanged);
		}

		if (ConquestGS->TechManager)
		{
			ConquestGS->TechManager->OnResearchChanged.RemoveDynamic(this, &UConquestGameWidget::HandleResearchChanged);
		}
	}

	Super::NativeDestruct();
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

void UConquestGameWidget::HandleTurnChanged(int32 NewTurn)
{
	(void)NewTurn;
	RefreshTurnInfo();
	RefreshTopBarYieldInfo();
	RefreshResearchInfo();
}

void UConquestGameWidget::HandleConquestStateChanged()
{
	RefreshTurnInfo();
	RefreshTopBarYieldInfo();
	RefreshResearchInfo();
}

void UConquestGameWidget::HandleResearchChanged()
{
	RefreshResearchInfo();
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

FConquestTopBarYieldData UConquestGameWidget::GetTopBarYieldData() const
{
	FConquestTopBarYieldData Result;

	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS)
	{
		return Result;
	}

	Result.EmpireStoredYields = ConquestGS->HumanPlayer.StoredYields;
	Result.EmpireYieldPerTurn = ConquestGS->HumanPlayer.CachedYieldPerTurn;
	Result.Happiness = ConquestGS->HumanPlayer.CachedHappiness;
	Result.bIsUnhappy = ConquestGS->HumanPlayer.CachedHappiness < 0;
	Result.SelectedCityId = SelectedCityYieldContextId;
	Result.bShowSelectedCityLocalYields = SelectedCityYieldContextId != INDEX_NONE;

	if (Result.bShowSelectedCityLocalYields && ConquestGS->CityManager)
	{
		if (const FCityState* City = ConquestGS->CityManager->GetCity(SelectedCityYieldContextId))
		{
			Result.SelectedCityYieldPerTurn = City->CachedYieldPerTurn;
		}
		else
		{
			Result.SelectedCityId = INDEX_NONE;
			Result.bShowSelectedCityLocalYields = false;
		}
	}

	return Result;
}

void UConquestGameWidget::RefreshTopBarYieldInfo()
{
	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		if (ConquestGS->YieldManager)
		{
			ConquestGS->YieldManager->RecalculateEmpireYieldPerTurn(ConquestGS->HumanPlayer.PlayerId);
		}
	}

	SetTopBarYieldTexts(GetTopBarYieldData());
}

FText UConquestGameWidget::GetCurrentResearchStatusText() const
{
	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS || !ConquestGS->ContentManager)
	{
		return NSLOCTEXT("Conquest", "ResearchStatusUnavailable", "Choose Research");
	}

	const FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayer();
	if (Player.CurrentResearchId.IsNone())
	{
		return NSLOCTEXT("Conquest", "ResearchStatusNone", "Choose Research");
	}

	const FConquestTechRow* CurrentTech = ConquestGS->ContentManager->FindTech(Player.CurrentResearchId);
	if (!CurrentTech)
	{
		return FText::FromName(Player.CurrentResearchId);
	}

	const float Cost = FMath::Max(1.0f, static_cast<float>(CurrentTech->ScienceCost));
	const int32 Percent = FMath::Clamp(
		FMath::RoundToInt((Player.CurrentResearchProgress / Cost) * 100.0f),
		0,
		100
	);

	return FText::Format(
		NSLOCTEXT("Conquest", "ResearchStatusPercentFormat", "{0} - {1}%"),
		CurrentTech->DisplayName,
		FText::AsNumber(Percent)
	);
}

void UConquestGameWidget::RefreshResearchInfo()
{
	SetText(CurrentResearchText, GetCurrentResearchStatusText());
}

void UConquestGameWidget::SetSelectedCityYieldContext(int32 CityId)
{
	SelectedCityYieldContextId = CityId;
	RefreshTopBarYieldInfo();
}

void UConquestGameWidget::ClearSelectedCityYieldContext()
{
	SelectedCityYieldContextId = INDEX_NONE;
	RefreshTopBarYieldInfo();
}

void UConquestGameWidget::ShowTileExpansionConfirmation(const FConquestTileExpansionChoiceData& ChoiceData)
{
	PendingTileExpansionChoice = ChoiceData;

	if (!ChoiceData.bIsValid)
	{
		ClearTileExpansionConfirmation();
		return;
	}

	SetVisibility(TileExpansionConfirmPanel, ESlateVisibility::Visible);

	SetText(
		TileExpansionTitleText,
		FString::Printf(TEXT("Claim Tile [%d, %d]"), ChoiceData.Coord.X, ChoiceData.Coord.Y)
	);
	SetText(
		TileExpansionDetailText,
		FString::Printf(
			TEXT("%s | Resource: %s | Features: %s"),
			*ChoiceData.TileType,
			*ChoiceData.Resource,
			*ChoiceData.Features
		)
	);
	SetText(TileExpansionYieldText, ChoiceData.Yield.ToCompactString());
}

void UConquestGameWidget::ClearTileExpansionConfirmation()
{
	PendingTileExpansionChoice = FConquestTileExpansionChoiceData();
	SetVisibility(TileExpansionConfirmPanel, ESlateVisibility::Collapsed);
	ClearText(TileExpansionTitleText);
	ClearText(TileExpansionDetailText);
	ClearText(TileExpansionYieldText);
}

void UConquestGameWidget::ShowTileImprovementChoices(
	const FConquestTileImprovementChoiceData& ChoiceData,
	const TArray<FConquestChoiceButtonData>& ImprovementChoices
)
{
	PendingTileImprovementChoice = ChoiceData;

	if (!ChoiceData.bIsValid || ImprovementChoices.Num() <= 0)
	{
		ClearTileImprovementChoices();
		return;
	}

	SetVisibility(TileImprovementChoicePanel, ESlateVisibility::Visible);
	SetText(
		TileImprovementTitleText,
		FString::Printf(TEXT("Upgrade Tile [%d, %d]"), ChoiceData.Coord.X, ChoiceData.Coord.Y)
	);

	if (!TileImprovementButtonBox)
	{
		return;
	}

	TileImprovementButtonBox->ClearChildren();

	if (!ChoiceButtonWidgetClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("ConquestGameWidget: ChoiceButtonWidgetClass is not set for tile improvements.")
		);
		return;
	}

	for (const FConquestChoiceButtonData& ImprovementChoice : ImprovementChoices)
	{
		UConquestChoiceButtonWidget* ChoiceButton =
			CreateWidget<UConquestChoiceButtonWidget>(GetOwningPlayer(), ChoiceButtonWidgetClass);

		if (!ChoiceButton)
		{
			continue;
		}

		ChoiceButton->SetupChoiceButton(ImprovementChoice);
		ChoiceButton->OnChoiceClicked.RemoveDynamic(
			this,
			&UConquestGameWidget::HandleTileImprovementChoiceClicked
		);
		ChoiceButton->OnChoiceClicked.AddDynamic(
			this,
			&UConquestGameWidget::HandleTileImprovementChoiceClicked
		);

		TileImprovementButtonBox->AddChild(ChoiceButton);
	}
}

void UConquestGameWidget::ClearTileImprovementChoices()
{
	PendingTileImprovementChoice = FConquestTileImprovementChoiceData();
	SetVisibility(TileImprovementChoicePanel, ESlateVisibility::Collapsed);
	ClearText(TileImprovementTitleText);

	if (TileImprovementButtonBox)
	{
		TileImprovementButtonBox->ClearChildren();
	}
}

void UConquestGameWidget::HandleTileExpansionConfirmClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		ConquestHUD->ConfirmSelectedExpansionTile();
	}
}

void UConquestGameWidget::HandleTileExpansionCancelClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		ConquestHUD->CancelSelectedExpansionTile();
	}
	else
	{
		ClearTileExpansionConfirmation();
	}
}

void UConquestGameWidget::HandleTileImprovementChoiceClicked(const FConquestChoiceButtonData& ChoiceData)
{
	if (!PendingTileImprovementChoice.bIsValid || ChoiceData.ChoiceType != EConquestChoiceType::TileImprovement)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		ConquestHUD->PurchaseSelectedTileImprovement(ChoiceData.ChoiceId);
	}
}

void UConquestGameWidget::HandleTileImprovementCloseClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		ConquestHUD->ClearTileImprovementChoices();
	}
	else
	{
		ClearTileImprovementChoices();
	}
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
