#include "ConquestHUD.h"

#include "ConquestCityPanelWidget.h"
#include "Blueprint/UserWidget.h"
#include "ConquestHUDWidget.h"
#include "ConquestGameSetupWidget.h"
#include "ConquestGameWidget.h"
#include "ConquestMainMenuWidget.h"
#include "ConquestResearchPanelWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Units/ConquestUnitActor.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexImprovementTypes.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "Conquest/World/Generation/ModularHexGridActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	FString ConquestHUDHexTileTypeToString(EHexTileType TileType)
	{
		if (const UEnum* EnumPtr = StaticEnum<EHexTileType>())
		{
			return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(TileType)).ToString();
		}

		return TEXT("Unknown");
	}

	FString ConquestHUDHexFeatureTypeToString(EHexFeatureType FeatureType)
	{
		if (const UEnum* EnumPtr = StaticEnum<EHexFeatureType>())
		{
			return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(FeatureType)).ToString();
		}

		return TEXT("Unknown");
	}

	FString ConquestHUDFormatTileFeatures(const FHexTileData& TileData)
	{
		TArray<FString> FeatureNames;

		for (const EHexFeatureType Feature : TileData.Features)
		{
			if (Feature != EHexFeatureType::None)
			{
				FeatureNames.Add(ConquestHUDHexFeatureTypeToString(Feature));
			}
		}

		return FeatureNames.Num() > 0
			? FString::Join(FeatureNames, TEXT(", "))
			: TEXT("None");
	}

	FString ConquestHUDFormatTileResource(const FHexTileData& TileData)
	{
		if (!TileData.Resource.HasResource())
		{
			return TEXT("None");
		}

		return TileData.Resource.Quantity > 0
			? FString::Printf(TEXT("%s x%d"), *TileData.Resource.ResourceId.ToString(), TileData.Resource.Quantity)
			: TileData.Resource.ResourceId.ToString();
	}

	bool ConquestHUDHasDifficultFeature(const FHexTileData& TileData)
	{
		return
			TileData.Features.Contains(EHexFeatureType::Forest) ||
			TileData.Features.Contains(EHexFeatureType::Jungle) ||
			TileData.Features.Contains(EHexFeatureType::Hill) ||
			TileData.Features.Contains(EHexFeatureType::Marsh);
	}

	bool ConquestHUDIsImpassableForLandUnit(const FHexGridModel& GridModel, const FHexTileData& TileData)
	{
		return GridModel.IsWaterTileType(TileData.TileType) || TileData.TileType == EHexTileType::Mountain;
	}

	FConquestUnitState* ConquestHUDFindUnitMutable(
		FConquestPlayerEmpireState& Player,
		int32 UnitInstanceId
	)
	{
		return Player.Units.FindByPredicate([UnitInstanceId](const FConquestUnitState& Unit)
		{
			return Unit.UnitInstanceId == UnitInstanceId;
		});
	}
}

AConquestHUD::AConquestHUD()
{
	HUDWidgetClass = UConquestHUDWidget::StaticClass();
	MainMenuWidgetClass = UConquestMainMenuWidget::StaticClass();
	GameSetupWidgetClass = UConquestGameSetupWidget::StaticClass();
	GameWidgetClass = UConquestGameWidget::StaticClass();
	HexGridActorClass = AModularHexGridActor::StaticClass();
}

void AConquestHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (!HUDWidgetClass)
	{
		return;
	}

	HUDWidget = CreateWidget<UConquestHUDWidget>(PlayerController, HUDWidgetClass);
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->InitializeHUDWidget(
		this,
		MainMenuWidgetClass,
		GameSetupWidgetClass,
		GameWidgetClass
	);

	if (CityPanelWidgetClass)
	{
		CityPanelWidget = CreateWidget<UConquestCityPanelWidget>(PlayerController, CityPanelWidgetClass);
		if (CityPanelWidget)
		{
			CityPanelWidget->AddToViewport(5);
			CityPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (ResearchPanelWidgetClass)
	{
		ResearchPanelWidget = CreateWidget<UConquestResearchPanelWidget>(PlayerController, ResearchPanelWidgetClass);
		if (ResearchPanelWidget)
		{
			ResearchPanelWidget->AddToViewport(6);
			ResearchPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	ShowMainMenu();
}

void AConquestHUD::ShowMainMenu()
{
	ConfigureMenuInputMode();

	if (HUDWidget)
	{
		HUDWidget->ShowMainMenu();
	}
}

void AConquestHUD::ShowCityPanel(int32 CityId)
{
	if (!CityPanelWidget)
	{
		return;
	}

	ClearTileImprovementChoices();
	CityPanelWidget->SetCity(CityId);
	CityPanelWidget->SetVisibility(ESlateVisibility::Visible);
	SetCityWorldLabelHiddenForPanel(CityId);

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->SetSelectedCityYieldContext(CityId);
	}

	BeginCityTileExpansionSelection(CityId);
}

void AConquestHUD::HideCityPanel()
{
	if (CityPanelWidget)
	{
		CityPanelWidget->ClosePanel();
	}

	RestoreHiddenCityWorldLabel();

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearSelectedCityYieldContext();
	}

	ClearCityTileExpansionSelection();
	ClearTileImprovementChoices();
}

void AConquestHUD::SetCityWorldLabelHiddenForPanel(int32 CityId)
{
	if (HiddenCityWorldLabelId == CityId)
	{
		AConquestGameState* ConquestGS = GetWorld()
			? GetWorld()->GetGameState<AConquestGameState>()
			: nullptr;

		if (ConquestGS && ConquestGS->ActiveGridActor)
		{
			ConquestGS->ActiveGridActor->SetCityWorldLabelVisible(CityId, false);
		}

		return;
	}

	RestoreHiddenCityWorldLabel();

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->ActiveGridActor || CityId == INDEX_NONE)
	{
		return;
	}

	ConquestGS->ActiveGridActor->SetCityWorldLabelVisible(CityId, false);
	HiddenCityWorldLabelId = CityId;
}

void AConquestHUD::RestoreHiddenCityWorldLabel()
{
	if (HiddenCityWorldLabelId == INDEX_NONE)
	{
		return;
	}

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (ConquestGS && ConquestGS->ActiveGridActor)
	{
		ConquestGS->ActiveGridActor->SetCityWorldLabelVisible(HiddenCityWorldLabelId, true);
	}

	HiddenCityWorldLabelId = INDEX_NONE;
}

void AConquestHUD::BeginCityTileExpansionSelection(int32 CityId)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->CityManager || !ConquestGS->ActiveGridActor)
	{
		return;
	}

	const FCityState* City = ConquestGS->CityManager->GetCity(CityId);
	if (!City || City->PendingBorderExpansions <= 0)
	{
		ClearCityTileExpansionSelection();
		return;
	}

	ExpansionSelectionCityId = CityId;
	PendingExpansionTileCoord = FIntPoint(INT32_MIN, INT32_MIN);

	const TArray<FIntPoint> Candidates = ConquestGS->CityManager->GetExpansionCandidateTiles(CityId);
	ConquestGS->ActiveGridActor->RebuildExpansionCandidateHighlights(Candidates);

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearTileExpansionConfirmation();
	}
}

bool AConquestHUD::SelectExpansionCandidateTile(int32 Q, int32 R)
{
	if (ExpansionSelectionCityId == INDEX_NONE)
	{
		return false;
	}

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return false;
	}

	const FIntPoint Coord(Q, R);
	const TArray<FIntPoint> Candidates =
		ConquestGS->CityManager->GetExpansionCandidateTiles(ExpansionSelectionCityId);

	if (!Candidates.Contains(Coord))
	{
		return false;
	}

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	const FHexTileData* Tile = GridModel ? GridModel->GetTile(Coord) : nullptr;
	if (!Tile)
	{
		return false;
	}

	PendingExpansionTileCoord = Coord;

	int32 CurrentAssignedCitizens = 0;
	bool bAssigningToOwnedTile = false;
	if (const FCityState* City = ConquestGS->CityManager->GetCity(ExpansionSelectionCityId))
	{
		bAssigningToOwnedTile = City->OwnedTiles.Contains(Coord);
		for (const FCityWorkedTileAssignment& Assignment : City->WorkedTileAssignments)
		{
			if (Assignment.Coord == Coord)
			{
				CurrentAssignedCitizens = Assignment.Citizens;
				break;
			}
		}
	}

	FConquestTileExpansionChoiceData ChoiceData;
	ChoiceData.CityId = ExpansionSelectionCityId;
	ChoiceData.Coord = Coord;
	ChoiceData.TileType = ConquestHUDHexTileTypeToString(Tile->TileType);
	ChoiceData.Resource = ConquestHUDFormatTileResource(*Tile);
	ChoiceData.Features = ConquestHUDFormatTileFeatures(*Tile);
	ChoiceData.Yield = Tile->FinalYield;
	ChoiceData.bAssigningToOwnedTile = bAssigningToOwnedTile;
	ChoiceData.CurrentAssignedCitizens = CurrentAssignedCitizens;
	ChoiceData.ResultAssignedCitizens = bAssigningToOwnedTile
		? FMath::Clamp(CurrentAssignedCitizens + 1, 1, 3)
		: 1;
	ChoiceData.bIsValid = true;

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ShowTileExpansionConfirmation(ChoiceData);
	}

	return true;
}

bool AConquestHUD::ConfirmSelectedExpansionTile()
{
	if (ExpansionSelectionCityId == INDEX_NONE || PendingExpansionTileCoord == FIntPoint(INT32_MIN, INT32_MIN))
	{
		return false;
	}

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return false;
	}

	const bool bClaimed = ConquestGS->CityManager->ClaimExpansionTileForCity(
		ExpansionSelectionCityId,
		PendingExpansionTileCoord
	);

	if (!bClaimed)
	{
		return false;
	}

	if (CityPanelWidget)
	{
		CityPanelWidget->Refresh();
	}

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->SetSelectedCityYieldContext(ExpansionSelectionCityId);
		ActiveGameWidget->ClearTileExpansionConfirmation();
	}

	BeginCityTileExpansionSelection(ExpansionSelectionCityId);
	return true;
}

void AConquestHUD::CancelSelectedExpansionTile()
{
	PendingExpansionTileCoord = FIntPoint(INT32_MIN, INT32_MIN);

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearTileExpansionConfirmation();
	}
}

void AConquestHUD::ClearCityTileExpansionSelection()
{
	ExpansionSelectionCityId = INDEX_NONE;
	PendingExpansionTileCoord = FIntPoint(INT32_MIN, INT32_MIN);

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (ConquestGS && ConquestGS->ActiveGridActor)
	{
		ConquestGS->ActiveGridActor->ClearExpansionCandidateHighlights();
	}

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearTileExpansionConfirmation();
	}
}

bool AConquestHUD::ShowTileImprovementChoicesForTile(int32 Q, int32 R)
{
	if (!CityPanelWidget || !CityPanelWidget->IsVisible())
	{
		return false;
	}

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return false;
	}

	const FIntPoint Coord(Q, R);
	if (ConquestGS->CityManager->FindCityAtTile(Coord) != INDEX_NONE)
	{
		return false;
	}

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	const FHexTileData* Tile = GridModel ? GridModel->GetTile(Coord) : nullptr;
	const int32 PlayerId = ConquestGS->HumanPlayer.PlayerId;
	if (!GridModel || !Tile || Tile->Gameplay.OwnerPlayerId != PlayerId || !Tile->ImprovementId.IsNone())
	{
		ClearTileImprovementChoices();
		return false;
	}

	TArray<FName> AvailableImprovementIds =
		ConquestGS->CityManager->GetAvailableTileImprovementIdsForPlayer(PlayerId, Coord);

	if (AvailableImprovementIds.Num() <= 0)
	{
		ClearTileImprovementChoices();
		return false;
	}

	TArray<const FHexImprovementDefinition*> PossibleImprovements;
	GridModel->GetPossibleImprovementsForTile(Q, R, PossibleImprovements);

	TArray<FConquestChoiceButtonData> ImprovementChoices;
	for (const FHexImprovementDefinition* Improvement : PossibleImprovements)
	{
		if (!Improvement || !AvailableImprovementIds.Contains(Improvement->ImprovementId))
		{
			continue;
		}

		const int32 GoldCost = FMath::Max(0, Improvement->PurchaseGoldCost);
		const bool bCanAfford = ConquestGS->HumanPlayer.StoredYields.Gold >= GoldCost;

		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::TileImprovement;
		ChoiceData.ChoiceId = Improvement->ImprovementId;
		ChoiceData.Title = !Improvement->DisplayName.IsEmpty()
			? Improvement->DisplayName
			: FText::FromName(Improvement->ImprovementId);
		ChoiceData.Cost = GoldCost;
		ChoiceData.bEnabled = bCanAfford;
		ChoiceData.Subtitle = FText::Format(
			NSLOCTEXT("Conquest", "TileImprovementChoiceSubtitle", "{0} Gold | Gain: {1}"),
			FText::AsNumber(GoldCost),
			FText::FromString(Improvement->YieldModifier.ToCompactString())
		);

		if (!bCanAfford)
		{
			ChoiceData.DisabledReason = NSLOCTEXT("Conquest", "TileImprovementNotEnoughGold", "Not enough gold");
			ChoiceData.DetailText = ChoiceData.DisabledReason;
		}

		ImprovementChoices.Add(ChoiceData);
	}

	if (ImprovementChoices.Num() <= 0)
	{
		ClearTileImprovementChoices();
		return false;
	}

	PendingImprovementTileCoord = Coord;

	FConquestTileImprovementChoiceData ChoiceData;
	ChoiceData.Coord = Coord;
	ChoiceData.bIsValid = true;

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ShowTileImprovementChoices(ChoiceData, ImprovementChoices);
	}

	return true;
}

bool AConquestHUD::PurchaseSelectedTileImprovement(FName ImprovementId)
{
	if (PendingImprovementTileCoord == FIntPoint(INT32_MIN, INT32_MIN))
	{
		return false;
	}

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return false;
	}

	const int32 PlayerId = ConquestGS->HumanPlayer.PlayerId;
	const bool bPurchased = ConquestGS->CityManager->PurchaseTileImprovementForPlayer(
		PlayerId,
		PendingImprovementTileCoord,
		ImprovementId
	);

	if (!bPurchased)
	{
		return false;
	}

	if (CityPanelWidget)
	{
		CityPanelWidget->Refresh();
	}

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->RefreshTopBarYieldInfo();
	}

	ClearTileImprovementChoices();
	return true;
}

void AConquestHUD::ClearTileImprovementChoices()
{
	PendingImprovementTileCoord = FIntPoint(INT32_MIN, INT32_MIN);

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearTileImprovementChoices();
	}
}

bool AConquestHUD::SelectUnitAtTile(int32 Q, int32 R)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->ContentManager)
	{
		return false;
	}

	const FIntPoint Coord(Q, R);
	const FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayer();

	const FConquestUnitState* UnitToSelect = nullptr;
	for (const FConquestUnitState& Unit : Player.Units)
	{
		if (Unit.OwnerPlayerId == Player.PlayerId && Unit.TileCoord == Coord)
		{
			UnitToSelect = &Unit;
			break;
		}
	}

	if (!UnitToSelect)
	{
		return false;
	}

	ClearUnitSelection();

	ConquestGS->SelectedUnitInstanceId = UnitToSelect->UnitInstanceId;

	UMaterialInterface* SelectionMaterial = nullptr;
	if (const UConquestCivilisationData* Civilisation = ConquestGS->GetCivilisationForPlayer(Player.PlayerId))
	{
		SelectionMaterial = Civilisation->BorderMaterial;
	}

	if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = ConquestGS->UnitActorsByInstanceId.Find(UnitToSelect->UnitInstanceId))
	{
		if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
		{
			UnitActor->SetSelected(true, SelectionMaterial);
		}
	}

	RefreshSelectedUnitWidget(*UnitToSelect);
	if (!UnitToSelect->bIsSleeping)
	{
		EnterSelectedUnitMoveMode();
	}

	return true;
}

void AConquestHUD::ClearUnitSelection()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS)
	{
		return;
	}

	if (ConquestGS->SelectedUnitInstanceId != INDEX_NONE)
	{
		if (TObjectPtr<AConquestUnitActor>* UnitActorPtr =
			ConquestGS->UnitActorsByInstanceId.Find(ConquestGS->SelectedUnitInstanceId))
		{
			if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
			{
				UnitActor->SetSelected(false);
			}
		}
	}

	ConquestGS->SelectedUnitInstanceId = INDEX_NONE;
	ClearUnitMovementHighlights();

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearSelectedUnitInfo();
	}
}

void AConquestHUD::RefreshSelectedUnitWidget(const FConquestUnitState& UnitState)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->ContentManager)
	{
		return;
	}

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		FConquestSelectedUnitWidgetData UnitWidgetData;
		UnitWidgetData.UnitInstanceId = UnitState.UnitInstanceId;
		UnitWidgetData.bIsValid = true;

		const FConquestUnitRow* UnitRow = ConquestGS->ContentManager->FindUnit(UnitState.UnitId);
		UnitWidgetData.UnitName = UnitRow && !UnitRow->DisplayName.IsEmpty()
			? UnitRow->DisplayName
			: FText::FromName(UnitState.UnitId);
		UnitWidgetData.bCanFoundCity = UnitRow && UnitRow->bCanFoundCity;
		UnitWidgetData.HealthText = FText::Format(
			NSLOCTEXT("Conquest", "SelectedUnitHealthMovementFormat", "{0}/{1} HP | {2}/{3} Move"),
			FText::AsNumber(UnitState.CurrentHealth),
			FText::AsNumber(UnitState.CachedMaxHealth),
			FText::AsNumber(UnitState.CurrentMovementPoints),
			FText::AsNumber(UnitState.CachedMovementPoints)
		);

		ActiveGameWidget->ShowSelectedUnitInfo(UnitWidgetData);
	}
}

void AConquestHUD::ClearUnitMovementHighlights()
{
	CurrentUnitMovementRemainingByTile.Reset();

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (ConquestGS && ConquestGS->ActiveGridActor)
	{
		ConquestGS->ActiveGridActor->ClearExpansionCandidateHighlights();
	}
}

bool AConquestHUD::EnterSelectedUnitMoveMode()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->ActiveGridActor || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit || SelectedUnit->CurrentMovementPoints <= 0)
	{
		ClearUnitMovementHighlights();
		return false;
	}

	SelectedUnit->bIsSleeping = false;

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	if (!GridModel || !GridModel->IsValidTile(SelectedUnit->TileCoord.X, SelectedUnit->TileCoord.Y))
	{
		ClearUnitMovementHighlights();
		return false;
	}

	CurrentUnitMovementRemainingByTile.Reset();
	TMap<FIntPoint, int32> BestRemainingByCoord;
	TArray<FIntPoint> Frontier;

	BestRemainingByCoord.Add(SelectedUnit->TileCoord, SelectedUnit->CurrentMovementPoints);
	Frontier.Add(SelectedUnit->TileCoord);

	auto IsOccupiedByAnotherUnit = [&Player, SelectedUnit](const FIntPoint& Coord)
	{
		for (const FConquestUnitState& Unit : Player.Units)
		{
			if (Unit.UnitInstanceId != SelectedUnit->UnitInstanceId && Unit.TileCoord == Coord)
			{
				return true;
			}
		}

		return false;
	};

	auto GetMoveCost = [GridModel](const FIntPoint& FromCoord, const FIntPoint& ToCoord)
	{
		const FHexTileData* FromTile = GridModel->GetTile(FromCoord);
		const FHexTileData* ToTile = GridModel->GetTile(ToCoord);
		if (!FromTile || !ToTile)
		{
			return TNumericLimits<int32>::Max();
		}

		if (ConquestHUDIsImpassableForLandUnit(*GridModel, *ToTile))
		{
			return TNumericLimits<int32>::Max();
		}

		int32 Cost = 1;
		if (
			ToTile->TileType == EHexTileType::Jungle ||
			ConquestHUDHasDifficultFeature(*ToTile) ||
			FromTile->bHasRiver ||
			ToTile->bHasRiver
		)
		{
			Cost = 2;
		}

		return Cost;
	};

	for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
	{
		const FIntPoint CurrentCoord = Frontier[FrontierIndex];
		const int32 CurrentRemaining = BestRemainingByCoord.FindRef(CurrentCoord);

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NeighborQ = INDEX_NONE;
			int32 NeighborR = INDEX_NONE;
			if (!GridModel->GetNeighbourCoord(CurrentCoord.X, CurrentCoord.Y, Direction, NeighborQ, NeighborR))
			{
				continue;
			}

			const FIntPoint NeighborCoord(NeighborQ, NeighborR);
			if (IsOccupiedByAnotherUnit(NeighborCoord))
			{
				continue;
			}

			const int32 MoveCost = GetMoveCost(CurrentCoord, NeighborCoord);
			if (MoveCost == TNumericLimits<int32>::Max())
			{
				continue;
			}

			const bool bOneTileMinimumMove =
				CurrentCoord == SelectedUnit->TileCoord &&
				CurrentRemaining > 0;
			if (CurrentRemaining < MoveCost && !bOneTileMinimumMove)
			{
				continue;
			}

			const int32 NewRemaining = FMath::Max(0, CurrentRemaining - MoveCost);
			const int32* ExistingRemaining = BestRemainingByCoord.Find(NeighborCoord);
			if (ExistingRemaining && *ExistingRemaining >= NewRemaining)
			{
				continue;
			}

			BestRemainingByCoord.Add(NeighborCoord, NewRemaining);
			CurrentUnitMovementRemainingByTile.Add(NeighborCoord, NewRemaining);

			if (NewRemaining > 0)
			{
				Frontier.Add(NeighborCoord);
			}
		}
	}

	TArray<FIntPoint> ReachableTiles;
	CurrentUnitMovementRemainingByTile.GetKeys(ReachableTiles);

	UMaterialInterface* HighlightMaterial = nullptr;
	if (const UConquestCivilisationData* Civilisation = ConquestGS->GetCivilisationForPlayer(Player.PlayerId))
	{
		HighlightMaterial = Civilisation->BorderMaterial;
	}

	ConquestGS->ActiveGridActor->RebuildExpansionCandidateHighlights(ReachableTiles, HighlightMaterial);
	return ReachableTiles.Num() > 0;
}

bool AConquestHUD::TryMoveSelectedUnitToTile(int32 Q, int32 R)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->ContentManager || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit)
	{
		return false;
	}

	const FIntPoint TargetCoord(Q, R);
	if (TargetCoord == SelectedUnit->TileCoord)
	{
		return false;
	}

	if (!CurrentUnitMovementRemainingByTile.Contains(TargetCoord))
	{
		EnterSelectedUnitMoveMode();
	}

	const int32* NewRemainingMovement = CurrentUnitMovementRemainingByTile.Find(TargetCoord);
	if (!NewRemainingMovement)
	{
		return false;
	}

	SelectedUnit->TileCoord = TargetCoord;
	SelectedUnit->CurrentMovementPoints = FMath::Clamp(
		*NewRemainingMovement,
		0,
		SelectedUnit->CachedMovementPoints
	);
	SelectedUnit->bIsFortified = false;
	SelectedUnit->bIsSleeping = false;

	if (TObjectPtr<AConquestUnitActor>* UnitActorPtr =
		ConquestGS->UnitActorsByInstanceId.Find(SelectedUnit->UnitInstanceId))
	{
		if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
		{
			UnitActor->MoveToTile(TargetCoord);
		}
	}

	RefreshSelectedUnitWidget(*SelectedUnit);

	if (SelectedUnit->CurrentMovementPoints > 0)
	{
		EnterSelectedUnitMoveMode();
	}
	else
	{
		ClearUnitMovementHighlights();
	}

	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestHUD::IsSelectedUnitMovementTile(int32 Q, int32 R) const
{
	return CurrentUnitMovementRemainingByTile.Contains(FIntPoint(Q, R));
}

bool AConquestHUD::FortifySelectedUnit()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit)
	{
		return false;
	}

	if (SelectedUnit->CurrentMovementPoints <= 0)
	{
		return false;
	}

	SelectedUnit->bIsFortified = true;
	SelectedUnit->bIsSleeping = false;
	SelectedUnit->CurrentMovementPoints = FMath::Max(0, SelectedUnit->CurrentMovementPoints - 1);
	ClearUnitMovementHighlights();
	RefreshSelectedUnitWidget(*SelectedUnit);
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestHUD::DoNothingSelectedUnit()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit || SelectedUnit->CurrentMovementPoints <= 0)
	{
		return false;
	}

	SelectedUnit->bIsFortified = false;
	SelectedUnit->bIsSleeping = false;
	SelectedUnit->CurrentMovementPoints = 0;
	ClearUnitMovementHighlights();
	RefreshSelectedUnitWidget(*SelectedUnit);
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestHUD::SleepSelectedUnit()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit)
	{
		return false;
	}

	SelectedUnit->bIsSleeping = true;
	SelectedUnit->bIsFortified = false;
	ClearUnitMovementHighlights();
	RefreshSelectedUnitWidget(*SelectedUnit);
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestHUD::DisbandSelectedUnit()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	const int32 UnitInstanceId = ConquestGS->SelectedUnitInstanceId;

	if (TObjectPtr<AConquestUnitActor>* UnitActorPtr =
		ConquestGS->UnitActorsByInstanceId.Find(UnitInstanceId))
	{
		if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
		{
			UnitActor->Destroy();
		}

		ConquestGS->UnitActorsByInstanceId.Remove(UnitInstanceId);
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	Player.Units.RemoveAll([UnitInstanceId](const FConquestUnitState& Unit)
	{
		return Unit.UnitInstanceId == UnitInstanceId;
	});

	ClearUnitSelection();

	if (ConquestGS->YieldManager)
	{
		ConquestGS->YieldManager->RecalculateEmpireYieldPerTurn(Player.PlayerId);
	}

	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestHUD::SettleSelectedUnit()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (
		!ConquestGS ||
		!ConquestGS->CityManager ||
		!ConquestGS->ContentManager ||
		ConquestGS->SelectedUnitInstanceId == INDEX_NONE
	)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit)
	{
		return false;
	}

	if (SelectedUnit->CurrentMovementPoints <= 0)
	{
		return false;
	}

	const FConquestUnitRow* UnitRow = ConquestGS->ContentManager->FindUnit(SelectedUnit->UnitId);
	if (!UnitRow || !UnitRow->bCanFoundCity)
	{
		return false;
	}

	SelectedUnit->bIsSleeping = false;

	const int32 UnitInstanceId = SelectedUnit->UnitInstanceId;
	const int32 OwnerPlayerId = SelectedUnit->OwnerPlayerId;
	const FIntPoint CityCoord = SelectedUnit->TileCoord;

	if (!ConquestGS->CityManager->FoundCity(OwnerPlayerId, CityCoord, NAME_None))
	{
		return false;
	}

	if (TObjectPtr<AConquestUnitActor>* UnitActorPtr =
		ConquestGS->UnitActorsByInstanceId.Find(UnitInstanceId))
	{
		if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
		{
			UnitActor->Destroy();
		}

		ConquestGS->UnitActorsByInstanceId.Remove(UnitInstanceId);
	}

	Player.Units.RemoveAll([UnitInstanceId](const FConquestUnitState& Unit)
	{
		return Unit.UnitInstanceId == UnitInstanceId;
	});

	const int32 NewCityId = ConquestGS->CityManager->FindCityAtTile(CityCoord);
	ClearUnitSelection();

	if (NewCityId != INDEX_NONE)
	{
		ShowCityPanel(NewCityId);
	}

	if (ConquestGS->YieldManager)
	{
		ConquestGS->YieldManager->RecalculateEmpireYieldPerTurn(Player.PlayerId);
	}

	ConquestGS->BroadcastStateChanged();
	return true;
}

void AConquestHUD::ShowResearchPanel()
{
	if (!ResearchPanelWidget)
	{
		return;
	}

	ResearchPanelWidget->RefreshResearchOptions();
	ResearchPanelWidget->SetVisibility(ESlateVisibility::Visible);
}

void AConquestHUD::HideResearchPanel()
{
	if (ResearchPanelWidget)
	{
		ResearchPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AConquestHUD::ShowGameSetup()
{
	ConfigureMenuInputMode();

	if (HUDWidget)
	{
		HUDWidget->ShowGameSetup();
	}
}

void AConquestHUD::ShowGame()
{
	ConfigureGameInputMode();

	if (HUDWidget)
	{
		HUDWidget->ShowGame();
	}

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->RefreshTopBarYieldInfo();
	}
}

void AConquestHUD::RequestStartGame(const FConquestGameSetupSettings& SetupSettings)
{
	UWorld* World = GetWorld();
	if (!World || !HexGridActorClass)
	{
		return;
	}

	if (SpawnedHexGridActor)
	{
		SpawnedHexGridActor->Destroy();
		SpawnedHexGridActor = nullptr;
	}

	AModularHexGridActor* NewGridActor = World->SpawnActorDeferred<AModularHexGridActor>(
		HexGridActorClass,
		HexGridSpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!NewGridActor)
	{
		return;
	}

	UGameplayStatics::FinishSpawningActor(NewGridActor, HexGridSpawnTransform);

	NewGridActor->ApplyGameSetupSettings(SetupSettings);

	SpawnedHexGridActor = NewGridActor;
	if (AConquestGameState* ConquestGS = World->GetGameState<AConquestGameState>())
	{
		ConquestGS->ActiveGridActor = SpawnedHexGridActor;
		ConquestGS->ApplyGameSetupSettings(SetupSettings);
	}

	if (AConquestGameMode* ConquestGM = World->GetAuthGameMode<AConquestGameMode>())
	{
		ConquestGM->StartSinglePlayerGame();
	}

	ShowGame();
}

UConquestGameWidget* AConquestHUD::GetGameWidget() const
{
	return HUDWidget ? HUDWidget->GetGameWidget() : nullptr;
}

UConquestGameWidget* AConquestHUD::GetActiveGameWidget() const
{
	if (!HUDWidget || !HUDWidget->IsGameWidgetActive())
	{
		return nullptr;
	}

	return HUDWidget->GetGameWidget();
}

void AConquestHUD::StartGameWithMapPreset(EHexMapTypePreset MapPreset)
{
	FConquestGameSetupSettings SetupSettings;
	SetupSettings.MapTypePreset = MapPreset;

	RequestStartGame(SetupSettings);
}

void AConquestHUD::ConfigureMenuInputMode()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}

void AConquestHUD::ConfigureGameInputMode()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	PlayerController->SetInputMode(InputMode);
}
