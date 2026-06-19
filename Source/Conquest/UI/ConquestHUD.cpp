#include "ConquestHUD.h"

#include "ConquestCityPanelWidget.h"
#include "Blueprint/UserWidget.h"
#include "ConquestHUDWidget.h"
#include "ConquestGameSetupWidget.h"
#include "ConquestGameWidget.h"
#include "ConquestMainMenuWidget.h"
#include "ConquestResearchPanelWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
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
