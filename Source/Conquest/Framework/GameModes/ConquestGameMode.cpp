
#include "ConquestGameMode.h"
#include "ConquestGameState.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/Units/ConquestUnitActor.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexTileTypes.h"

namespace
{
	FConquestUnitState* FindUnitMutable(FConquestPlayerEmpireState& Player, int32 UnitInstanceId)
	{
		return Player.Units.FindByPredicate([UnitInstanceId](const FConquestUnitState& Unit)
		{
			return Unit.UnitInstanceId == UnitInstanceId;
		});
	}

	bool HasDifficultFeature(const FHexTileData& TileData)
	{
		return
			TileData.Features.Contains(EHexFeatureType::Forest) ||
			TileData.Features.Contains(EHexFeatureType::Jungle) ||
			TileData.Features.Contains(EHexFeatureType::Hill) ||
			TileData.Features.Contains(EHexFeatureType::Marsh);
	}

	bool IsImpassableForLandUnit(const FHexGridModel& GridModel, const FHexTileData& TileData)
	{
		return GridModel.IsWaterTileType(TileData.TileType) || TileData.TileType == EHexTileType::Mountain;
	}

	int32 GetMoveCost(const FHexGridModel& GridModel, const FIntPoint& FromCoord, const FIntPoint& ToCoord)
	{
		const FHexTileData* FromTile = GridModel.GetTile(FromCoord);
		const FHexTileData* ToTile = GridModel.GetTile(ToCoord);
		if (!FromTile || !ToTile || IsImpassableForLandUnit(GridModel, *ToTile))
		{
			return TNumericLimits<int32>::Max();
		}

		return ToTile->TileType == EHexTileType::Jungle ||
			HasDifficultFeature(*ToTile) ||
			FromTile->bHasRiver ||
			ToTile->bHasRiver
				? 2
				: 1;
	}
}

AConquestGameMode::AConquestGameMode()
{
	GameStateClass = AConquestGameState::StaticClass();
	PlayerControllerClass = AConquestPlayerController::StaticClass();
}

void AConquestGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AConquestGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(NewPlayer);
	if (!ConquestPC)
	{
		return;
	}

	const int32 PlayerId = NextAssignedPlayerId++;
	ConquestPC->SetAssignedPlayerId(PlayerId);
	ConnectedHumanPlayerIds.AddUnique(PlayerId);

	if (AConquestGameState* ConquestGS = GetGameState<AConquestGameState>())
	{
		ConquestGS->EnsurePlayerEmpire(PlayerId);
		ConquestGS->PushReplicatedState();
	}
}

void AConquestGameMode::Logout(AController* Exiting)
{
	if (const AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(Exiting))
	{
		ConnectedHumanPlayerIds.Remove(ConquestPC->GetAssignedPlayerId());
		ReadyPlayerIds.Remove(ConquestPC->GetAssignedPlayerId());
	}

	Super::Logout(Exiting);
}

TArray<UConquestCivilisationData*> AConquestGameMode::GetAvailableCivilisations() const
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

void AConquestGameMode::StartSinglePlayerGame()
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->TurnManager)
	{
		return;
	}

	InitializeEmpiresFromLobby();
	ConquestGS->TurnManager->BeginGameSetup();
	ConquestGS->TurnManager->EnterAwaitingFirstCity();
}

void AConquestGameMode::EndCurrentTurn()
{
	EndTurnForPlayer(0);
}

bool AConquestGameMode::EndTurnForPlayer(int32 PlayerId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->TurnManager || ConquestGS->TurnManager->CurrentPhase != EConquestTurnPhase::PlayerActions)
	{
		return false;
	}

	FText BlockReason;
	if (!CanPlayerEndTurn(*ConquestGS, PlayerId, BlockReason))
	{
		return false;
	}

	if (!ReadyPlayerIds.Contains(PlayerId))
	{
		ReadyPlayerIds.Add(PlayerId);
	}

	ConquestGS->ReplicatedConquestState.ReadyPlayerIds = ReadyPlayerIds;
	ConquestGS->PushReplicatedState();

	if (!AreAllHumanPlayersReady(*ConquestGS))
	{
		ConquestGS->BroadcastStateChanged();
		return true;
	}

	ReadyPlayerIds.Reset();
	ConquestGS->ReplicatedConquestState.ReadyPlayerIds.Reset();
	ConquestGS->TurnManager->RequestEndTurn();
	ConquestGS->PushReplicatedState();
	return true;
}

bool AConquestGameMode::SetCurrentResearchForPlayer(int32 PlayerId, FName TechId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->TechManager)
	{
		return false;
	}

	return ConquestGS->TechManager->SetCurrentResearchById(PlayerId, TechId);
}

bool AConquestGameMode::SetCityProductionForPlayer(
	int32 PlayerId,
	int32 CityId,
	ECityProductionType ProductionType,
	FName ProductionId
)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->CityManager || !DoesPlayerOwnCity(*ConquestGS, PlayerId, CityId))
	{
		return false;
	}

	switch (ProductionType)
	{
	case ECityProductionType::Building:
		return ConquestGS->CityManager->SetCityProductionBuildingById(CityId, ProductionId);
	case ECityProductionType::Unit:
		return ConquestGS->CityManager->SetCityProductionUnitById(CityId, ProductionId);
	case ECityProductionType::Project:
		return ConquestGS->CityManager->SetCityProductionProjectById(CityId, ProductionId);
	default:
		return false;
	}
}

bool AConquestGameMode::ClaimExpansionTileForPlayer(int32 PlayerId, int32 CityId, FIntPoint Coord)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->CityManager || !DoesPlayerOwnCity(*ConquestGS, PlayerId, CityId))
	{
		return false;
	}

	return ConquestGS->CityManager->ClaimExpansionTileForCity(CityId, Coord);
}

bool AConquestGameMode::PurchaseTileImprovementForPlayer(int32 PlayerId, FIntPoint Coord, FName ImprovementId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return false;
	}

	return ConquestGS->CityManager->PurchaseTileImprovementForPlayer(PlayerId, Coord, ImprovementId);
}

bool AConquestGameMode::MoveUnitForPlayer(int32 PlayerId, int32 UnitInstanceId, FIntPoint TargetCoord)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->ActiveGridActor)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetPlayerEmpireMutable(PlayerId);
	FConquestUnitState* Unit = FindUnitMutable(Player, UnitInstanceId);
	if (!Unit || Unit->OwnerPlayerId != PlayerId || Unit->CurrentMovementPoints <= 0)
	{
		return false;
	}

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	if (!GridModel || !GridModel->IsValidTile(TargetCoord.X, TargetCoord.Y))
	{
		return false;
	}

	for (const FConquestPlayerEmpireState& OtherPlayer : ConquestGS->PlayerEmpires)
	{
		for (const FConquestUnitState& OtherUnit : OtherPlayer.Units)
		{
			if (OtherUnit.UnitInstanceId != UnitInstanceId && OtherUnit.TileCoord == TargetCoord)
			{
				return false;
			}
		}
	}

	TMap<FIntPoint, int32> BestRemainingByCoord;
	TArray<FIntPoint> Frontier;
	BestRemainingByCoord.Add(Unit->TileCoord, Unit->CurrentMovementPoints);
	Frontier.Add(Unit->TileCoord);

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
			const int32 MoveCost = GetMoveCost(*GridModel, CurrentCoord, NeighborCoord);
			if (MoveCost == TNumericLimits<int32>::Max())
			{
				continue;
			}

			const bool bOneTileMinimumMove = CurrentCoord == Unit->TileCoord && CurrentRemaining > 0;
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
			if (NewRemaining > 0)
			{
				Frontier.Add(NeighborCoord);
			}
		}
	}

	const int32* RemainingMovement = BestRemainingByCoord.Find(TargetCoord);
	if (!RemainingMovement || TargetCoord == Unit->TileCoord)
	{
		return false;
	}

	const FIntPoint FromCoord = Unit->TileCoord;
	Unit->TileCoord = TargetCoord;
	Unit->CurrentMovementPoints = FMath::Clamp(*RemainingMovement, 0, Unit->CachedMovementPoints);
	Unit->bIsFortified = false;
	Unit->bIsSleeping = false;

	if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = ConquestGS->UnitActorsByInstanceId.Find(Unit->UnitInstanceId))
	{
		if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
		{
			UnitActor->MoveToTile(TargetCoord);
		}
	}

	ConquestGS->MulticastNotifyUnitMoved(Unit->UnitInstanceId, PlayerId, FromCoord, TargetCoord);
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestGameMode::ApplyUnitActionForPlayer(int32 PlayerId, int32 UnitInstanceId, FName ActionId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetPlayerEmpireMutable(PlayerId);
	FConquestUnitState* Unit = FindUnitMutable(Player, UnitInstanceId);
	if (!Unit || Unit->OwnerPlayerId != PlayerId)
	{
		return false;
	}

	bool bApplied = false;
	if (ActionId == FName(TEXT("Fortify")) && Unit->CurrentMovementPoints > 0)
	{
		Unit->bIsFortified = true;
		Unit->bIsSleeping = false;
		Unit->CurrentMovementPoints = FMath::Max(0, Unit->CurrentMovementPoints - 1);
		bApplied = true;
	}
	else if (ActionId == FName(TEXT("DoNothing")) && Unit->CurrentMovementPoints > 0)
	{
		Unit->bIsFortified = false;
		Unit->bIsSleeping = false;
		Unit->CurrentMovementPoints = 0;
		bApplied = true;
	}
	else if (ActionId == FName(TEXT("Sleep")))
	{
		Unit->bIsSleeping = true;
		Unit->bIsFortified = false;
		bApplied = true;
	}
	else if (ActionId == FName(TEXT("Disband")))
	{
		if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = ConquestGS->UnitActorsByInstanceId.Find(UnitInstanceId))
		{
			if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
			{
				UnitActor->Destroy();
			}
			ConquestGS->UnitActorsByInstanceId.Remove(UnitInstanceId);
		}

		Player.Units.RemoveAll([UnitInstanceId](const FConquestUnitState& Candidate)
		{
			return Candidate.UnitInstanceId == UnitInstanceId;
		});
		bApplied = true;
	}
	else if (ActionId == FName(TEXT("Settle")) && ConquestGS->CityManager && ConquestGS->ContentManager)
	{
		if (Unit->CurrentMovementPoints <= 0)
		{
			return false;
		}

		const FConquestUnitRow* UnitRow = ConquestGS->ContentManager->FindUnit(Unit->UnitId);
		if (!UnitRow || !UnitRow->bCanFoundCity)
		{
			return false;
		}

		const FIntPoint CityCoord = Unit->TileCoord;
		if (!ConquestGS->CityManager->FoundCity(PlayerId, CityCoord, NAME_None))
		{
			return false;
		}

		if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = ConquestGS->UnitActorsByInstanceId.Find(UnitInstanceId))
		{
			if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
			{
				UnitActor->Destroy();
			}
			ConquestGS->UnitActorsByInstanceId.Remove(UnitInstanceId);
		}

		Player.Units.RemoveAll([UnitInstanceId](const FConquestUnitState& Candidate)
		{
			return Candidate.UnitInstanceId == UnitInstanceId;
		});
		bApplied = true;
	}

	if (!bApplied)
	{
		return false;
	}

	ConquestGS->MulticastNotifyUnitAction(UnitInstanceId, PlayerId, ActionId);
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestGameMode::CanEndCurrentTurn(FText& OutBlockReason) const
{
	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS || !ConquestGS->TurnManager)
	{
		OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedNoGameState", "Game is not ready");
		return false;
	}

	if (ConquestGS->TurnManager->CurrentPhase != EConquestTurnPhase::PlayerActions)
	{
		OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedWrongPhase", "It is not currently the player action phase");
		return false;
	}

	const FConquestPlayerEmpireState& Player = ConquestGS->GetHumanPlayer();

	if (Player.CurrentResearchId.IsNone())
	{
		OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedResearch", "Select research");
		return false;
	}

	if (ConquestGS->CityManager)
	{
		for (const FCityState& City : ConquestGS->CityManager->Cities)
		{
			if (City.OwnerPlayerId == Player.PlayerId && !City.CurrentProduction.IsValid())
			{
				OutBlockReason = FText::Format(
					NSLOCTEXT("Conquest", "EndTurnBlockedCityProduction", "{0} needs production"),
					FText::FromName(City.CityName)
				);
				return false;
			}
		}
	}

	for (const FConquestUnitState& Unit : Player.Units)
	{
		if (Unit.OwnerPlayerId == Player.PlayerId && !Unit.bIsSleeping && Unit.CurrentMovementPoints > 0)
		{
			OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedUnitOrders", "Unit pending orders");
			return false;
		}
	}

	OutBlockReason = FText::GetEmpty();
	return true;
}

bool AConquestGameMode::FoundStartingCity(const FIntPoint& TileCoord, FName CityName)
{
	return FoundStartingCityForPlayer(0, TileCoord, CityName);
}

bool AConquestGameMode::FoundStartingCityForPlayer(int32 PlayerId, const FIntPoint& TileCoord, FName CityName)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->CityManager || !ConquestGS->TurnManager)
	{
		return false;
	}

	const bool bFounded = ConquestGS->CityManager->FoundCity(PlayerId, TileCoord, CityName);

	if (bFounded)
	{
		ConquestGS->TurnManager->BeginFirstTurn();
	}

	return bFounded;
}

void AConquestGameMode::InitializeEmpiresFromLobby()
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS)
	{
		return;
	}

	for (const FConquestLobbyPlayerSlot& Slot : ConquestGS->LobbyPlayerSlots)
	{
		if (Slot.PlayerId != INDEX_NONE && Slot.SlotType != EConquestLobbySlotType::Closed)
		{
			ConquestGS->EnsurePlayerEmpire(Slot.PlayerId);
		}
	}

	if (ConquestGS->PlayerEmpires.Num() <= 0)
	{
		ConquestGS->EnsurePlayerEmpire(0);
	}
}

bool AConquestGameMode::AreAllHumanPlayersReady(const AConquestGameState& ConquestGS) const
{
	const TArray<int32> HumanPlayerIds = GetHumanPlayerIds(ConquestGS);
	if (HumanPlayerIds.Num() <= 0)
	{
		return ReadyPlayerIds.Num() > 0;
	}

	for (const int32 PlayerId : HumanPlayerIds)
	{
		if (!ReadyPlayerIds.Contains(PlayerId))
		{
			return false;
		}
	}

	return true;
}

TArray<int32> AConquestGameMode::GetHumanPlayerIds(const AConquestGameState& ConquestGS) const
{
	TArray<int32> Result;

	if (ConnectedHumanPlayerIds.Num() > 0)
	{
		return ConnectedHumanPlayerIds;
	}

	for (const FConquestLobbyPlayerSlot& Slot : ConquestGS.LobbyPlayerSlots)
	{
		if (Slot.PlayerId != INDEX_NONE && Slot.SlotType == EConquestLobbySlotType::Human)
		{
			Result.AddUnique(Slot.PlayerId);
		}
	}

	if (Result.Num() <= 0)
	{
		for (const FConquestPlayerEmpireState& PlayerEmpire : ConquestGS.PlayerEmpires)
		{
			if (PlayerEmpire.PlayerId != INDEX_NONE)
			{
				Result.AddUnique(PlayerEmpire.PlayerId);
			}
		}
	}

	return Result;
}

bool AConquestGameMode::DoesPlayerOwnCity(const AConquestGameState& ConquestGS, int32 PlayerId, int32 CityId) const
{
	if (!ConquestGS.CityManager)
	{
		return false;
	}

	const FCityState* City = ConquestGS.CityManager->GetCity(CityId);
	return City && City->OwnerPlayerId == PlayerId;
}

bool AConquestGameMode::CanPlayerEndTurn(
	const AConquestGameState& ConquestGS,
	int32 PlayerId,
	FText& OutBlockReason
) const
{
	if (!ConquestGS.TurnManager || ConquestGS.TurnManager->CurrentPhase != EConquestTurnPhase::PlayerActions)
	{
		OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedWrongPhase", "It is not currently the player action phase");
		return false;
	}

	const FConquestPlayerEmpireState& Player = ConquestGS.GetPlayerEmpire(PlayerId);
	if (Player.CurrentResearchId.IsNone())
	{
		OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedResearch", "Select research");
		return false;
	}

	if (ConquestGS.CityManager)
	{
		for (const FCityState& City : ConquestGS.CityManager->Cities)
		{
			if (City.OwnerPlayerId == PlayerId && !City.CurrentProduction.IsValid())
			{
				OutBlockReason = FText::Format(
					NSLOCTEXT("Conquest", "EndTurnBlockedCityProduction", "{0} needs production"),
					FText::FromName(City.CityName)
				);
				return false;
			}
		}
	}

	for (const FConquestUnitState& Unit : Player.Units)
	{
		if (Unit.OwnerPlayerId == PlayerId && !Unit.bIsSleeping && Unit.CurrentMovementPoints > 0)
		{
			OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedUnitOrders", "Unit pending orders");
			return false;
		}
	}

	OutBlockReason = FText::GetEmpty();
	return true;
}
