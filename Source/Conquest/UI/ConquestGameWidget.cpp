#include "ConquestGameWidget.h"

#include "ConquestHUD.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "Conquest/UI/ConquestChoiceButtonWidget.h"
#include "Conquest/UI/ConquestUnitActionButtonWidget.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/Tech/ConquestTechTypes.h"
#include "Conquest/Units/ConquestUnitTypes.h"

namespace
{
	const FLinearColor ConquestTopBarHappinessPositiveColor(0.2f, 0.85f, 0.35f, 1.0f);
	const FLinearColor ConquestTopBarHappinessZeroColor(1.0f, 0.82f, 0.2f, 1.0f);
	const FLinearColor ConquestTopBarHappinessUnhappyColor(1.0f, 0.48f, 0.12f, 1.0f);
	const FLinearColor ConquestTopBarHappinessSevereColor(0.95f, 0.08f, 0.08f, 1.0f);

	FText FormatHappinessValueText(int32 Happiness)
	{
		return Happiness > 0
			? FText::FromString(FString::Printf(TEXT("+%d"), Happiness))
			: FText::AsNumber(Happiness);
	}

	FLinearColor GetHappinessDisplayColor(int32 Happiness)
	{
		if (ConquestHappiness::IsSeverelyUnhappy(Happiness))
		{
			return ConquestTopBarHappinessSevereColor;
		}

		if (ConquestHappiness::IsUnhappy(Happiness))
		{
			return ConquestTopBarHappinessUnhappyColor;
		}

		return Happiness == 0
			? ConquestTopBarHappinessZeroColor
			: ConquestTopBarHappinessPositiveColor;
	}
}

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

void UConquestGameWidget::SetWidgetVisibility(UWidget* Widget, ESlateVisibility Visibility)
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
	SetWidgetVisibility(TopBarYieldPanel, ESlateVisibility::Visible);
	SetWidgetVisibility(
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
				NSLOCTEXT("Conquest", "TopBarUnhappyFormat", "Happy: {0} ({1})"),
				FormatHappinessValueText(YieldData.Happiness),
				YieldData.HappinessPenaltyText
			)
			: FText::Format(
				NSLOCTEXT("Conquest", "TopBarHappinessFormat", "Happy: {0}"),
				FormatHappinessValueText(YieldData.Happiness)
			)
	);
	if (TopBarHappinessText)
	{
		TopBarHappinessText->SetColorAndOpacity(FSlateColor(GetHappinessDisplayColor(YieldData.Happiness)));
	}

	auto FindStockpile = [&YieldData](FName ResourceId) -> const FConquestStrategicResourceStockpile*
	{
		return YieldData.StrategicResources.FindByPredicate([ResourceId](const FConquestStrategicResourceStockpile& Stockpile)
		{
			return Stockpile.ResourceId == ResourceId;
		});
	};

	auto FormatStrategicResource = [&FindStockpile](const TCHAR* Label, FName ResourceId)
	{
		const FConquestStrategicResourceStockpile* Stockpile = FindStockpile(ResourceId);
		const int32 Stored = Stockpile ? Stockpile->Stored : 0;
		const int32 Cap = Stockpile ? Stockpile->Cap : 0;
		return FText::FromString(FString::Printf(TEXT("%s: %d/%d"), Label, Stored, Cap));
	};

	SetText(TopBarHorsesText, FormatStrategicResource(TEXT("Horses"), FName(TEXT("Horses"))));
	SetText(TopBarIronText, FormatStrategicResource(TEXT("Iron"), FName(TEXT("Iron"))));
	SetText(TopBarCoalText, FormatStrategicResource(TEXT("Coal"), FName(TEXT("Coal"))));
	SetText(TopBarAluminiumText, FormatStrategicResource(TEXT("Aluminium"), FName(TEXT("Aluminium"))));
	SetWidgetVisibility(TopBarStrategicResources, ESlateVisibility::Visible);
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

	if (UnitAugmentCloseButton)
	{
		UnitAugmentCloseButton->OnClicked.RemoveDynamic(this, &UConquestGameWidget::HandleUnitAugmentCloseClicked);
		UnitAugmentCloseButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleUnitAugmentCloseClicked);
	}

	if (EndGameReturnToMenuButton)
	{
		EndGameReturnToMenuButton->OnClicked.RemoveDynamic(this, &UConquestGameWidget::HandleEndGameReturnToMenuClicked);
		EndGameReturnToMenuButton->OnClicked.AddDynamic(this, &UConquestGameWidget::HandleEndGameReturnToMenuClicked);
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
	ClearUnitAugmentChoices();
	ClearCombatPreview();
	ClearEndGameResult();
	ClearSelectedUnitInfo();
	RefreshTurnInfo();
	RefreshTopBarYieldInfo();
	RefreshResearchInfo();
	RefreshEndGameResultFromGameState();
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
	if (FocusNextRequiredEndTurnAction())
	{
		RefreshTurnInfo();
		return;
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestEndTurn();
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
	RefreshSelectedUnitInfoFromGameState();
	RefreshEndGameResultFromGameState();
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

FText UConquestGameWidget::GetEndTurnButtonText() const
{
	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS)
	{
		return NSLOCTEXT("Conquest", "EndTurnButtonDefault", "Next Turn");
	}

	FConquestEndTurnBlocker Blocker;
	if (!ConquestGS->GetEndTurnBlockerForPlayer(ConquestGS->GetLocalPlayerId(), Blocker))
	{
		return NSLOCTEXT("Conquest", "EndTurnButtonReady", "Next Turn");
	}

	return Blocker.Message.IsEmpty()
		? NSLOCTEXT("Conquest", "EndTurnButtonNeedsActions", "Actions Required")
		: Blocker.Message;
}

FConquestTopBarYieldData UConquestGameWidget::GetTopBarYieldData() const
{
	FConquestTopBarYieldData Result;

	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS)
	{
		return Result;
	}

	const FConquestPlayerEmpireState& LocalPlayer = ConquestGS->GetHumanPlayer();
	Result.EmpireStoredYields = LocalPlayer.StoredYields;
	Result.EmpireYieldPerTurn = LocalPlayer.CachedYieldPerTurn;
	Result.Happiness = LocalPlayer.CachedHappiness;
	Result.bIsUnhappy = ConquestHappiness::IsUnhappy(LocalPlayer.CachedHappiness);
	Result.bIsSeverelyUnhappy = ConquestHappiness::IsSeverelyUnhappy(LocalPlayer.CachedHappiness);
	Result.HappinessPenaltyText = ConquestHappiness::GetPenaltyText(LocalPlayer.CachedHappiness);
	Result.StrategicResources = LocalPlayer.StrategicResources;
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
		const int32 LocalPlayerId = ConquestGS->GetHumanPlayer().PlayerId;
		if (ConquestGS->CityManager)
		{
			ConquestGS->CityManager->RecalculateEmpireYields(LocalPlayerId);
		}
		else if (ConquestGS->YieldManager)
		{
			ConquestGS->YieldManager->RecalculateEmpireYieldPerTurn(LocalPlayerId);
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

void UConquestGameWidget::RefreshSelectedUnitInfoFromGameState()
{
	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS || !ConquestGS->ContentManager || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		ClearSelectedUnitInfo();
		return;
	}

	const FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayer();
	const FConquestUnitState* SelectedUnit = Player.Units.FindByPredicate([ConquestGS](const FConquestUnitState& Unit)
	{
		return Unit.UnitInstanceId == ConquestGS->SelectedUnitInstanceId;
	});

	if (!SelectedUnit)
	{
		ClearSelectedUnitInfo();
		return;
	}

	FConquestSelectedUnitWidgetData UnitWidgetData;
	UnitWidgetData.UnitInstanceId = SelectedUnit->UnitInstanceId;
	UnitWidgetData.bIsValid = true;

	const FConquestUnitRow* UnitRow = ConquestGS->ContentManager->FindUnit(SelectedUnit->UnitId);
	UnitWidgetData.UnitName = UnitRow && !UnitRow->DisplayName.IsEmpty()
		? UnitRow->DisplayName
		: FText::FromName(SelectedUnit->UnitId);
	UnitWidgetData.bCanFoundCity = UnitRow && UnitRow->bCanFoundCity;
	UnitWidgetData.CurrentMovementPoints = SelectedUnit->CurrentMovementPoints;
	UnitWidgetData.bIsSleeping = SelectedUnit->bIsSleeping;
	UnitWidgetData.HealthText = FText::Format(
		NSLOCTEXT("Conquest", "SelectedUnitHealthMovementFormat", "{0}/{1} HP | {2}/{3} Move"),
		FText::AsNumber(SelectedUnit->CurrentHealth),
		FText::AsNumber(SelectedUnit->CachedMaxHealth),
		FText::AsNumber(SelectedUnit->CurrentMovementPoints),
		FText::AsNumber(SelectedUnit->CachedMovementPoints)
	);

	if (SelectedUnit->bIsSleeping || SelectedUnit->CurrentMovementPoints <= 0)
	{
		ClearSelectedUnitInfo();
		return;
	}

	ShowSelectedUnitInfo(UnitWidgetData);
}

void UConquestGameWidget::ShowSelectedUnitInfo(const FConquestSelectedUnitWidgetData& UnitData)
{
	if (!UnitData.bIsValid)
	{
		ClearSelectedUnitInfo();
		return;
	}

	SetWidgetVisibility(UnitActionPanel, ESlateVisibility::Visible);
	SetText(
		SelectedUnitText,
		FText::Format(
			NSLOCTEXT("Conquest", "SelectedUnitPanelFormat", "{0} | {1}"),
			UnitData.UnitName,
			UnitData.HealthText
		)
	);

	if (UnitActionButtonBox)
	{
		UnitActionButtonBox->ClearChildren();

		if (UnitActionButtonWidgetClass)
		{
			struct FUnitActionButtonDefinition
			{
				FName ActionId;
				FText Title;
			};

			TArray<FUnitActionButtonDefinition> ActionDefinitions =
			{
				{ FName(TEXT("Move")), NSLOCTEXT("Conquest", "UnitActionMove", "Move") },
				{ FName(TEXT("Attack")), NSLOCTEXT("Conquest", "UnitActionAttack", "Attack") },
				{ FName(TEXT("Augment")), NSLOCTEXT("Conquest", "UnitActionAugment", "Augment") },
				{ FName(TEXT("Fortify")), NSLOCTEXT("Conquest", "UnitActionFortify", "Fortify") },
				{ FName(TEXT("DoNothing")), NSLOCTEXT("Conquest", "UnitActionDoNothing", "Do Nothing") },
				{ FName(TEXT("Sleep")), NSLOCTEXT("Conquest", "UnitActionSleep", "Sleep") },
				{ FName(TEXT("Disband")), NSLOCTEXT("Conquest", "UnitActionDisband", "Disband") }
			};

			if (UnitData.bCanFoundCity)
			{
				FUnitActionButtonDefinition SettleAction;
				SettleAction.ActionId = FName(TEXT("Settle"));
				SettleAction.Title = NSLOCTEXT("Conquest", "UnitActionSettle", "Settle");
				ActionDefinitions.Insert(SettleAction, 1);
			}

			for (const FUnitActionButtonDefinition& ActionDefinition : ActionDefinitions)
			{
				UConquestUnitActionButtonWidget* ActionButton =
					CreateWidget<UConquestUnitActionButtonWidget>(GetOwningPlayer(), UnitActionButtonWidgetClass);
				if (!ActionButton)
				{
					continue;
				}

				ActionButton->SetupUnitActionButton(ActionDefinition.ActionId, ActionDefinition.Title);
				ActionButton->SetActionEnabled(IsUnitActionEnabled(ActionDefinition.ActionId, UnitData));
				ActionButton->OnUnitActionClicked.RemoveDynamic(
					this,
					&UConquestGameWidget::HandleUnitActionClicked
				);
				ActionButton->OnUnitActionClicked.AddDynamic(
					this,
					&UConquestGameWidget::HandleUnitActionClicked
				);
				UnitActionButtonBox->AddChild(ActionButton);
			}
		}
	}
}

void UConquestGameWidget::ClearSelectedUnitInfo()
{
	SetWidgetVisibility(UnitActionPanel, ESlateVisibility::Collapsed);
	ClearText(SelectedUnitText);

	if (UnitActionButtonBox)
	{
		UnitActionButtonBox->ClearChildren();
	}
}

bool UConquestGameWidget::FocusNextRequiredEndTurnAction()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS)
	{
		return false;
	}

	FConquestEndTurnBlocker Blocker;
	if (!ConquestGS->GetEndTurnBlockerForPlayer(ConquestGS->GetLocalPlayerId(), Blocker))
	{
		return false;
	}

	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (!ConquestHUD)
	{
		return true;
	}

	if (ConquestGS->TurnManager && ConquestGS->TurnManager->CurrentPhase == EConquestTurnPhase::AwaitingFirstCity)
	{
		const int32 LocalPlayerId = ConquestGS->GetLocalPlayerId();
		const bool bLocalPlayerHasCity =
			ConquestGS->CityManager &&
			ConquestGS->CityManager->Cities.ContainsByPredicate(
				[LocalPlayerId](const FCityState& City)
				{
					return City.OwnerPlayerId == LocalPlayerId;
				}
			);
		if (!bLocalPlayerHasCity)
		{
			ConquestHUD->BeginStartingRegionSelection();
		}
		return true;
	}

	switch (Blocker.Type)
	{
	case EConquestEndTurnBlockType::Research:
		ConquestHUD->ShowResearchPanel();
		return true;
	case EConquestEndTurnBlockType::CityProduction:
		if (Blocker.CityId != INDEX_NONE)
		{
			if (ConquestGS->CityManager)
			{
				if (const FCityState* City = ConquestGS->CityManager->GetCity(Blocker.CityId))
				{
					ConquestHUD->FocusCameraOnTile(City->CenterTile, false);
				}
			}
			ConquestHUD->ShowCityPanel(Blocker.CityId);
		}
		return true;
	case EConquestEndTurnBlockType::CityGrowth:
		if (Blocker.CityId != INDEX_NONE)
		{
			if (ConquestGS->CityManager)
			{
				if (const FCityState* City = ConquestGS->CityManager->GetCity(Blocker.CityId))
				{
					ConquestHUD->FocusCameraOnTile(City->CenterTile, false);
				}
			}
			ConquestHUD->ShowCityPanel(Blocker.CityId);
		}
		return true;
	case EConquestEndTurnBlockType::UnitOrders:
		{
			const FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayer();
			const FConquestUnitState* UnitToSelect = Player.Units.FindByPredicate([&Blocker](const FConquestUnitState& Unit)
			{
				return Unit.UnitInstanceId == Blocker.UnitInstanceId;
			});

			if (UnitToSelect)
			{
				ConquestHUD->FocusCameraOnTile(UnitToSelect->TileCoord, false);
				ConquestHUD->SelectUnitAtTile(UnitToSelect->TileCoord.X, UnitToSelect->TileCoord.Y);
			}
			return true;
		}
	default:
		return true;
	}
}

bool UConquestGameWidget::IsUnitActionEnabled(FName ActionId, const FConquestSelectedUnitWidgetData& UnitData) const
{
	if (!UnitData.bIsValid)
	{
		return false;
	}

	if (ActionId == FName(TEXT("Move")) ||
		ActionId == FName(TEXT("Attack")) ||
		ActionId == FName(TEXT("Fortify")) ||
		ActionId == FName(TEXT("DoNothing")))
	{
		return UnitData.CurrentMovementPoints > 0;
	}

	if (ActionId == FName(TEXT("Settle")))
	{
		return UnitData.bCanFoundCity && UnitData.CurrentMovementPoints > 0;
	}

	return true;
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

	SetWidgetVisibility(TileExpansionConfirmPanel, ESlateVisibility::Visible);

	SetText(
		TileExpansionTitleText,
		ChoiceData.bAssigningToOwnedTile
			? FString::Printf(TEXT("Assign Citizen [%d, %d]"), ChoiceData.Coord.X, ChoiceData.Coord.Y)
			: FString::Printf(TEXT("Claim Tile [%d, %d]"), ChoiceData.Coord.X, ChoiceData.Coord.Y)
	);
	SetText(
		TileExpansionDetailText,
		ChoiceData.bAssigningToOwnedTile
			? FString::Printf(
				TEXT("%s | Resource: %s | Features: %s | Citizens: %d -> %d"),
				*ChoiceData.TileType,
				*ChoiceData.Resource,
				*ChoiceData.Features,
				ChoiceData.CurrentAssignedCitizens,
				ChoiceData.ResultAssignedCitizens
			)
			: FString::Printf(
				TEXT("%s | Resource: %s | Features: %s"),
				*ChoiceData.TileType,
				*ChoiceData.Resource,
				*ChoiceData.Features
			)
	);
	SetText(TileExpansionYieldText, ChoiceData.Yield.ToCompactString());
}

void UConquestGameWidget::ShowStartingCityConfirmation(const FConquestTileExpansionChoiceData& ChoiceData)
{
	PendingTileExpansionChoice = ChoiceData;

	if (!ChoiceData.bIsValid)
	{
		ClearTileExpansionConfirmation();
		return;
	}

	SetWidgetVisibility(TileExpansionConfirmPanel, ESlateVisibility::Visible);
	SetText(
		TileExpansionTitleText,
		FString::Printf(TEXT("Settle City [%d, %d]"), ChoiceData.Coord.X, ChoiceData.Coord.Y)
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
	SetWidgetVisibility(TileExpansionConfirmPanel, ESlateVisibility::Collapsed);
	ClearText(TileExpansionTitleText);
	ClearText(TileExpansionDetailText);
	ClearText(TileExpansionYieldText);
}

void UConquestGameWidget::ShowEndGameResult(bool bLocalPlayerWon)
{
	SetWidgetVisibility(EndGamePanel, ESlateVisibility::Visible);
	SetText(
		EndGameTitleText,
		bLocalPlayerWon
			? NSLOCTEXT("Conquest", "EndGameYouWin", "You Win")
			: NSLOCTEXT("Conquest", "EndGameYouLose", "You Lose")
	);
}

void UConquestGameWidget::ClearEndGameResult()
{
	SetWidgetVisibility(EndGamePanel, ESlateVisibility::Collapsed);
	ClearText(EndGameTitleText);
}

void UConquestGameWidget::RefreshEndGameResultFromGameState()
{
	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->bGameEnded)
	{
		ClearEndGameResult();
		return;
	}

	ShowEndGameResult(ConquestGS->WinningPlayerId == ConquestGS->GetLocalPlayerId());
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

	SetWidgetVisibility(TileImprovementChoicePanel, ESlateVisibility::Visible);
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
	SetWidgetVisibility(TileImprovementChoicePanel, ESlateVisibility::Collapsed);
	ClearText(TileImprovementTitleText);

	if (TileImprovementButtonBox)
	{
		TileImprovementButtonBox->ClearChildren();
	}
}

void UConquestGameWidget::ShowUnitAugmentChoices(
	int32 UnitInstanceId,
	const TArray<FConquestChoiceButtonData>& AugmentChoices
)
{
	PendingUnitAugmentUnitInstanceId = UnitInstanceId;

	if (UnitInstanceId == INDEX_NONE || AugmentChoices.Num() <= 0)
	{
		ClearUnitAugmentChoices();
		return;
	}

	SetWidgetVisibility(UnitAugmentChoicePanel, ESlateVisibility::Visible);
	SetText(UnitAugmentTitleText, NSLOCTEXT("Conquest", "UnitAugmentTitle", "Augment Unit"));

	if (!UnitAugmentButtonBox)
	{
		return;
	}

	UnitAugmentButtonBox->ClearChildren();

	if (!ChoiceButtonWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConquestGameWidget: ChoiceButtonWidgetClass is not set for unit augments."));
		return;
	}

	for (const FConquestChoiceButtonData& AugmentChoice : AugmentChoices)
	{
		UConquestChoiceButtonWidget* ChoiceButton =
			CreateWidget<UConquestChoiceButtonWidget>(GetOwningPlayer(), ChoiceButtonWidgetClass);
		if (!ChoiceButton)
		{
			continue;
		}

		ChoiceButton->SetupChoiceButton(AugmentChoice);
		ChoiceButton->OnChoiceClicked.RemoveDynamic(
			this,
			&UConquestGameWidget::HandleUnitAugmentChoiceClicked
		);
		ChoiceButton->OnChoiceClicked.AddDynamic(
			this,
			&UConquestGameWidget::HandleUnitAugmentChoiceClicked
		);

		UnitAugmentButtonBox->AddChild(ChoiceButton);
	}
}

void UConquestGameWidget::ClearUnitAugmentChoices()
{
	PendingUnitAugmentUnitInstanceId = INDEX_NONE;
	SetWidgetVisibility(UnitAugmentChoicePanel, ESlateVisibility::Collapsed);
	ClearText(UnitAugmentTitleText);

	if (UnitAugmentButtonBox)
	{
		UnitAugmentButtonBox->ClearChildren();
	}
}

void UConquestGameWidget::ShowCombatPreview(const FConquestCombatPreviewData& PreviewData)
{
	CurrentCombatPreviewData = PreviewData;

	if (!PreviewData.bIsValid)
	{
		ClearCombatPreview();
		return;
	}

	SetWidgetVisibility(CombatPreviewPanel, ESlateVisibility::Visible);
	SetText(
		CombatPreviewTitleText,
		FText::Format(
			NSLOCTEXT("Conquest", "CombatPreviewTitleFormat", "{0} vs {1}"),
			PreviewData.AttackerName.IsEmpty() ? FText::FromString(TEXT("Attacker")) : PreviewData.AttackerName,
			PreviewData.DefenderName.IsEmpty() ? FText::FromString(TEXT("Defender")) : PreviewData.DefenderName
		)
	);
	SetText(CombatPreviewRatingText, PreviewData.RatingText);
	SetText(
		CombatPreviewDamageText,
		FText::Format(
			NSLOCTEXT("Conquest", "CombatPreviewDamageFormat", "Deal {0} damage | Take {1} damage"),
			FText::AsNumber(PreviewData.DamageDealt),
			FText::AsNumber(PreviewData.DamageTaken)
		)
	);
	SetText(
		CombatPreviewHealthText,
		FText::Format(
			NSLOCTEXT("Conquest", "CombatPreviewHealthFormat", "{0} HP: {1}/{2} -> {3}/{2} | {4} HP: {5}/{6} -> {7}/{6}"),
			PreviewData.AttackerName.IsEmpty() ? FText::FromString(TEXT("Attacker")) : PreviewData.AttackerName,
			FText::AsNumber(PreviewData.AttackerCurrentHealth),
			FText::AsNumber(PreviewData.AttackerMaxHealth),
			FText::AsNumber(PreviewData.AttackerProjectedHealth),
			PreviewData.DefenderName.IsEmpty() ? FText::FromString(TEXT("Defender")) : PreviewData.DefenderName,
			FText::AsNumber(PreviewData.DefenderCurrentHealth),
			FText::AsNumber(PreviewData.DefenderMaxHealth),
			FText::AsNumber(PreviewData.DefenderProjectedHealth)
		)
	);
	SetText(
		CombatPreviewDetailText,
		PreviewData.bHasCounterAttack
			? NSLOCTEXT("Conquest", "CombatPreviewCounterAttack", "Melee counterattack expected")
			: NSLOCTEXT("Conquest", "CombatPreviewNoCounterAttack", "No counterattack expected")
	);

	if (CombatPreviewModifiersText)
	{
		TArray<FString> ModifierLines;

		for (const FText& ModifierText : PreviewData.ModifierTexts)
		{
			if (ModifierText.IsEmpty())
			{
				continue;
			}

			ModifierLines.Add(ModifierText.ToString());
		}

		SetText(CombatPreviewModifiersText, FString::Join(ModifierLines, TEXT("\n")));
		const ESlateVisibility ModifierVisibility = ModifierLines.Num() > 0
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed;
		SetWidgetVisibility(CombatPreviewModifiersPanel, ModifierVisibility);
		SetWidgetVisibility(
			CombatPreviewModifiersText,
			ModifierVisibility
		);
	}
}

void UConquestGameWidget::ClearCombatPreview()
{
	CurrentCombatPreviewData = FConquestCombatPreviewData();
	SetWidgetVisibility(CombatPreviewPanel, ESlateVisibility::Collapsed);
	ClearText(CombatPreviewTitleText);
	ClearText(CombatPreviewRatingText);
	ClearText(CombatPreviewDamageText);
	ClearText(CombatPreviewHealthText);
	ClearText(CombatPreviewDetailText);
	if (CombatPreviewModifiersText)
	{
		ClearText(CombatPreviewModifiersText);
		SetWidgetVisibility(CombatPreviewModifiersText, ESlateVisibility::Collapsed);
	}
	SetWidgetVisibility(CombatPreviewModifiersPanel, ESlateVisibility::Collapsed);
}

void UConquestGameWidget::HandleTileExpansionConfirmClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		if (!ConquestHUD->ConfirmSelectedStartingCity())
		{
			ConquestHUD->ConfirmSelectedExpansionTile();
		}
	}
}

void UConquestGameWidget::HandleTileExpansionCancelClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		if (PendingTileExpansionChoice.bIsValid && PendingTileExpansionChoice.CityId == INDEX_NONE)
		{
			ConquestHUD->CancelSelectedStartingCity();
		}
		else
		{
			ConquestHUD->CancelSelectedExpansionTile();
		}
	}
	else
	{
		ClearTileExpansionConfirmation();
	}
}

void UConquestGameWidget::HandleEndGameReturnToMenuClicked()
{
	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestReturnToMainMenu();
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

void UConquestGameWidget::HandleUnitAugmentChoiceClicked(const FConquestChoiceButtonData& ChoiceData)
{
	if (
		PendingUnitAugmentUnitInstanceId == INDEX_NONE ||
		ChoiceData.ChoiceType != EConquestChoiceType::UnitAugment
	)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		ConquestHUD->PurchaseSelectedUnitAugment(ChoiceData.ChoiceId);
	}
}

void UConquestGameWidget::HandleUnitAugmentCloseClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (ConquestHUD)
	{
		ConquestHUD->ClearUnitAugmentChoices();
	}
	else
	{
		ClearUnitAugmentChoices();
	}
}

void UConquestGameWidget::HandleUnitActionClicked(FName ActionId)
{
	APlayerController* PC = GetOwningPlayer();
	AConquestHUD* ConquestHUD = PC ? Cast<AConquestHUD>(PC->GetHUD()) : nullptr;
	if (!ConquestHUD)
	{
		return;
	}

	if (ActionId == FName(TEXT("Move")))
	{
		ConquestHUD->EnterSelectedUnitMoveMode();
	}
	else if (ActionId == FName(TEXT("Attack")))
	{
		ConquestHUD->EnterSelectedUnitAttackMode();
	}
	else if (ActionId == FName(TEXT("Augment")))
	{
		ConquestHUD->ShowAugmentChoicesForSelectedUnit();
	}
	else if (ActionId == FName(TEXT("Fortify")))
	{
		ConquestHUD->FortifySelectedUnit();
	}
	else if (ActionId == FName(TEXT("DoNothing")))
	{
		ConquestHUD->DoNothingSelectedUnit();
	}
	else if (ActionId == FName(TEXT("Sleep")))
	{
		ConquestHUD->SleepSelectedUnit();
	}
	else if (ActionId == FName(TEXT("Settle")))
	{
		ConquestHUD->SettleSelectedUnit();
	}
	else if (ActionId == FName(TEXT("Disband")))
	{
		ConquestHUD->DisbandSelectedUnit();
	}
}

void UConquestGameWidget::UpdateHoveredTileInfo(const FHoveredHexTileWidgetData& HoveredTileData)
{
	SetWidgetVisibility(HoveredTileInfoPanel, ESlateVisibility::Visible);
	SetWidgetVisibility(TileDetailsBox, ESlateVisibility::Visible);
	SetWidgetVisibility(TileYieldsBox, ESlateVisibility::Visible);

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
	SetWidgetVisibility(HoveredTileInfoPanel, ESlateVisibility::Collapsed);
	ClearTileTexts();
}
