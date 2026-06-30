#include "ConquestHUD.h"

#include "ConquestCityPanelWidget.h"
#include "Blueprint/UserWidget.h"
#include "ConquestHUDWidget.h"
#include "ConquestGameSetupWidget.h"
#include "ConquestGameWidget.h"
#include "ConquestMainMenuWidget.h"
#include "ConquestResearchPanelWidget.h"
#include "ConquestPhilosophyPanelWidget.h"
#include "ConquestSettingsWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestModifierManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Player/ConquestPawn.h"
#include "Conquest/Player/ConquestPlayerController.h"
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
	constexpr int32 ConquestHUDStartingRegionFogRevealRadius = 7;
	constexpr int32 ConquestHUDCityFogRevealRadius = 5;
	constexpr int32 ConquestHUDCityOwnedTileFogRevealRadius = 1;
	constexpr int32 ConquestHUDUnitFogRevealRadius = 2;

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

	bool ConquestHUDIsOwnedByOtherPlayer(const FHexTileData& TileData, int32 PlayerId)
	{
		return
			TileData.Gameplay.OwnerPlayerId != INDEX_NONE &&
			TileData.Gameplay.OwnerPlayerId != PlayerId;
	}

	bool ConquestHUDIsEnemyCityTile(const AConquestGameState& GameState, int32 PlayerId, const FIntPoint& Coord)
	{
		if (!GameState.CityManager)
		{
			return false;
		}

		const int32 CityId = GameState.CityManager->FindCityAtTile(Coord);
		const FCityState* City = CityId != INDEX_NONE
			? GameState.CityManager->GetCity(CityId)
			: nullptr;

		return City && City->OwnerPlayerId != PlayerId;
	}

	bool ConquestHUDIsLocalOwnedCity(const AConquestGameState& GameState, int32 CityId)
	{
		const FCityState* City = CityId != INDEX_NONE && GameState.CityManager
			? GameState.CityManager->GetCity(CityId)
			: nullptr;

		return City && City->OwnerPlayerId == GameState.GetLocalPlayerId();
	}

	float ConquestHUDGetCityHealthCombatMultiplier(const FCityState& City)
	{
		return FMath::Clamp(
			static_cast<float>(FMath::Max(1, City.CurrentHealth)) /
			static_cast<float>(FMath::Max(1, City.MaxHealth)),
			0.01f,
			1.0f
		);
	}

	float ConquestHUDGetCityCombatValue(const FCityState& City)
	{
		return static_cast<float>(FMath::Max(
			1,
			FMath::CeilToInt(static_cast<float>(FMath::Max(1, City.CachedStrength)) * ConquestHUDGetCityHealthCombatMultiplier(City))
		));
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

	FText ConquestHUDGetUnitDisplayName(
		const AConquestGameState& GameState,
		const FConquestUnitState& UnitState
	)
	{
		if (GameState.ContentManager)
		{
			const FConquestUnitRow* UnitRow = GameState.ContentManager->FindUnit(UnitState.UnitId);
			if (UnitRow && !UnitRow->DisplayName.IsEmpty())
			{
				return UnitRow->DisplayName;
			}
		}

		return FText::FromName(UnitState.UnitId);
	}

	void ConquestHUDAppendHappinessCombatModifierText(
		TArray<FText>& OutModifierTexts,
		const FText& CombatantLabel,
		const FConquestPlayerEmpireState& Player
	)
	{
		if (!ConquestHappiness::IsUnhappy(Player.CachedHappiness))
		{
			return;
		}

		const FText PenaltyText = ConquestHappiness::GetPenaltyText(Player.CachedHappiness);
		if (PenaltyText.IsEmpty())
		{
			return;
		}

		OutModifierTexts.Add(FText::Format(
			NSLOCTEXT("Conquest", "CombatPreviewModifierFormat", "{0}: {1}"),
			CombatantLabel,
			PenaltyText
		));
	}

	bool ConquestHUDIsHappinessCombatModifier(FName ModifierId)
	{
		return
			ModifierId == FName(TEXT("Happiness_Unhappy_Attack")) ||
			ModifierId == FName(TEXT("Happiness_Unhappy_Defense"));
	}

	FString ConquestHUDFormatCombatModifierName(FName ModifierId)
	{
		if (ModifierId == FName(TEXT("Tile_Defense_Modifier")))
		{
			return TEXT("Tile Defense");
		}

		FString Result = ModifierId.IsNone()
			? FString(TEXT("Modifier"))
			: ModifierId.ToString();
		Result.ReplaceInline(TEXT("_"), TEXT(" "));
		return Result;
	}

	void ConquestHUDAppendUnitCombatModifierTexts(
		TArray<FText>& OutModifierTexts,
		const FText& CombatantLabel,
		const FConquestUnitState& Unit,
		EConquestUnitCombatModifierType ModifierType
	)
	{
		for (const FConquestUnitCombatModifier& Modifier : Unit.CombatModifiers)
		{
			if (
				Modifier.ModifierType != ModifierType ||
				ConquestHUDIsHappinessCombatModifier(Modifier.ModifierId)
			)
			{
				continue;
			}

			TArray<FString> ModifierParts;
			if (Modifier.FlatBonus != 0)
			{
				ModifierParts.Add(FString::Printf(TEXT("%+d"), Modifier.FlatBonus));
			}

			if (!FMath::IsNearlyEqual(Modifier.Multiplier, 1.0f))
			{
				const int32 Percent = FMath::RoundToInt((Modifier.Multiplier - 1.0f) * 100.0f);
				if (Percent != 0)
				{
					ModifierParts.Add(FString::Printf(TEXT("%+d%%"), Percent));
				}
			}

			if (ModifierParts.Num() <= 0)
			{
				continue;
			}

			OutModifierTexts.Add(FText::FromString(FString::Printf(
				TEXT("%s: %s (%s)"),
				*CombatantLabel.ToString(),
				*ConquestHUDFormatCombatModifierName(Modifier.ModifierId),
				*FString::Join(ModifierParts, TEXT(", "))
			)));
		}
	}
}

AConquestHUD::AConquestHUD()
{
	HUDWidgetClass = UConquestHUDWidget::StaticClass();
	MainMenuWidgetClass = UConquestMainMenuWidget::StaticClass();
	GameSetupWidgetClass = UConquestGameSetupWidget::StaticClass();
	GameWidgetClass = UConquestGameWidget::StaticClass();
	SettingsMenuWidgetClass = UConquestSettingsWidget::StaticClass();
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

	if (PhilosophyPanelWidgetClass)
	{
		PhilosophyPanelWidget = CreateWidget<UConquestPhilosophyPanelWidget>(PlayerController, PhilosophyPanelWidgetClass);
		if (PhilosophyPanelWidget)
		{
			PhilosophyPanelWidget->AddToViewport(7);
			PhilosophyPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (SettingsMenuWidgetClass)
	{
		SettingsMenuWidget = CreateWidget<UConquestSettingsWidget>(PlayerController, SettingsMenuWidgetClass);
		if (SettingsMenuWidget)
		{
			SettingsMenuWidget->AddToViewport(50);
			SettingsMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		ConquestGS->OnConquestStateChanged.RemoveDynamic(this, &AConquestHUD::HandleConquestStateChangedForHUD);
		ConquestGS->OnConquestStateChanged.AddDynamic(this, &AConquestHUD::HandleConquestStateChangedForHUD);
		ConquestGS->OnConquestUnitMoved.RemoveDynamic(this, &AConquestHUD::HandleUnitMovedForHUD);
		ConquestGS->OnConquestUnitMoved.AddDynamic(this, &AConquestHUD::HandleUnitMovedForHUD);
	}

	if (
		UWorld* World = GetWorld();
		World &&
		(
			World->URL.HasOption(TEXT("ConquestHostSetup")) ||
			World->URL.HasOption(TEXT("ConquestJoinSetup"))
		)
	)
	{
		ShowGameSetup();
	}
	else
	{
		ShowMainMenu();
	}
}

void AConquestHUD::ShowMainMenu()
{
	ConfigureMenuInputMode();

	if (HUDWidget)
	{
		HUDWidget->ShowMainMenu();
	}
}

void AConquestHUD::ShowSettingsMenu()
{
	if (!SettingsMenuWidget && SettingsMenuWidgetClass)
	{
		if (APlayerController* PlayerController = GetOwningPlayerController())
		{
			SettingsMenuWidget = CreateWidget<UConquestSettingsWidget>(PlayerController, SettingsMenuWidgetClass);
			if (SettingsMenuWidget)
			{
				SettingsMenuWidget->AddToViewport(50);
			}
		}
	}

	if (!SettingsMenuWidget)
	{
		return;
	}

	SettingsMenuWidget->RefreshFromSettings();
	SettingsMenuWidget->SetVisibility(ESlateVisibility::Visible);

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(SettingsMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
}

void AConquestHUD::HideSettingsMenu()
{
	if (SettingsMenuWidget)
	{
		SettingsMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (HUDWidget && HUDWidget->IsGameWidgetActive())
	{
		ConfigureGameInputMode();
	}
	else
	{
		ConfigureMenuInputMode();
	}
}

void AConquestHUD::ToggleSettingsMenu()
{
	if (SettingsMenuWidget && SettingsMenuWidget->IsVisible())
	{
		HideSettingsMenu();
		return;
	}

	ShowSettingsMenu();
}

void AConquestHUD::ShowCityPanel(int32 CityId)
{
	if (!CityPanelWidget)
	{
		return;
	}

	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	const FCityState* City = ConquestGS && ConquestGS->CityManager
		? ConquestGS->CityManager->GetCity(CityId)
		: nullptr;
	if (!ConquestGS || !City || City->OwnerPlayerId != ConquestGS->GetLocalPlayerId())
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

		if (ConquestGS && ConquestGS->ActiveGridActor && ConquestHUDIsLocalOwnedCity(*ConquestGS, CityId))
		{
			ConquestGS->ActiveGridActor->SetCityWorldLabelVisible(CityId, false);
		}

		return;
	}

	RestoreHiddenCityWorldLabel();

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (
		!ConquestGS ||
		!ConquestGS->ActiveGridActor ||
		CityId == INDEX_NONE ||
		!ConquestHUDIsLocalOwnedCity(*ConquestGS, CityId)
	)
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
	if (!City || City->OwnerPlayerId != ConquestGS->GetLocalPlayerId() || City->PendingBorderExpansions <= 0)
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

	const int32 ConfirmedCityId = ExpansionSelectionCityId;
	const FIntPoint ConfirmedCoord = PendingExpansionTileCoord;
	const FCityState* City = ConquestGS->CityManager->GetCity(ConfirmedCityId);
	const bool bAssigningExistingOwnedTile = City && City->OwnedTiles.Contains(ConfirmedCoord);

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestClaimExpansionTile(ConfirmedCityId, ConfirmedCoord);
	}
	else
	{
		return false;
	}

	if (bAssigningExistingOwnedTile)
	{
		HideCityPanel();
		return true;
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

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		if (ConquestPC->HasAuthority())
		{
			BeginCityTileExpansionSelection(ExpansionSelectionCityId);
		}
		else
		{
			ClearCityTileExpansionSelection();
		}
	}

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
	const int32 PlayerId = ConquestGS->GetHumanPlayer().PlayerId;
	if (!GridModel || !Tile || Tile->Gameplay.OwnerPlayerId != PlayerId)
	{
		ClearTileImprovementChoices();
		return false;
	}

	TArray<const FHexImprovementDefinition*> PossibleImprovements;
	GridModel->GetPossibleImprovementsForTile(Q, R, PossibleImprovements);

	TArray<FConquestChoiceButtonData> ImprovementChoices;
	if (!Tile->ImprovementId.IsNone())
	{
		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::TileImprovement;
		ChoiceData.Title = NSLOCTEXT("Conquest", "TileImprovementAlreadyUpgraded", "Tile already upgraded");
		ChoiceData.Subtitle = FText::FromName(Tile->ImprovementId);
		ChoiceData.bEnabled = false;
		ChoiceData.DisabledReason = NSLOCTEXT("Conquest", "TileImprovementAlreadyHasImprovement", "This tile already has an improvement");
		ChoiceData.DetailText = ChoiceData.DisabledReason;
		ImprovementChoices.Add(ChoiceData);
	}
	else if (PossibleImprovements.Num() <= 0)
	{
		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::TileImprovement;
		ChoiceData.Title = NSLOCTEXT("Conquest", "TileImprovementNoneAvailable", "No upgrades available");
		ChoiceData.Subtitle = NSLOCTEXT("Conquest", "TileImprovementNoneAvailableSubtitle", "This tile cannot currently be upgraded");
		ChoiceData.bEnabled = false;
		ChoiceData.DisabledReason = ChoiceData.Subtitle;
		ChoiceData.DetailText = ChoiceData.DisabledReason;
		ImprovementChoices.Add(ChoiceData);
	}
	else
	{
		const TArray<FName> AvailableImprovementIds =
			ConquestGS->CityManager->GetAvailableTileImprovementIdsForPlayer(PlayerId, Coord);
		const int32 StoredGold = ConquestGS->GetHumanPlayer().StoredYields.Gold;

		for (const FHexImprovementDefinition* Improvement : PossibleImprovements)
		{
			if (!Improvement)
			{
				continue;
			}

			const int32 GoldCost = FMath::Max(0, Improvement->PurchaseGoldCost);
			const bool bUnlocked = AvailableImprovementIds.Contains(Improvement->ImprovementId);
			if (!bUnlocked)
			{
				continue;
			}

			const bool bCanAfford = StoredGold >= GoldCost;

			FConquestChoiceButtonData ChoiceData;
			ChoiceData.ChoiceType = EConquestChoiceType::TileImprovement;
			ChoiceData.ChoiceId = Improvement->ImprovementId;
			ChoiceData.Title = !Improvement->DisplayName.IsEmpty()
				? Improvement->DisplayName
				: FText::FromName(Improvement->ImprovementId);
			ChoiceData.Cost = GoldCost;
			ChoiceData.bEnabled = bUnlocked && bCanAfford;
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
	}

	if (ImprovementChoices.Num() <= 0)
	{
		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::TileImprovement;
		ChoiceData.Title = NSLOCTEXT("Conquest", "TileImprovementNoneUnlocked", "No upgrades available");
		ChoiceData.Subtitle = NSLOCTEXT("Conquest", "TileImprovementNoneUnlockedSubtitle", "No upgrades are unlocked for this tile");
		ChoiceData.bEnabled = false;
		ChoiceData.DisabledReason = ChoiceData.Subtitle;
		ChoiceData.DetailText = ChoiceData.DisabledReason;
		ImprovementChoices.Add(ChoiceData);
	}

	PendingImprovementTileCoord = Coord;
	PendingExpansionTileCoord = FIntPoint(INT32_MIN, INT32_MIN);

	FConquestTileImprovementChoiceData ChoiceData;
	ChoiceData.Coord = Coord;
	ChoiceData.bIsValid = true;

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearTileExpansionConfirmation();
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

	const int32 PlayerId = ConquestGS->GetHumanPlayer().PlayerId;
	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestPurchaseTileImprovement(PendingImprovementTileCoord, ImprovementId);
	}
	else
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
	ClearUnitAttackHighlights();
	ClearUnitAugmentChoices();

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
		UnitWidgetData.CurrentMovementPoints = UnitState.CurrentMovementPoints;
		UnitWidgetData.bIsSleeping = UnitState.bIsSleeping;
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

void AConquestHUD::ClearUnitAttackHighlights()
{
	CurrentUnitAttackTiles.Reset();
	ClearCombatPreview();

	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (ConquestGS && ConquestGS->ActiveGridActor)
	{
		ConquestGS->ActiveGridActor->ClearExpansionCandidateHighlights();
	}
}

void AConquestHUD::FocusCameraOnTile(FIntPoint Coord, bool bPreserveCurrentHeight)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	APlayerController* PlayerController = GetOwningPlayerController();
	AConquestPawn* ConquestPawn = PlayerController
		? Cast<AConquestPawn>(PlayerController->GetPawn())
		: nullptr;

	if (!ConquestGS || !ConquestGS->ActiveGridActor || !ConquestPawn)
	{
		return;
	}

	ConquestPawn->FocusCameraOnHex(ConquestGS->ActiveGridActor, Coord, bPreserveCurrentHeight);
}

bool AConquestHUD::FocusCameraOnFirstLocalCity(bool bPreserveCurrentHeight)
{
	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return false;
	}

	const int32 LocalPlayerId = ConquestGS->GetLocalPlayerId();
	const FCityState* FirstLocalCity = ConquestGS->CityManager->Cities.FindByPredicate(
		[LocalPlayerId](const FCityState& City)
		{
			return City.OwnerPlayerId == LocalPlayerId;
		}
	);

	if (!FirstLocalCity)
	{
		return false;
	}

	FocusCameraOnTile(FirstLocalCity->CenterTile, bPreserveCurrentHeight);
	return true;
}

bool AConquestHUD::FocusCameraOnLocalStartingRegion(bool bPreserveCurrentHeight)
{
	FConquestPlayerStartRegion StartRegion;
	if (!GetLocalStartingRegion(StartRegion))
	{
		return false;
	}

	FocusCameraOnTile(StartRegion.Center, bPreserveCurrentHeight);
	bStartingCameraFocused = true;
	return true;
}

void AConquestHUD::TriggerEndTurnShortcut()
{
	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->HandleEndTurnClicked();
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

	ClearUnitAttackHighlights();
	ClearUnitAugmentChoices();

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

	auto IsOccupiedByAnotherUnit = [ConquestGS, SelectedUnit](const FIntPoint& Coord)
	{
		for (const FConquestPlayerEmpireState& CandidatePlayer : ConquestGS->PlayerEmpires)
		{
			for (const FConquestUnitState& Unit : CandidatePlayer.Units)
			{
				if (Unit.UnitInstanceId != SelectedUnit->UnitInstanceId && Unit.TileCoord == Coord)
				{
					return true;
				}
			}
		}

		return false;
	};

	auto GetMoveCost = [GridModel, ConquestGS, SelectedUnit](const FIntPoint& FromCoord, const FIntPoint& ToCoord)
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

		return ConquestGS->ModifierManager
			? ConquestGS->ModifierManager->CalculateMovementCost(*SelectedUnit, FromCoord, ToCoord, Cost)
			: Cost;
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
			const FHexTileData* NeighborTile = GridModel->GetTile(NeighborCoord);
			if (
				!NeighborTile ||
				ConquestHUDIsOwnedByOtherPlayer(*NeighborTile, Player.PlayerId) ||
				ConquestHUDIsEnemyCityTile(*ConquestGS, Player.PlayerId, NeighborCoord)
			)
			{
				continue;
			}

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

bool AConquestHUD::EnterSelectedUnitAttackMode()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->ActiveGridActor || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	ClearUnitMovementHighlights();
	ClearUnitAugmentChoices();

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit || SelectedUnit->CurrentMovementPoints <= 0)
	{
		ClearUnitAttackHighlights();
		return false;
	}

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	if (!GridModel || !GridModel->IsValidTile(SelectedUnit->TileCoord.X, SelectedUnit->TileCoord.Y))
	{
		ClearUnitAttackHighlights();
		return false;
	}

	CurrentUnitAttackTiles.Reset();
	const int32 AttackRange = FMath::Max(1, SelectedUnit->CachedAttackRange);
	const TArray<FIntPoint> CoordsInRange = GridModel->GetCoordsInRange(SelectedUnit->TileCoord, AttackRange);
	for (const FIntPoint& Coord : CoordsInRange)
	{
		if (Coord != SelectedUnit->TileCoord && GridModel->IsValidTile(Coord.X, Coord.Y))
		{
			CurrentUnitAttackTiles.Add(Coord);
		}
	}

	TArray<FIntPoint> AttackTiles = CurrentUnitAttackTiles.Array();
	UMaterialInterface* HighlightMaterial = nullptr;
	if (TObjectPtr<AConquestUnitActor>* UnitActorPtr =
		ConquestGS->UnitActorsByInstanceId.Find(SelectedUnit->UnitInstanceId))
	{
		if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
		{
			HighlightMaterial = UnitActor->GetAttackRangeMaterial();
		}
	}

	ConquestGS->ActiveGridActor->RebuildExpansionCandidateHighlights(AttackTiles, HighlightMaterial);
	return AttackTiles.Num() > 0;
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

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestMoveUnit(SelectedUnit->UnitInstanceId, TargetCoord);
	}

	ClearUnitMovementHighlights();
	if (*NewRemainingMovement <= 0)
	{
		ClearUnitSelection();
	}
	else
	{
		SelectedUnit->CurrentMovementPoints = *NewRemainingMovement;
		RefreshSelectedUnitWidget(*SelectedUnit);
	}
	return true;
}

bool AConquestHUD::TryAttackSelectedUnitAtTile(int32 Q, int32 R)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	const FIntPoint TargetCoord(Q, R);
	if (!CurrentUnitAttackTiles.Contains(TargetCoord))
	{
		EnterSelectedUnitAttackMode();
	}

	if (!CurrentUnitAttackTiles.Contains(TargetCoord))
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit || SelectedUnit->CurrentMovementPoints <= 0)
	{
		return false;
	}

	int32 DefenderUnitInstanceId = INDEX_NONE;
	for (const FConquestPlayerEmpireState& OtherPlayer : ConquestGS->PlayerEmpires)
	{
		if (OtherPlayer.PlayerId == Player.PlayerId)
		{
			continue;
		}

		for (const FConquestUnitState& OtherUnit : OtherPlayer.Units)
		{
			if (OtherUnit.TileCoord == TargetCoord)
			{
				DefenderUnitInstanceId = OtherUnit.UnitInstanceId;
				break;
			}
		}

		if (DefenderUnitInstanceId != INDEX_NONE)
		{
			break;
		}
	}

	if (DefenderUnitInstanceId == INDEX_NONE)
	{
		if (!ConquestGS->CityManager)
		{
			return false;
		}

		const int32 DefenderCityId = ConquestGS->CityManager->FindCityAtTile(TargetCoord);
		const FCityState* DefenderCity = DefenderCityId != INDEX_NONE
			? ConquestGS->CityManager->GetCity(DefenderCityId)
			: nullptr;
		if (DefenderCity && DefenderCity->OwnerPlayerId != Player.PlayerId)
		{
			if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
			{
				ConquestPC->RequestAttackCity(SelectedUnit->UnitInstanceId, DefenderCityId);
			}
		}
		else
		{
			const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
			const FHexTileData* TargetTile = GridModel ? GridModel->GetTile(TargetCoord) : nullptr;
			if (
				!TargetTile ||
				TargetTile->Gameplay.OwnerPlayerId == INDEX_NONE ||
				TargetTile->Gameplay.OwnerPlayerId == Player.PlayerId
			)
			{
				return false;
			}

			if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
			{
				ConquestPC->RequestAttackTile(SelectedUnit->UnitInstanceId, TargetCoord);
			}
		}

		SelectedUnit->CurrentMovementPoints = 0;
		ClearUnitAttackHighlights();
		if (SelectedUnit->CurrentMovementPoints <= 0)
		{
			ClearUnitSelection();
		}
		else
		{
			RefreshSelectedUnitWidget(*SelectedUnit);
		}

		return true;
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestAttackUnit(SelectedUnit->UnitInstanceId, DefenderUnitInstanceId);
	}

	SelectedUnit->CurrentMovementPoints = 0;
	ClearUnitAttackHighlights();
	if (SelectedUnit->CurrentMovementPoints <= 0)
	{
		ClearUnitSelection();
	}
	else
	{
		RefreshSelectedUnitWidget(*SelectedUnit);
	}

	return true;
}

bool AConquestHUD::IsSelectedUnitMovementTile(int32 Q, int32 R) const
{
	return CurrentUnitMovementRemainingByTile.Contains(FIntPoint(Q, R));
}

bool AConquestHUD::IsSelectedUnitAttackTile(int32 Q, int32 R) const
{
	return CurrentUnitAttackTiles.Contains(FIntPoint(Q, R));
}

bool AConquestHUD::UpdateSelectedUnitCombatPreviewForTile(int32 Q, int32 R)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->GetHexGridModel() || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		ClearCombatPreview();
		return false;
	}

	const FIntPoint TargetCoord(Q, R);
	const FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayer();
	const FConquestUnitState* SelectedUnit = Player.Units.FindByPredicate([ConquestGS](const FConquestUnitState& Unit)
	{
		return Unit.UnitInstanceId == ConquestGS->SelectedUnitInstanceId;
	});
	if (!SelectedUnit || SelectedUnit->CurrentMovementPoints <= 0)
	{
		ClearCombatPreview();
		return false;
	}

	const int32 AttackDistance = ConquestGS->GetHexGridModel()->GetHexDistance(SelectedUnit->TileCoord, TargetCoord);
	if (AttackDistance <= 0 || AttackDistance > FMath::Max(1, SelectedUnit->CachedAttackRange))
	{
		ClearCombatPreview();
		return false;
	}

	const FConquestUnitState* DefenderUnit = nullptr;
	for (const FConquestPlayerEmpireState& OtherPlayer : ConquestGS->PlayerEmpires)
	{
		if (OtherPlayer.PlayerId == Player.PlayerId)
		{
			continue;
		}

		DefenderUnit = OtherPlayer.Units.FindByPredicate([TargetCoord](const FConquestUnitState& Unit)
		{
			return Unit.TileCoord == TargetCoord;
		});
		if (DefenderUnit)
		{
			break;
		}
	}

	if (!DefenderUnit)
	{
		const int32 DefenderCityId = ConquestGS->CityManager
			? ConquestGS->CityManager->FindCityAtTile(TargetCoord)
			: INDEX_NONE;
		const FCityState* DefenderCity = DefenderCityId != INDEX_NONE && ConquestGS->CityManager
			? ConquestGS->CityManager->GetCity(DefenderCityId)
			: nullptr;
		if (!DefenderCity || DefenderCity->OwnerPlayerId == Player.PlayerId)
		{
			const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
			const FHexTileData* TargetTile = GridModel ? GridModel->GetTile(TargetCoord) : nullptr;
			FCityOwnedTileCombatState TileCombatState;
			if (
				!TargetTile ||
				TargetTile->Gameplay.OwnerPlayerId == INDEX_NONE ||
				TargetTile->Gameplay.OwnerPlayerId == Player.PlayerId ||
				!ConquestGS->CityManager ||
				!ConquestGS->CityManager->GetOwnedTileCombatState(TargetCoord, TileCombatState)
			)
			{
				ClearCombatPreview();
				return false;
			}

			const float AttackerCombatValue = ConquestGS->ModifierManager
				? ConquestGS->ModifierManager->CalculateUnitCombatValue(
					*SelectedUnit,
					EConquestUnitCombatModifierType::Attack,
					TargetTile->Gameplay.OwnerPlayerId
				)
				: ConquestUnitCombat::GetCombatValue(*SelectedUnit, EConquestUnitCombatModifierType::Attack);
			const float TileDefenseValue = FMath::Max(1.0f, static_cast<float>(FMath::Max(0, TileCombatState.CombatStrength)));

			FConquestCombatPreviewData PreviewData;
			PreviewData.AttackerUnitInstanceId = SelectedUnit->UnitInstanceId;
			PreviewData.DefenderUnitInstanceId = INDEX_NONE;
			PreviewData.AttackerName = ConquestHUDGetUnitDisplayName(*ConquestGS, *SelectedUnit);
			PreviewData.DefenderName = NSLOCTEXT("Conquest", "CombatPreviewTileDefenderName", "Border Tile");
			PreviewData.AttackerAttackValue = FMath::Max(1, FMath::CeilToInt(AttackerCombatValue));
			PreviewData.DefenderAttackValue = FMath::Max(1, FMath::CeilToInt(TileDefenseValue));
			PreviewData.AttackerCurrentHealth = SelectedUnit->CurrentHealth;
			PreviewData.AttackerMaxHealth = SelectedUnit->CachedMaxHealth;
			PreviewData.DefenderCurrentHealth = TileCombatState.CurrentHealth;
			PreviewData.DefenderMaxHealth = TileCombatState.MaxHealth;
			PreviewData.AttackDistance = AttackDistance;
			PreviewData.DamageDealt = ConquestUnitCombat::CalculateDeterministicDamage(
				AttackerCombatValue,
				TileDefenseValue,
				50.0f
			);
			PreviewData.DefenderProjectedHealth = FMath::Clamp(
				TileCombatState.CurrentHealth - PreviewData.DamageDealt,
				0,
				FMath::Max(1, TileCombatState.MaxHealth)
			);
			PreviewData.bDefenderKilled = PreviewData.DefenderProjectedHealth <= 0;
			PreviewData.bHasCounterAttack =
				!PreviewData.bDefenderKilled &&
				AttackDistance <= 1 &&
				TileCombatState.CombatStrength > 0;
			if (PreviewData.bHasCounterAttack)
			{
				PreviewData.DamageTaken = ConquestUnitCombat::CalculateDeterministicDamage(
					TileDefenseValue,
					AttackerCombatValue,
					25.0f
				);
			}
			PreviewData.AttackerProjectedHealth = FMath::Clamp(
				SelectedUnit->CurrentHealth - PreviewData.DamageTaken,
				0,
				SelectedUnit->CachedMaxHealth
			);
			PreviewData.bAttackerKilled = PreviewData.AttackerProjectedHealth <= 0;
			PreviewData.bCanAttack = true;
			PreviewData.bIsValid = true;
			PreviewData.Rating = PreviewData.bAttackerKilled
				? EConquestCombatPreviewRating::Costly
				: EConquestCombatPreviewRating::Safe;
			PreviewData.RatingText = PreviewData.bAttackerKilled
				? NSLOCTEXT("Conquest", "CombatPreviewCostlyTileAttack", "Costly Attack")
				: NSLOCTEXT("Conquest", "CombatPreviewSafeTileAttack", "Safe Attack");
			ConquestHUDAppendHappinessCombatModifierText(
				PreviewData.ModifierTexts,
				NSLOCTEXT("Conquest", "CombatPreviewAttackerLabel", "Attacker"),
				Player
			);
			ConquestHUDAppendUnitCombatModifierTexts(
				PreviewData.ModifierTexts,
				NSLOCTEXT("Conquest", "CombatPreviewAttackerAttackLabel", "Attacker ATK"),
				*SelectedUnit,
				EConquestUnitCombatModifierType::Attack
			);

			if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
			{
				ActiveGameWidget->ShowCombatPreview(PreviewData);
				return true;
			}

			return false;
		}

		const float AttackerCombatValue = ConquestGS->ModifierManager
			? ConquestGS->ModifierManager->CalculateUnitCombatValue(
				*SelectedUnit,
				EConquestUnitCombatModifierType::Attack,
				DefenderCity->OwnerPlayerId
			)
			: ConquestUnitCombat::GetCombatValue(*SelectedUnit, EConquestUnitCombatModifierType::Attack);
		const float CityDefenseValue = ConquestHUDGetCityCombatValue(*DefenderCity);

		FConquestCombatPreviewData PreviewData;
		PreviewData.AttackerUnitInstanceId = SelectedUnit->UnitInstanceId;
		PreviewData.DefenderUnitInstanceId = INDEX_NONE;
		PreviewData.AttackerName = ConquestHUDGetUnitDisplayName(*ConquestGS, *SelectedUnit);
		PreviewData.DefenderName = FText::FromName(DefenderCity->CityName);
		PreviewData.AttackerAttackValue = FMath::Max(1, FMath::CeilToInt(AttackerCombatValue));
		PreviewData.DefenderAttackValue = FMath::Max(1, FMath::CeilToInt(CityDefenseValue));
		PreviewData.AttackerCurrentHealth = SelectedUnit->CurrentHealth;
		PreviewData.AttackerMaxHealth = SelectedUnit->CachedMaxHealth;
		PreviewData.DefenderCurrentHealth = DefenderCity->CurrentHealth;
		PreviewData.DefenderMaxHealth = DefenderCity->MaxHealth;
		PreviewData.AttackDistance = AttackDistance;
		PreviewData.DamageDealt = DefenderCity->CurrentHealth <= 0
			? 0
			: ConquestUnitCombat::CalculateDeterministicDamage(AttackerCombatValue, CityDefenseValue, 50.0f);
		PreviewData.DefenderProjectedHealth = FMath::Clamp(
			DefenderCity->CurrentHealth - PreviewData.DamageDealt,
			0,
			FMath::Max(1, DefenderCity->MaxHealth)
		);
		PreviewData.bDefenderKilled = PreviewData.DefenderProjectedHealth <= 0;
		PreviewData.bHasCounterAttack = DefenderCity->CurrentHealth > 0 && !PreviewData.bDefenderKilled && AttackDistance <= 1;
		if (PreviewData.bHasCounterAttack)
		{
			FCityState CounterAttackCity = *DefenderCity;
			CounterAttackCity.CurrentHealth = PreviewData.DefenderProjectedHealth;
			PreviewData.DamageTaken = ConquestUnitCombat::CalculateDeterministicDamage(
				ConquestHUDGetCityCombatValue(CounterAttackCity),
				AttackerCombatValue,
				25.0f
			);
		}
		PreviewData.AttackerProjectedHealth = FMath::Clamp(
			SelectedUnit->CurrentHealth - PreviewData.DamageTaken,
			0,
			SelectedUnit->CachedMaxHealth
		);
		PreviewData.bAttackerKilled = PreviewData.AttackerProjectedHealth <= 0;
		PreviewData.bCanAttack = true;
		PreviewData.bIsValid = true;
		PreviewData.Rating = PreviewData.bAttackerKilled
			? EConquestCombatPreviewRating::Costly
			: EConquestCombatPreviewRating::Safe;
		PreviewData.RatingText = PreviewData.bAttackerKilled
			? NSLOCTEXT("Conquest", "CombatPreviewCostlyCityAttack", "Costly Attack")
			: NSLOCTEXT("Conquest", "CombatPreviewSafeCityAttack", "Safe Attack");
		ConquestHUDAppendHappinessCombatModifierText(
			PreviewData.ModifierTexts,
			NSLOCTEXT("Conquest", "CombatPreviewAttackerLabel", "Attacker"),
			Player
		);
		ConquestHUDAppendUnitCombatModifierTexts(
			PreviewData.ModifierTexts,
			NSLOCTEXT("Conquest", "CombatPreviewAttackerAttackLabel", "Attacker ATK"),
			*SelectedUnit,
			EConquestUnitCombatModifierType::Attack
		);

		if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
		{
			ActiveGameWidget->ShowCombatPreview(PreviewData);
			return true;
		}

		return false;
	}

	FConquestUnitState PreviewAttacker = *SelectedUnit;
	FConquestUnitState PreviewDefender = *DefenderUnit;
	if (ConquestGS->ModifierManager)
	{
		TArray<FConquestUnitCombatModifier> ExternalModifiers;
		ConquestGS->ModifierManager->BuildExternalUnitCombatModifiers(
			*SelectedUnit,
			DefenderUnit->OwnerPlayerId,
			ExternalModifiers
		);
		PreviewAttacker.CombatModifiers.Append(ExternalModifiers);

		ConquestGS->ModifierManager->BuildExternalUnitCombatModifiers(
			*DefenderUnit,
			Player.PlayerId,
			ExternalModifiers
		);
		PreviewDefender.CombatModifiers.Append(ExternalModifiers);
	}

	FConquestCombatPreviewData PreviewData =
		ConquestUnitCombat::CalculatePreview(PreviewAttacker, PreviewDefender, AttackDistance);
	PreviewData.AttackerName = ConquestHUDGetUnitDisplayName(*ConquestGS, *SelectedUnit);
	PreviewData.DefenderName = ConquestHUDGetUnitDisplayName(*ConquestGS, *DefenderUnit);
	ConquestHUDAppendHappinessCombatModifierText(
		PreviewData.ModifierTexts,
		NSLOCTEXT("Conquest", "CombatPreviewAttackerLabel", "Attacker"),
		Player
	);
	ConquestHUDAppendUnitCombatModifierTexts(
		PreviewData.ModifierTexts,
		NSLOCTEXT("Conquest", "CombatPreviewAttackerAttackLabel", "Attacker ATK"),
		PreviewAttacker,
		EConquestUnitCombatModifierType::Attack
	);
	if (DefenderUnit->OwnerPlayerId != INDEX_NONE)
	{
		const FConquestPlayerEmpireState& DefenderPlayer = ConquestGS->GetPlayerEmpire(DefenderUnit->OwnerPlayerId);
		if (DefenderPlayer.PlayerId == DefenderUnit->OwnerPlayerId)
		{
			ConquestHUDAppendHappinessCombatModifierText(
				PreviewData.ModifierTexts,
				NSLOCTEXT("Conquest", "CombatPreviewDefenderLabel", "Defender"),
				DefenderPlayer
			);
		}
	}
	ConquestHUDAppendUnitCombatModifierTexts(
		PreviewData.ModifierTexts,
		NSLOCTEXT("Conquest", "CombatPreviewDefenderDefenseLabel", "Defender DEF"),
		PreviewDefender,
		EConquestUnitCombatModifierType::Defense
	);
	if (PreviewData.bHasCounterAttack)
	{
		ConquestHUDAppendUnitCombatModifierTexts(
			PreviewData.ModifierTexts,
			NSLOCTEXT("Conquest", "CombatPreviewDefenderCounterLabel", "Defender Counter"),
			PreviewDefender,
			EConquestUnitCombatModifierType::Attack
		);
	}

	if (!PreviewData.bIsValid)
	{
		ClearCombatPreview();
		return false;
	}

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ShowCombatPreview(PreviewData);
		return true;
	}

	return false;
}

void AConquestHUD::ClearCombatPreview()
{
	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearCombatPreview();
	}
}

bool AConquestHUD::ShowAugmentChoicesForSelectedUnit()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!ConquestGS || !ConquestGS->ContentManager || ConquestGS->SelectedUnitInstanceId == INDEX_NONE)
	{
		return false;
	}

	ClearUnitMovementHighlights();
	ClearUnitAttackHighlights();

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit)
	{
		return false;
	}

	if (ConquestGS->CityManager)
	{
		ConquestGS->CityManager->RecalculateStrategicResourceEconomy(Player.PlayerId);
	}

	const FConquestUnitRow* UnitRow = ConquestGS->ContentManager->FindUnit(SelectedUnit->UnitId);
	if (!UnitRow)
	{
		return false;
	}

	TArray<const FConquestUnitAugmentRow*> AugmentRows;
	ConquestGS->ContentManager->GetAllUnitAugments(AugmentRows);

	TArray<FConquestChoiceButtonData> AugmentChoices;
	for (const FConquestUnitAugmentRow* AugmentRow : AugmentRows)
	{
		if (!AugmentRow || AugmentRow->AugmentId.IsNone())
		{
			continue;
		}

		if (UnitRow->AllowedAugmentIds.Num() > 0 && !UnitRow->AllowedAugmentIds.Contains(AugmentRow->AugmentId))
		{
			continue;
		}

		const bool bAlreadyApplied = SelectedUnit->Augments.ContainsByPredicate([AugmentRow](const FConquestUnitAugmentState& Augment)
		{
			return Augment.AugmentId == AugmentRow->AugmentId;
		});

		TArray<FString> CostParts;
		bool bCanAfford = !bAlreadyApplied;
		for (const FConquestStrategicResourceCost& Cost : AugmentRow->ResourceCosts)
		{
			if (Cost.ResourceId.IsNone() || Cost.Amount <= 0)
			{
				continue;
			}

			const FConquestStrategicResourceStockpile* Stockpile = Player.FindStrategicResource(Cost.ResourceId);
			const int32 Stored = Stockpile ? Stockpile->Stored : 0;
			CostParts.Add(FString::Printf(TEXT("%s %d/%d"), *Cost.ResourceId.ToString(), Stored, Cost.Amount));
			if (Stored < Cost.Amount)
			{
				bCanAfford = false;
			}
		}

		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::UnitAugment;
		ChoiceData.ChoiceId = AugmentRow->AugmentId;
		ChoiceData.Title = !AugmentRow->DisplayName.IsEmpty()
			? AugmentRow->DisplayName
			: FText::FromName(AugmentRow->AugmentId);
		ChoiceData.Subtitle = FText::FromString(CostParts.Num() > 0 ? FString::Join(CostParts, TEXT(" | ")) : TEXT("No strategic cost"));
		ChoiceData.DetailText = AugmentRow->Description;
		ChoiceData.bEnabled = bCanAfford;

		if (bAlreadyApplied)
		{
			ChoiceData.DisabledReason = NSLOCTEXT("Conquest", "UnitAugmentAlreadyApplied", "Already applied");
			ChoiceData.DetailText = ChoiceData.DisabledReason;
		}
		else if (!bCanAfford)
		{
			ChoiceData.DisabledReason = NSLOCTEXT("Conquest", "UnitAugmentCantAfford", "Not enough strategic resources");
			ChoiceData.DetailText = ChoiceData.DisabledReason;
		}

		AugmentChoices.Add(ChoiceData);
	}

	PendingAugmentUnitInstanceId = SelectedUnit->UnitInstanceId;
	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ShowUnitAugmentChoices(PendingAugmentUnitInstanceId, AugmentChoices);
	}

	return AugmentChoices.Num() > 0;
}

bool AConquestHUD::PurchaseSelectedUnitAugment(FName AugmentId)
{
	if (PendingAugmentUnitInstanceId == INDEX_NONE || AugmentId.IsNone())
	{
		return false;
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestApplyUnitAugment(PendingAugmentUnitInstanceId, AugmentId);
	}
	else
	{
		return false;
	}

	ClearUnitAugmentChoices();
	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->RefreshTopBarYieldInfo();
	}
	return true;
}

void AConquestHUD::ClearUnitAugmentChoices()
{
	PendingAugmentUnitInstanceId = INDEX_NONE;

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearUnitAugmentChoices();
	}
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

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestUnitAction(SelectedUnit->UnitInstanceId, FName(TEXT("Fortify")));
	}
	ClearUnitMovementHighlights();
	SelectedUnit->bIsFortified = true;
	SelectedUnit->bIsSleeping = true;
	SelectedUnit->CurrentMovementPoints = 0;
	ClearUnitSelection();
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

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestUnitAction(SelectedUnit->UnitInstanceId, FName(TEXT("DoNothing")));
	}
	ClearUnitMovementHighlights();
	ClearUnitSelection();
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

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestUnitAction(SelectedUnit->UnitInstanceId, FName(TEXT("Sleep")));
	}
	ClearUnitMovementHighlights();
	ClearUnitSelection();
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

	FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayerMutable();
	FConquestUnitState* SelectedUnit = ConquestHUDFindUnitMutable(Player, ConquestGS->SelectedUnitInstanceId);
	if (!SelectedUnit)
	{
		return false;
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestUnitAction(SelectedUnit->UnitInstanceId, FName(TEXT("Disband")));
	}
	ClearUnitSelection();
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

	const FIntPoint CityCoord = SelectedUnit->TileCoord;
	const int32 UnitInstanceId = SelectedUnit->UnitInstanceId;
	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController()))
	{
		ConquestPC->RequestUnitAction(UnitInstanceId, FName(TEXT("Settle")));
	}

	const int32 NewCityId = ConquestGS->CityManager->FindCityAtTile(CityCoord);
	ClearUnitSelection();

	if (NewCityId != INDEX_NONE)
	{
		ShowCityPanel(NewCityId);
	}

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

void AConquestHUD::ShowPhilosophyPanel()
{
	if (!PhilosophyPanelWidget)
	{
		return;
	}

	PhilosophyPanelWidget->RefreshPhilosophyOptions();
	PhilosophyPanelWidget->SetVisibility(ESlateVisibility::Visible);
}

void AConquestHUD::HidePhilosophyPanel()
{
	if (PhilosophyPanelWidget)
	{
		PhilosophyPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AConquestHUD::CloseEndTurnBlockingMenus()
{
	HideResearchPanel();
	HidePhilosophyPanel();
	HideCityPanel();
	ClearUnitSelection();
}

bool AConquestHUD::GetLocalStartingRegion(FConquestPlayerStartRegion& OutStartRegion) const
{
	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	return ConquestGS && ConquestGS->GetStartRegionForPlayer(ConquestGS->GetLocalPlayerId(), OutStartRegion);
}

TArray<FIntPoint> AConquestHUD::GetValidStartingRegionTiles(const FConquestPlayerStartRegion& StartRegion) const
{
	TArray<FIntPoint> Result;

	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	const FHexGridModel* GridModel = ConquestGS ? ConquestGS->GetHexGridModel() : nullptr;
	if (!GridModel)
	{
		return Result;
	}

	for (const FIntPoint& Coord : GridModel->GetCoordsInRange(StartRegion.Center, StartRegion.RegionRadius))
	{
		const FHexTileData* Tile = GridModel->GetTile(Coord);
		if (
			Tile &&
			Tile->Gameplay.OwnerPlayerId == INDEX_NONE &&
			!GridModel->IsWaterTileType(Tile->TileType) &&
			Tile->TileType != EHexTileType::Mountain
		)
		{
			Result.Add(Coord);
		}
	}

	return Result;
}

void AConquestHUD::BeginStartingRegionSelection()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->TurnManager || !ConquestGS->ActiveGridActor)
	{
		return;
	}

	if (ConquestGS->TurnManager->CurrentPhase != EConquestTurnPhase::AwaitingFirstCity)
	{
		return;
	}

	const int32 LocalPlayerId = ConquestGS->GetLocalPlayerId();
	const bool bHasLocalCity = ConquestGS->CityManager && ConquestGS->CityManager->Cities.ContainsByPredicate(
		[LocalPlayerId](const FCityState& City)
		{
			return City.OwnerPlayerId == LocalPlayerId;
		}
	);
	if (bHasLocalCity)
	{
		ConquestGS->ActiveGridActor->ClearExpansionCandidateHighlights();
		return;
	}

	FConquestPlayerStartRegion StartRegion;
	if (!GetLocalStartingRegion(StartRegion))
	{
		return;
	}

	ConquestGS->ActiveGridActor->RevealFogOfWarAroundTile(StartRegion.Center, ConquestHUDStartingRegionFogRevealRadius);
	if (!bStartingCameraFocused)
	{
		FocusCameraOnLocalStartingRegion(false);
	}

	UMaterialInterface* HighlightMaterial = nullptr;
	if (const UConquestCivilisationData* Civilisation = ConquestGS->GetCivilisationForPlayer(LocalPlayerId))
	{
		HighlightMaterial = Civilisation->BorderMaterial;
	}

	ConquestGS->ActiveGridActor->RebuildExpansionCandidateHighlights(
		GetValidStartingRegionTiles(StartRegion),
		HighlightMaterial
	);
}

bool AConquestHUD::SelectStartingCandidateTile(int32 Q, int32 R)
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->TurnManager || ConquestGS->TurnManager->CurrentPhase != EConquestTurnPhase::AwaitingFirstCity)
	{
		return false;
	}

	FConquestPlayerStartRegion StartRegion;
	if (!GetLocalStartingRegion(StartRegion))
	{
		return false;
	}

	const FIntPoint Coord(Q, R);
	if (!GetValidStartingRegionTiles(StartRegion).Contains(Coord))
	{
		return false;
	}

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	const FHexTileData* Tile = GridModel ? GridModel->GetTile(Coord) : nullptr;
	if (!Tile)
	{
		return false;
	}

	PendingStartingCityCoord = Coord;

	FConquestTileExpansionChoiceData ChoiceData;
	ChoiceData.CityId = INDEX_NONE;
	ChoiceData.Coord = Coord;
	ChoiceData.TileType = ConquestHUDHexTileTypeToString(Tile->TileType);
	ChoiceData.Resource = ConquestHUDFormatTileResource(*Tile);
	ChoiceData.Features = ConquestHUDFormatTileFeatures(*Tile);
	ChoiceData.Yield = Tile->FinalYield;
	ChoiceData.bIsValid = true;

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ShowStartingCityConfirmation(ChoiceData);
	}

	return true;
}

bool AConquestHUD::ConfirmSelectedStartingCity()
{
	if (PendingStartingCityCoord == FIntPoint(INT32_MIN, INT32_MIN))
	{
		return false;
	}

	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayerController());
	if (!ConquestPC)
	{
		return false;
	}

	const FIntPoint CityCoord = PendingStartingCityCoord;
	PendingStartingCityCoord = FIntPoint(INT32_MIN, INT32_MIN);
	ConquestPC->RequestFoundStartingCity(CityCoord, NAME_None);

	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		if (ConquestGS->ActiveGridActor)
		{
			ConquestGS->ActiveGridActor->RevealFogOfWarAroundTile(CityCoord, ConquestHUDCityFogRevealRadius);
			ConquestGS->ActiveGridActor->ClearExpansionCandidateHighlights();
		}
	}

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearTileExpansionConfirmation();
	}

	return true;
}

void AConquestHUD::CancelSelectedStartingCity()
{
	PendingStartingCityCoord = FIntPoint(INT32_MIN, INT32_MIN);

	if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
	{
		ActiveGameWidget->ClearTileExpansionConfirmation();
	}

	BeginStartingRegionSelection();
}

void AConquestHUD::ApplyLocalFogOfWar()
{
	AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->ActiveGridActor)
	{
		return;
	}

	const int32 LocalPlayerId = ConquestGS->GetLocalPlayerId();
	if (!bLocalFogInitialized || LocalFogPlayerId != LocalPlayerId)
	{
		ConquestGS->ActiveGridActor->ResetLocalFogOfWar(true);
		bLocalFogInitialized = true;
		LocalFogPlayerId = LocalPlayerId;
		bStartingCameraFocused = false;
		LastStartingCameraFocusCoord = FIntPoint(INT32_MIN, INT32_MIN);
	}

	FConquestPlayerStartRegion StartRegion;
	if (GetLocalStartingRegion(StartRegion))
	{
		if (LastStartingCameraFocusCoord != StartRegion.Center)
		{
			ConquestGS->ActiveGridActor->ResetLocalFogOfWar(true);
			bStartingCameraFocused = false;
			LastStartingCameraFocusCoord = StartRegion.Center;
			PendingStartingCityCoord = FIntPoint(INT32_MIN, INT32_MIN);
			if (UConquestGameWidget* ActiveGameWidget = GetActiveGameWidget())
			{
				ActiveGameWidget->ClearTileExpansionConfirmation();
			}
		}

		ConquestGS->ActiveGridActor->RevealFogOfWarAroundTile(StartRegion.Center, ConquestHUDStartingRegionFogRevealRadius);
	}

	if (ConquestGS->CityManager)
	{
		for (const FCityState& City : ConquestGS->CityManager->Cities)
		{
			if (City.OwnerPlayerId != LocalPlayerId)
			{
				continue;
			}

			ConquestGS->ActiveGridActor->RevealFogOfWarAroundTile(City.CenterTile, ConquestHUDCityFogRevealRadius);
			for (const FIntPoint& OwnedTile : City.OwnedTiles)
			{
				ConquestGS->ActiveGridActor->RevealFogOfWarAroundTile(OwnedTile, ConquestHUDCityOwnedTileFogRevealRadius);
			}
		}
	}

	const FConquestPlayerEmpireState& LocalPlayer = ConquestGS->GetPlayerEmpire(LocalPlayerId);
	for (const FConquestUnitState& Unit : LocalPlayer.Units)
	{
		if (Unit.OwnerPlayerId == LocalPlayerId)
		{
			ConquestGS->ActiveGridActor->RevealFogOfWarAroundTile(Unit.TileCoord, ConquestHUDUnitFogRevealRadius);
		}
	}

	if (ConquestGS->TurnManager && ConquestGS->TurnManager->CurrentPhase == EConquestTurnPhase::AwaitingFirstCity)
	{
		BeginStartingRegionSelection();
	}
}

void AConquestHUD::HandleConquestStateChangedForHUD()
{
	ApplyLocalFogOfWar();

	if (HiddenCityWorldLabelId != INDEX_NONE)
	{
		const AConquestGameState* ConquestGS = GetWorld()
			? GetWorld()->GetGameState<AConquestGameState>()
			: nullptr;
		if (ConquestGS && ConquestHUDIsLocalOwnedCity(*ConquestGS, HiddenCityWorldLabelId))
		{
			SetCityWorldLabelHiddenForPanel(HiddenCityWorldLabelId);
		}
		else
		{
			RestoreHiddenCityWorldLabel();
		}
	}
}

void AConquestHUD::HandleUnitMovedForHUD(int32 UnitInstanceId, int32 PlayerId, FIntPoint FromCoord, FIntPoint ToCoord)
{
	(void)UnitInstanceId;
	(void)FromCoord;

	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->ActiveGridActor || PlayerId != ConquestGS->GetLocalPlayerId())
	{
		return;
	}

	ConquestGS->ActiveGridActor->RevealFogOfWarAroundTile(ToCoord, ConquestHUDUnitFogRevealRadius);
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

	ApplyLocalFogOfWar();
}

void AConquestHUD::RequestStartGame(const FConquestGameSetupSettings& SetupSettings)
{
	UWorld* World = GetWorld();
	if (!World || !HexGridActorClass)
	{
		return;
	}

	if (IsValid(SpawnedHexGridActor))
	{
		SpawnedHexGridActor->Destroy();
	}
	SpawnedHexGridActor = nullptr;

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
		ConquestGS->ForceNetUpdate();
	}

	NewGridActor->ForceNetUpdate();

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
