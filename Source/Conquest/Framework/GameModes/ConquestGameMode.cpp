
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
#include "Conquest/World/Generation/HexResourceTypes.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "GameFramework/PlayerState.h"

namespace
{
	FConquestUnitState* FindUnitMutable(FConquestPlayerEmpireState& Player, int32 UnitInstanceId)
	{
		return Player.Units.FindByPredicate([UnitInstanceId](const FConquestUnitState& Unit)
		{
			return Unit.UnitInstanceId == UnitInstanceId;
		});
	}

	FConquestUnitState* FindUnitMutableById(
		TArray<FConquestPlayerEmpireState>& Players,
		int32 UnitInstanceId,
		int32* OutOwnerPlayerId = nullptr
	)
	{
		for (FConquestPlayerEmpireState& Player : Players)
		{
			if (FConquestUnitState* Unit = FindUnitMutable(Player, UnitInstanceId))
			{
				if (OutOwnerPlayerId)
				{
					*OutOwnerPlayerId = Player.PlayerId;
				}
				return Unit;
			}
		}

		return nullptr;
	}

	void DestroyUnitActor(AConquestGameState& GameState, int32 UnitInstanceId)
	{
		if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = GameState.UnitActorsByInstanceId.Find(UnitInstanceId))
		{
			if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
			{
				UnitActor->Destroy();
			}
			GameState.UnitActorsByInstanceId.Remove(UnitInstanceId);
		}
	}

	bool CanPayStrategicResourceCosts(
		const FConquestPlayerEmpireState& Player,
		const TArray<FConquestStrategicResourceCost>& Costs
	)
	{
		for (const FConquestStrategicResourceCost& Cost : Costs)
		{
			if (Cost.ResourceId.IsNone() || Cost.Amount <= 0)
			{
				continue;
			}

			const FConquestStrategicResourceStockpile* Stockpile = Player.FindStrategicResource(Cost.ResourceId);
			if (!Stockpile || Stockpile->Stored < Cost.Amount)
			{
				return false;
			}
		}

		return true;
	}

	void PayStrategicResourceCosts(
		FConquestPlayerEmpireState& Player,
		const TArray<FConquestStrategicResourceCost>& Costs
	)
	{
		for (const FConquestStrategicResourceCost& Cost : Costs)
		{
			if (Cost.ResourceId.IsNone() || Cost.Amount <= 0)
			{
				continue;
			}

			if (FConquestStrategicResourceStockpile* Stockpile = Player.FindStrategicResource(Cost.ResourceId))
			{
				Stockpile->Stored = FMath::Max(0, Stockpile->Stored - Cost.Amount);
			}
		}
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

	bool IsPoorStartingTerrain(EHexTileType TileType)
	{
		return
			TileType == EHexTileType::Ocean ||
			TileType == EHexTileType::Coast ||
			TileType == EHexTileType::Lake ||
			TileType == EHexTileType::Mountain;
	}

	bool IsOwnedByOtherPlayer(const FHexTileData& TileData, int32 PlayerId)
	{
		return
			TileData.Gameplay.OwnerPlayerId != INDEX_NONE &&
			TileData.Gameplay.OwnerPlayerId != PlayerId;
	}

	bool IsEnemyCityTile(const AConquestGameState& GameState, int32 PlayerId, const FIntPoint& Coord)
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

	float GetCityHealthCombatMultiplier(const FCityState& City)
	{
		return FMath::Clamp(
			static_cast<float>(FMath::Max(1, City.CurrentHealth)) /
			static_cast<float>(FMath::Max(1, City.MaxHealth)),
			0.01f,
			1.0f
		);
	}

	float GetCityCombatValue(const FCityState& City)
	{
		return static_cast<float>(FMath::Max(
			1,
			FMath::CeilToInt(static_cast<float>(FMath::Max(1, City.CachedStrength)) * GetCityHealthCombatMultiplier(City))
		));
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
	SyncAvailableCivilisationsToGameState();
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
		SyncAvailableCivilisationsToGameState();
		AssignPlayerToLobbySlot(PlayerId, NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerName() : FString());
		ConquestGS->EnsurePlayerEmpire(PlayerId);
		ConquestGS->PushReplicatedState();
	}
}

void AConquestGameMode::Logout(AController* Exiting)
{
	if (const AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(Exiting))
	{
		const int32 ExitingPlayerId = ConquestPC->GetAssignedPlayerId();
		ConnectedHumanPlayerIds.Remove(ExitingPlayerId);
		ReadyPlayerIds.Remove(ExitingPlayerId);

		if (AConquestGameState* ConquestGS = GetGameState<AConquestGameState>())
		{
			for (FConquestLobbyPlayerSlot& Slot : ConquestGS->LobbyPlayerSlots)
			{
				if (Slot.PlayerId == ExitingPlayerId && ExitingPlayerId != 0)
				{
					Slot.PlayerId = INDEX_NONE;
					Slot.SlotType = EConquestLobbySlotType::AI;
					Slot.bIsHost = false;
					Slot.PlayerName.Reset();
					Slot.bIsReady = false;
					Slot.Civilisation = nullptr;
					break;
				}
			}
			ConquestGS->BroadcastStateChanged();
		}
	}

	Super::Logout(Exiting);
}

void AConquestGameMode::SyncAvailableCivilisationsToGameState()
{
	if (AConquestGameState* ConquestGS = GetGameState<AConquestGameState>())
	{
		TArray<UConquestCivilisationData*> Civilisations;
		for (UConquestCivilisationData* Civilisation : AvailableCivilisations)
		{
			if (Civilisation)
			{
				Civilisations.Add(Civilisation);
			}
		}
		ConquestGS->SetAvailableCivilisations(Civilisations);
	}
}

void AConquestGameMode::AssignPlayerToLobbySlot(int32 PlayerId, const FString& PlayerName)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || ConquestGS->bGameEnded)
	{
		return;
	}

	if (ConquestGS->LobbyPlayerSlots.Num() <= 0)
	{
		FConquestMapSizePresetDefinition SizePreset;
		const int32 PlayerCount = FConquestMapSizePresets::GetPreset(EConquestMapSizePreset::Standard, SizePreset)
			? FMath::Max(1, SizePreset.PlayerCount)
			: 1;

		ConquestGS->LobbyPlayerSlots.Reset(PlayerCount);
		for (int32 SlotIndex = 0; SlotIndex < PlayerCount; ++SlotIndex)
		{
			FConquestLobbyPlayerSlot Slot;
			Slot.SlotIndex = SlotIndex;
			Slot.PlayerId = INDEX_NONE;
			Slot.SlotType = EConquestLobbySlotType::AI;
			ConquestGS->LobbyPlayerSlots.Add(Slot);
		}
	}

	int32 TargetSlotIndex = INDEX_NONE;
	if (PlayerId == 0)
	{
		TargetSlotIndex = 0;
	}
	else
	{
		for (int32 SlotIndex = 1; SlotIndex < ConquestGS->LobbyPlayerSlots.Num(); ++SlotIndex)
		{
			if (ConquestGS->LobbyPlayerSlots[SlotIndex].SlotType != EConquestLobbySlotType::Human)
			{
				TargetSlotIndex = SlotIndex;
				break;
			}
		}
	}

	if (!ConquestGS->LobbyPlayerSlots.IsValidIndex(TargetSlotIndex))
	{
		return;
	}

	FConquestLobbyPlayerSlot& Slot = ConquestGS->LobbyPlayerSlots[TargetSlotIndex];
	Slot.PlayerId = PlayerId;
	Slot.SlotType = EConquestLobbySlotType::Human;
	Slot.bIsHost = PlayerId == 0;
	Slot.PlayerName = PlayerName.IsEmpty()
		? FString::Printf(TEXT("Player %d"), PlayerId + 1)
		: PlayerName;
	Slot.bIsReady = false;
	if (PlayerId != 0)
	{
		Slot.Civilisation = nullptr;
	}

	ConquestGS->BroadcastStateChanged();
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

bool AConquestGameMode::SetLobbySlotCivilisationForPlayer(
	int32 PlayerId,
	int32 SlotIndex,
	UConquestCivilisationData* Civilisation
)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	return ConquestGS && ConquestGS->SetLobbySlotCivilisationForPlayer(PlayerId, SlotIndex, Civilisation);
}

void AConquestGameMode::SetLobbyReadyForPlayer(int32 PlayerId, bool bReady)
{
	if (AConquestGameState* ConquestGS = GetGameState<AConquestGameState>())
	{
		ConquestGS->SetLobbyPlayerReady(PlayerId, bReady);
	}
}

bool AConquestGameMode::AreAllLobbyHumanPlayersReady() const
{
	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	return ConquestGS &&
		ConquestGS->GetRequiredReadyHumanPlayerCount() > 0 &&
		ConquestGS->GetReadyHumanPlayerCount() >= ConquestGS->GetRequiredReadyHumanPlayerCount();
}

void AConquestGameMode::StartSinglePlayerGame()
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->TurnManager)
	{
		return;
	}

	InitializeEmpiresFromLobby();
	AssignPlayerStartRegions();
	ConquestGS->TurnManager->BeginGameSetup();
	ConquestGS->TurnManager->EnterAwaitingFirstCity();
}

bool AConquestGameMode::RegenerateFirstTurnMapForPlayer(int32 PlayerId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (
		PlayerId != 0 ||
		!ConquestGS ||
		ConquestGS->bGameEnded ||
		!ConquestGS->TurnManager ||
		!ConquestGS->ActiveGridActor ||
		ConquestGS->TurnManager->CurrentTurn != 1
	)
	{
		return false;
	}

	ConquestGS->ClearUnitVisuals();

	if (ConquestGS->CityManager)
	{
		ConquestGS->CityManager->ResetCities();
	}

	ReadyPlayerIds.Reset();
	ConquestGS->ReplicatedConquestState.ReadyPlayerIds.Reset();
	ConquestGS->PlayerStartRegions.Reset();
	ConquestGS->PlayerEmpires.Reset();
	ConquestGS->HumanPlayer = FConquestPlayerEmpireState();
	ConquestGS->HumanPlayer.PlayerId = 0;
	ConquestGS->EnsurePlayerEmpire(0);

	ConquestGS->ActiveGridActor->RegenerateGridWithNewRandomSeed();
	InitializeEmpiresFromLobby();
	for (const int32 ConnectedPlayerId : ConnectedHumanPlayerIds)
	{
		ConquestGS->EnsurePlayerEmpire(ConnectedPlayerId);
	}
	AssignPlayerStartRegions();
	ConquestGS->TurnManager->BeginGameSetup();
	ConquestGS->TurnManager->EnterAwaitingFirstCity();
	ConquestGS->BroadcastStateChanged();
	return true;
}

void AConquestGameMode::EndCurrentTurn()
{
	EndTurnForPlayer(0);
}

bool AConquestGameMode::EndTurnForPlayer(int32 PlayerId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (
		!ConquestGS ||
		ConquestGS->bGameEnded ||
		!ConquestGS->TurnManager ||
		ConquestGS->TurnManager->CurrentPhase != EConquestTurnPhase::PlayerActions
	)
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
	if (!ConquestGS || ConquestGS->bGameEnded || !ConquestGS->TechManager)
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
	if (
		!ConquestGS ||
		ConquestGS->bGameEnded ||
		!ConquestGS->CityManager ||
		!DoesPlayerOwnCity(*ConquestGS, PlayerId, CityId)
	)
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
	if (
		!ConquestGS ||
		ConquestGS->bGameEnded ||
		!ConquestGS->CityManager ||
		!DoesPlayerOwnCity(*ConquestGS, PlayerId, CityId)
	)
	{
		return false;
	}

	return ConquestGS->CityManager->ClaimExpansionTileForCity(CityId, Coord);
}

bool AConquestGameMode::PurchaseTileImprovementForPlayer(int32 PlayerId, FIntPoint Coord, FName ImprovementId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || ConquestGS->bGameEnded || !ConquestGS->CityManager)
	{
		return false;
	}

	return ConquestGS->CityManager->PurchaseTileImprovementForPlayer(PlayerId, Coord, ImprovementId);
}

bool AConquestGameMode::MoveUnitForPlayer(int32 PlayerId, int32 UnitInstanceId, FIntPoint TargetCoord)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || ConquestGS->bGameEnded || !ConquestGS->ActiveGridActor)
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
			const FHexTileData* NeighborTile = GridModel->GetTile(NeighborCoord);
			if (
				!NeighborTile ||
				IsOwnedByOtherPlayer(*NeighborTile, PlayerId) ||
				IsEnemyCityTile(*ConquestGS, PlayerId, NeighborCoord)
			)
			{
				continue;
			}

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
		Unit->bIsSleeping = true;
		Unit->CurrentMovementPoints = 0;
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

bool AConquestGameMode::ApplyUnitAugmentForPlayer(int32 PlayerId, int32 UnitInstanceId, FName AugmentId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (
		!ConquestGS ||
		ConquestGS->bGameEnded ||
		!ConquestGS->ContentManager ||
		!ConquestGS->CityManager ||
		AugmentId.IsNone()
	)
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = ConquestGS->GetPlayerEmpireMutable(PlayerId);
	FConquestUnitState* Unit = FindUnitMutable(Player, UnitInstanceId);
	if (!Unit || Unit->OwnerPlayerId != PlayerId)
	{
		return false;
	}

	const FConquestUnitRow* UnitRow = ConquestGS->ContentManager->FindUnit(Unit->UnitId);
	const FConquestUnitAugmentRow* AugmentRow = ConquestGS->ContentManager->FindUnitAugment(AugmentId);
	if (!UnitRow || !AugmentRow)
	{
		return false;
	}

	if (UnitRow->AllowedAugmentIds.Num() > 0 && !UnitRow->AllowedAugmentIds.Contains(AugmentId))
	{
		return false;
	}

	if (Unit->Augments.ContainsByPredicate([AugmentId](const FConquestUnitAugmentState& Augment)
	{
		return Augment.AugmentId == AugmentId;
	}))
	{
		return false;
	}

	ConquestGS->CityManager->RecalculateStrategicResourceEconomy(PlayerId);
	if (!CanPayStrategicResourceCosts(Player, AugmentRow->ResourceCosts))
	{
		return false;
	}

	PayStrategicResourceCosts(Player, AugmentRow->ResourceCosts);

	FConquestUnitAugmentState NewAugment;
	NewAugment.AugmentId = AugmentId;
	NewAugment.ModifiedStat = AugmentRow->ModifiedStat;
	NewAugment.FlatBonus = AugmentRow->FlatBonus;
	NewAugment.MultiplierBonus = AugmentRow->MultiplierBonus;
	Unit->Augments.Add(NewAugment);

	if (AugmentRow->ModifiedStat == EConquestUnitAugmentStat::AttackMultiplier ||
		AugmentRow->ModifiedStat == EConquestUnitAugmentStat::DefenseMultiplier)
	{
		FConquestUnitCombatModifier CombatModifier;
		CombatModifier.ModifierId = AugmentId;
		CombatModifier.ModifierType = AugmentRow->ModifiedStat == EConquestUnitAugmentStat::AttackMultiplier
			? EConquestUnitCombatModifierType::Attack
			: EConquestUnitCombatModifierType::Defense;
		CombatModifier.Multiplier = FMath::Max(0.0f, 1.0f + AugmentRow->MultiplierBonus);
		Unit->CombatModifiers.Add(CombatModifier);
	}

	const int32 PreviousMaxHealth = FMath::Max(1, Unit->CachedMaxHealth);
	ConquestGS->CityManager->RecalculateUnitStats(*Unit);
	if (Unit->CachedMaxHealth > PreviousMaxHealth)
	{
		Unit->CurrentHealth = FMath::Clamp(
			Unit->CurrentHealth + (Unit->CachedMaxHealth - PreviousMaxHealth),
			1,
			Unit->CachedMaxHealth
		);
	}

	ConquestGS->MulticastNotifyUnitAction(UnitInstanceId, PlayerId, FName(TEXT("Augment")));
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestGameMode::AttackUnitForPlayer(
	int32 PlayerId,
	int32 AttackerUnitInstanceId,
	int32 DefenderUnitInstanceId
)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || ConquestGS->bGameEnded || !ConquestGS->GetHexGridModel())
	{
		return false;
	}

	FConquestPlayerEmpireState& AttackerPlayer = ConquestGS->GetPlayerEmpireMutable(PlayerId);
	FConquestUnitState* Attacker = FindUnitMutable(AttackerPlayer, AttackerUnitInstanceId);
	if (!Attacker || Attacker->OwnerPlayerId != PlayerId || Attacker->CurrentMovementPoints <= 0)
	{
		return false;
	}

	int32 DefenderOwnerPlayerId = INDEX_NONE;
	FConquestUnitState* Defender = FindUnitMutableById(
		ConquestGS->PlayerEmpires,
		DefenderUnitInstanceId,
		&DefenderOwnerPlayerId
	);
	if (!Defender || DefenderOwnerPlayerId == PlayerId || Defender->OwnerPlayerId == PlayerId)
	{
		return false;
	}

	const FIntPoint AttackerFromCoord = Attacker->TileCoord;
	const FIntPoint DefenderCoord = Defender->TileCoord;
	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	const int32 AttackDistance = GridModel->GetHexDistance(Attacker->TileCoord, Defender->TileCoord);
	if (AttackDistance <= 0 || AttackDistance > FMath::Max(1, Attacker->CachedAttackRange))
	{
		return false;
	}

	const float AttackerCombatValue = ConquestUnitCombat::GetCombatValue(*Attacker, EConquestUnitCombatModifierType::Attack);
	const float DefenderDefenseValue = ConquestUnitCombat::GetCombatValue(*Defender, EConquestUnitCombatModifierType::Defense);
	const int32 DefenderDamage = ConquestUnitCombat::CalculateDeterministicDamage(AttackerCombatValue, DefenderDefenseValue, 50.0f);

	Defender->CurrentHealth = FMath::Clamp(Defender->CurrentHealth - DefenderDamage, 0, Defender->CachedMaxHealth);
	Attacker->CurrentMovementPoints = 0;
	Attacker->bIsFortified = false;
	Attacker->bIsSleeping = false;

	const bool bDefenderKilled = Defender->CurrentHealth <= 0;
	if (!bDefenderKilled && AttackDistance <= 1)
	{
		const float DefenderCounterValue = ConquestUnitCombat::GetCombatValue(*Defender, EConquestUnitCombatModifierType::Attack);
		const int32 AttackerDamage = ConquestUnitCombat::CalculateDeterministicDamage(DefenderCounterValue, AttackerCombatValue, 25.0f);
		Attacker->CurrentHealth = FMath::Clamp(Attacker->CurrentHealth - AttackerDamage, 0, Attacker->CachedMaxHealth);
	}

	const bool bAttackerKilled = Attacker->CurrentHealth <= 0;
	if (bDefenderKilled)
	{
		FConquestPlayerEmpireState& DefenderPlayer = ConquestGS->GetPlayerEmpireMutable(DefenderOwnerPlayerId);
		DefenderPlayer.Units.RemoveAll([DefenderUnitInstanceId](const FConquestUnitState& Candidate)
		{
			return Candidate.UnitInstanceId == DefenderUnitInstanceId;
		});
		DestroyUnitActor(*ConquestGS, DefenderUnitInstanceId);
	}

	if (bAttackerKilled)
	{
		AttackerPlayer.Units.RemoveAll([AttackerUnitInstanceId](const FConquestUnitState& Candidate)
		{
			return Candidate.UnitInstanceId == AttackerUnitInstanceId;
		});
		DestroyUnitActor(*ConquestGS, AttackerUnitInstanceId);
	}
	else if (bDefenderKilled)
	{
		const FHexTileData* DefenderTile = GridModel->GetTile(DefenderCoord);
		const bool bCanAdvanceIntoDefenderTile =
			DefenderTile &&
			!IsOwnedByOtherPlayer(*DefenderTile, PlayerId) &&
			!IsEnemyCityTile(*ConquestGS, PlayerId, DefenderCoord);

		if (bCanAdvanceIntoDefenderTile)
		{
			Attacker->TileCoord = DefenderCoord;

			if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = ConquestGS->UnitActorsByInstanceId.Find(Attacker->UnitInstanceId))
			{
				if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
				{
					UnitActor->MoveToTile(DefenderCoord);
				}
			}

			ConquestGS->MulticastNotifyUnitMoved(Attacker->UnitInstanceId, PlayerId, AttackerFromCoord, DefenderCoord);
		}
	}

	ConquestGS->MulticastNotifyUnitAction(AttackerUnitInstanceId, PlayerId, FName(TEXT("Attack")));
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestGameMode::AttackCityForPlayer(
	int32 PlayerId,
	int32 AttackerUnitInstanceId,
	int32 DefenderCityId
)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || ConquestGS->bGameEnded || !ConquestGS->CityManager || !ConquestGS->GetHexGridModel())
	{
		return false;
	}

	FConquestPlayerEmpireState& AttackerPlayer = ConquestGS->GetPlayerEmpireMutable(PlayerId);
	FConquestUnitState* Attacker = FindUnitMutable(AttackerPlayer, AttackerUnitInstanceId);
	FCityState* DefenderCity = ConquestGS->CityManager->GetCityMutable(DefenderCityId);
	if (
		!Attacker ||
		!DefenderCity ||
		Attacker->OwnerPlayerId != PlayerId ||
		DefenderCity->OwnerPlayerId == PlayerId ||
		Attacker->CurrentMovementPoints <= 0
	)
	{
		return false;
	}

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	const int32 AttackDistance = GridModel->GetHexDistance(Attacker->TileCoord, DefenderCity->CenterTile);
	if (AttackDistance <= 0 || AttackDistance > FMath::Max(1, Attacker->CachedAttackRange))
	{
		return false;
	}

	DefenderCity->MaxHealth = FMath::Max(1, DefenderCity->MaxHealth);
	DefenderCity->CachedStrength = ConquestGS->CityManager->GetCityStrength(*DefenderCity);
	DefenderCity->CurrentHealth = FMath::Clamp(DefenderCity->CurrentHealth, 0, DefenderCity->MaxHealth);

	const bool bCityAlreadyDefeated = DefenderCity->CurrentHealth <= 0;
	const bool bMeleeAttack = Attacker->CachedAttackRange <= 1 && AttackDistance <= 1;
	const float AttackerCombatValue = ConquestUnitCombat::GetCombatValue(
		*Attacker,
		EConquestUnitCombatModifierType::Attack
	);
	const float CityDefenseValue = GetCityCombatValue(*DefenderCity);
	const int32 CityDamage = bCityAlreadyDefeated
		? 0
		: ConquestUnitCombat::CalculateDeterministicDamage(AttackerCombatValue, CityDefenseValue, 50.0f);

	DefenderCity->CurrentHealth = FMath::Clamp(
		DefenderCity->CurrentHealth - CityDamage,
		0,
		FMath::Max(1, DefenderCity->MaxHealth)
	);

	Attacker->CurrentMovementPoints = 0;
	Attacker->bIsFortified = false;
	Attacker->bIsSleeping = false;

	const bool bCityDefeated = DefenderCity->CurrentHealth <= 0;
	if (!bCityAlreadyDefeated && !bCityDefeated && AttackDistance <= 1)
	{
		const int32 AttackerDamage = ConquestUnitCombat::CalculateDeterministicDamage(
			GetCityCombatValue(*DefenderCity),
			AttackerCombatValue,
			25.0f
		);
		Attacker->CurrentHealth = FMath::Clamp(
			Attacker->CurrentHealth - AttackerDamage,
			0,
			Attacker->CachedMaxHealth
		);
	}

	const bool bAttackerKilled = Attacker->CurrentHealth <= 0;
	if (bAttackerKilled)
	{
		AttackerPlayer.Units.RemoveAll([AttackerUnitInstanceId](const FConquestUnitState& Candidate)
		{
			return Candidate.UnitInstanceId == AttackerUnitInstanceId;
		});
		DestroyUnitActor(*ConquestGS, AttackerUnitInstanceId);
	}

	if (!bAttackerKilled && bMeleeAttack && bCityDefeated)
	{
		ConquestGS->CityManager->CaptureCity(DefenderCityId, PlayerId);
		CheckCityOwnershipVictory(*ConquestGS);
	}
	else
	{
		ConquestGS->CityManager->DamageCity(DefenderCityId, 0);
	}

	ConquestGS->MulticastNotifyUnitAction(AttackerUnitInstanceId, PlayerId, FName(TEXT("AttackCity")));
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestGameMode::AttackTileForPlayer(
	int32 PlayerId,
	int32 AttackerUnitInstanceId,
	FIntPoint TargetCoord
)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || ConquestGS->bGameEnded || !ConquestGS->CityManager || !ConquestGS->GetHexGridModel())
	{
		return false;
	}

	FConquestPlayerEmpireState& AttackerPlayer = ConquestGS->GetPlayerEmpireMutable(PlayerId);
	FConquestUnitState* Attacker = FindUnitMutable(AttackerPlayer, AttackerUnitInstanceId);
	if (!Attacker || Attacker->OwnerPlayerId != PlayerId || Attacker->CurrentMovementPoints <= 0)
	{
		return false;
	}

	const FHexGridModel* GridModel = ConquestGS->GetHexGridModel();
	const FHexTileData* TargetTile = GridModel->GetTile(TargetCoord);
	if (
		!TargetTile ||
		TargetTile->Gameplay.OwnerPlayerId == INDEX_NONE ||
		TargetTile->Gameplay.OwnerPlayerId == PlayerId ||
		ConquestGS->CityManager->FindCityAtTile(TargetCoord) != INDEX_NONE
	)
	{
		return false;
	}

	for (const FConquestPlayerEmpireState& OtherPlayer : ConquestGS->PlayerEmpires)
	{
		for (const FConquestUnitState& OtherUnit : OtherPlayer.Units)
		{
			if (OtherUnit.TileCoord == TargetCoord)
			{
				return false;
			}
		}
	}

	FCityOwnedTileCombatState TileCombatState;
	if (!ConquestGS->CityManager->GetOwnedTileCombatState(TargetCoord, TileCombatState))
	{
		return false;
	}

	const int32 AttackDistance = GridModel->GetHexDistance(Attacker->TileCoord, TargetCoord);
	if (AttackDistance <= 0 || AttackDistance > FMath::Max(1, Attacker->CachedAttackRange))
	{
		return false;
	}

	const FIntPoint AttackerFromCoord = Attacker->TileCoord;
	const float AttackerCombatValue = ConquestUnitCombat::GetCombatValue(
		*Attacker,
		EConquestUnitCombatModifierType::Attack
	);
	const float TileDefenseValue = FMath::Max(1.0f, static_cast<float>(FMath::Max(0, TileCombatState.CombatStrength)));
	const int32 TileDamage = ConquestUnitCombat::CalculateDeterministicDamage(
		AttackerCombatValue,
		TileDefenseValue,
		50.0f
	);

	if (!ConquestGS->CityManager->DamageOwnedTile(TargetCoord, TileDamage))
	{
		return false;
	}

	ConquestGS->CityManager->GetOwnedTileCombatState(TargetCoord, TileCombatState);

	Attacker->CurrentMovementPoints = 0;
	Attacker->bIsFortified = false;
	Attacker->bIsSleeping = false;

	const bool bTileDestroyed = TileCombatState.CurrentHealth <= 0;
	if (!bTileDestroyed && AttackDistance <= 1 && TileCombatState.CombatStrength > 0)
	{
		const int32 AttackerDamage = ConquestUnitCombat::CalculateDeterministicDamage(
			TileDefenseValue,
			AttackerCombatValue,
			25.0f
		);
		Attacker->CurrentHealth = FMath::Clamp(
			Attacker->CurrentHealth - AttackerDamage,
			0,
			Attacker->CachedMaxHealth
		);
	}

	const bool bAttackerKilled = Attacker->CurrentHealth <= 0;
	if (bAttackerKilled)
	{
		AttackerPlayer.Units.RemoveAll([AttackerUnitInstanceId](const FConquestUnitState& Candidate)
		{
			return Candidate.UnitInstanceId == AttackerUnitInstanceId;
		});
		DestroyUnitActor(*ConquestGS, AttackerUnitInstanceId);
	}
	else if (bTileDestroyed)
	{
		const bool bDestroyed = ConquestGS->CityManager->DestroyOwnedTile(TargetCoord, PlayerId);
		if (bDestroyed && AttackDistance <= 1)
		{
			Attacker->TileCoord = TargetCoord;

			if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = ConquestGS->UnitActorsByInstanceId.Find(Attacker->UnitInstanceId))
			{
				if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
				{
					UnitActor->MoveToTile(TargetCoord);
				}
			}

			ConquestGS->MulticastNotifyUnitMoved(Attacker->UnitInstanceId, PlayerId, AttackerFromCoord, TargetCoord);
		}
	}

	ConquestGS->MulticastNotifyUnitAction(AttackerUnitInstanceId, PlayerId, FName(TEXT("AttackTile")));
	ConquestGS->BroadcastStateChanged();
	return true;
}

bool AConquestGameMode::CanEndCurrentTurn(FText& OutBlockReason) const
{
	const AConquestGameState* ConquestGS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	if (!ConquestGS)
	{
		OutBlockReason = NSLOCTEXT("Conquest", "EndTurnBlockedNoGameState", "Game is not ready");
		return false;
	}

	return ConquestGS->CanPlayerEndTurnFromState(ConquestGS->GetLocalPlayerId(), OutBlockReason);
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

	if (ConquestGS->TurnManager->CurrentPhase == EConquestTurnPhase::AwaitingFirstCity)
	{
		if (!IsTileInPlayerStartRegion(*ConquestGS, PlayerId, TileCoord))
		{
			return false;
		}
	}

	const bool bFounded = ConquestGS->CityManager->FoundCity(PlayerId, TileCoord, CityName);

	if (bFounded && HaveAllHumanPlayersFoundedStartingCities(*ConquestGS))
	{
		ConquestGS->TurnManager->BeginFirstTurn();
	}

	return bFounded;
}

bool AConquestGameMode::HaveAllHumanPlayersFoundedStartingCities(const AConquestGameState& ConquestGS) const
{
	if (!ConquestGS.CityManager)
	{
		return false;
	}

	const TArray<int32> HumanPlayerIds = GetHumanPlayerIds(ConquestGS);
	if (HumanPlayerIds.Num() <= 0)
	{
		return ConquestGS.CityManager->Cities.Num() > 0;
	}

	for (const int32 PlayerId : HumanPlayerIds)
	{
		const bool bHasCity = ConquestGS.CityManager->Cities.ContainsByPredicate(
			[PlayerId](const FCityState& City)
			{
				return City.OwnerPlayerId == PlayerId;
			}
		);

		if (!bHasCity)
		{
			return false;
		}
	}

	return true;
}

void AConquestGameMode::AssignPlayerStartRegions()
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	const FHexGridModel* GridModel = ConquestGS ? ConquestGS->GetHexGridModel() : nullptr;
	if (!ConquestGS || !GridModel)
	{
		return;
	}

	const TArray<int32> HumanPlayerIds = GetHumanPlayerIds(*ConquestGS);
	TArray<int32> PlayerIds = HumanPlayerIds;
	if (PlayerIds.Num() <= 0)
	{
		for (const FConquestPlayerEmpireState& PlayerEmpire : ConquestGS->PlayerEmpires)
		{
			if (PlayerEmpire.PlayerId != INDEX_NONE)
			{
				PlayerIds.AddUnique(PlayerEmpire.PlayerId);
			}
		}
	}

	if (PlayerIds.Num() <= 0)
	{
		PlayerIds.Add(0);
	}

	TMap<FIntPoint, int32> LandRegionSizeByCoord;
	TSet<FIntPoint> VisitedLandCoords;
	int32 LargestLandRegionSize = 0;
	auto IsStartPassableLand = [GridModel](const FIntPoint& Coord)
	{
		const FHexTileData* Tile = GridModel->GetTile(Coord);
		return
			Tile &&
			!GridModel->IsWaterTileType(Tile->TileType) &&
			Tile->TileType != EHexTileType::Mountain;
	};

	for (int32 R = 0; R < GridModel->GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < GridModel->GetGridWidth(); ++Q)
		{
			const FIntPoint SeedCoord(Q, R);
			if (VisitedLandCoords.Contains(SeedCoord) || !IsStartPassableLand(SeedCoord))
			{
				continue;
			}

			TArray<FIntPoint> RegionCoords;
			TArray<FIntPoint> Frontier;
			Frontier.Add(SeedCoord);
			VisitedLandCoords.Add(SeedCoord);

			for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
			{
				const FIntPoint CurrentCoord = Frontier[FrontierIndex];
				RegionCoords.Add(CurrentCoord);

				for (int32 Direction = 0; Direction < 6; ++Direction)
				{
					int32 NeighborQ = INDEX_NONE;
					int32 NeighborR = INDEX_NONE;
					if (!GridModel->GetNeighbourCoord(CurrentCoord.X, CurrentCoord.Y, Direction, NeighborQ, NeighborR))
					{
						continue;
					}

					const FIntPoint NeighborCoord(NeighborQ, NeighborR);
					if (!VisitedLandCoords.Contains(NeighborCoord) && IsStartPassableLand(NeighborCoord))
					{
						VisitedLandCoords.Add(NeighborCoord);
						Frontier.Add(NeighborCoord);
					}
				}
			}

			LargestLandRegionSize = FMath::Max(LargestLandRegionSize, RegionCoords.Num());
			for (const FIntPoint& RegionCoord : RegionCoords)
			{
				LandRegionSizeByCoord.Add(RegionCoord, RegionCoords.Num());
			}
		}
	}

	TArray<FIntPoint> CandidateCenters;
	for (int32 R = 0; R < GridModel->GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < GridModel->GetGridWidth(); ++Q)
		{
			const FIntPoint Coord(Q, R);
			if (IsValidStartRegionCenter(*ConquestGS, Coord))
			{
				CandidateCenters.Add(Coord);
			}
		}
	}

	ConquestGS->PlayerStartRegions.Reset();
	if (CandidateCenters.Num() <= 0)
	{
		ConquestGS->PushReplicatedState();
		return;
	}

	if (LargestLandRegionSize > 0)
	{
		const int32 MinPreferredLandRegionSize = FMath::Max(
			StartingRegionRadius * StartingRegionRadius,
			FMath::RoundToInt(static_cast<float>(LargestLandRegionSize) * 0.15f)
		);
		TArray<FIntPoint> LargeLandCandidates = CandidateCenters.FilterByPredicate(
			[&LandRegionSizeByCoord, MinPreferredLandRegionSize](const FIntPoint& Candidate)
			{
				const int32 RegionSize = LandRegionSizeByCoord.FindRef(Candidate);
				return RegionSize >= MinPreferredLandRegionSize;
			}
		);

		if (LargeLandCandidates.Num() >= PlayerIds.Num())
		{
			CandidateCenters = MoveTemp(LargeLandCandidates);
		}
	}

	const int32 MinDimension = FMath::Min(GridModel->GetGridWidth(), GridModel->GetGridHeight());
	const int32 IdealSpacing = FMath::Clamp(
		FMath::RoundToInt(static_cast<float>(MinDimension) * 0.25f),
		StartingRegionRadius + 5,
		StartingRegionRadius + 10
	);
	const int32 MinSpacing = FMath::Max(StartingRegionRadius + 1, IdealSpacing - 3);
	const int32 MaxPreferredSpacing = IdealSpacing + 5;

	for (const int32 PlayerId : PlayerIds)
	{
		FIntPoint SelectedCenter = CandidateCenters[0];
		float SelectedScore = -TNumericLimits<float>::Max();

		auto TrySelectCandidate = [&](bool bRequireMinimumSpacing, bool bRequireMaximumSpacing)
		{
			for (const FIntPoint& Candidate : CandidateCenters)
			{
				int32 ClosestStartDistance = TNumericLimits<int32>::Max();
				for (const FConquestPlayerStartRegion& ExistingRegion : ConquestGS->PlayerStartRegions)
				{
					ClosestStartDistance = FMath::Min(
						ClosestStartDistance,
						GridModel->GetHexDistance(ExistingRegion.Center, Candidate)
					);
				}

				if (
					ClosestStartDistance != TNumericLimits<int32>::Max() &&
					(
						(bRequireMinimumSpacing && ClosestStartDistance < MinSpacing) ||
						(bRequireMaximumSpacing && ClosestStartDistance > MaxPreferredSpacing)
					)
				)
				{
					continue;
				}

				const float CandidateScore =
					ScoreStartRegionCandidate(
						*ConquestGS,
						Candidate,
						ConquestGS->PlayerStartRegions,
						LandRegionSizeByCoord,
						LargestLandRegionSize,
						IdealSpacing,
						MaxPreferredSpacing
					) +
					FMath::FRandRange(0.0f, 0.01f);
				if (CandidateScore > SelectedScore)
				{
					SelectedCenter = Candidate;
					SelectedScore = CandidateScore;
				}
			}
		};

		TrySelectCandidate(true, true);

		if (SelectedScore <= -TNumericLimits<float>::Max())
		{
			TrySelectCandidate(true, false);
		}

		if (SelectedScore <= -TNumericLimits<float>::Max())
		{
			TrySelectCandidate(false, false);
		}

		FConquestPlayerStartRegion StartRegion;
		StartRegion.PlayerId = PlayerId;
		StartRegion.Center = SelectedCenter;
		StartRegion.RegionRadius = StartingRegionRadius;
		ConquestGS->PlayerStartRegions.Add(StartRegion);
		CandidateCenters.Remove(SelectedCenter);
	}

	ConquestGS->PushReplicatedState();
}

bool AConquestGameMode::IsValidStartRegionCenter(const AConquestGameState& ConquestGS, const FIntPoint& Coord) const
{
	const FHexGridModel* GridModel = ConquestGS.GetHexGridModel();
	const FHexTileData* Tile = GridModel ? GridModel->GetTile(Coord) : nullptr;
	return
		GridModel &&
		Tile &&
		Tile->Gameplay.OwnerPlayerId == INDEX_NONE &&
		!GridModel->IsWaterTileType(Tile->TileType) &&
		Tile->TileType != EHexTileType::Mountain;
}

float AConquestGameMode::ScoreStartRegionCandidate(
	const AConquestGameState& ConquestGS,
	const FIntPoint& Coord,
	const TArray<FConquestPlayerStartRegion>& ExistingRegions,
	const TMap<FIntPoint, int32>& LandRegionSizeByCoord,
	int32 LargestLandRegionSize,
	int32 IdealSpacing,
	int32 MaxPreferredSpacing
) const
{
	const FHexGridModel* GridModel = ConquestGS.GetHexGridModel();
	if (!GridModel)
	{
		return -TNumericLimits<float>::Max();
	}

	float Score = 0.0f;

	if (ExistingRegions.Num() > 0)
	{
		int32 ClosestStartDistance = TNumericLimits<int32>::Max();
		for (const FConquestPlayerStartRegion& ExistingRegion : ExistingRegions)
		{
			ClosestStartDistance = FMath::Min(
				ClosestStartDistance,
				GridModel->GetHexDistance(ExistingRegion.Center, Coord)
			);
		}

		const int32 DistanceFromIdeal = FMath::Abs(ClosestStartDistance - IdealSpacing);
		Score += FMath::Max(0.0f, 1.0f - (static_cast<float>(DistanceFromIdeal) / 8.0f)) * 1000.0f;
		if (ClosestStartDistance > MaxPreferredSpacing)
		{
			Score -= static_cast<float>(ClosestStartDistance - MaxPreferredSpacing) * 140.0f;
		}
	}

	const int32 EdgeDistance = FMath::Min(
		FMath::Min(Coord.X, Coord.Y),
		FMath::Min(GridModel->GetGridWidth() - 1 - Coord.X, GridModel->GetGridHeight() - 1 - Coord.Y)
	);
	const float MaxEdgeDistance = FMath::Max(
		1.0f,
		static_cast<float>(FMath::Min(GridModel->GetGridWidth(), GridModel->GetGridHeight())) * 0.5f
	);
	Score += FMath::Clamp(static_cast<float>(EdgeDistance) / MaxEdgeDistance, 0.0f, 1.0f) * 650.0f;

	if (LargestLandRegionSize > 0)
	{
		const float LandRegionRatio = FMath::Clamp(
			static_cast<float>(LandRegionSizeByCoord.FindRef(Coord)) / static_cast<float>(LargestLandRegionSize),
			0.0f,
			1.0f
		);
		Score += LandRegionRatio * 700.0f;
		if (LandRegionRatio < 0.15f)
		{
			Score -= (0.15f - LandRegionRatio) * 2200.0f;
		}
	}

	int32 CheckedTiles = 0;
	int32 PoorTerrainTiles = 0;
	int32 BonusResources = 0;
	int32 LuxuryResources = 0;
	int32 StrategicResources = 0;
	TSet<FName> UniqueLuxuryResources;
	TSet<FName> UniqueStrategicResources;

	for (const FIntPoint& RegionCoord : GridModel->GetCoordsInRange(Coord, StartingRegionRadius))
	{
		const FHexTileData* Tile = GridModel->GetTile(RegionCoord);
		if (!Tile)
		{
			continue;
		}

		++CheckedTiles;
		if (IsPoorStartingTerrain(Tile->TileType))
		{
			++PoorTerrainTiles;
		}

		if (!Tile->Resource.HasResource())
		{
			continue;
		}

		const FHexResourceDefinition* ResourceDefinition =
			GridModel->FindResourceDefinition(Tile->Resource.ResourceId);
		if (!ResourceDefinition)
		{
			continue;
		}

		switch (ResourceDefinition->Category)
		{
		case EHexResourceCategory::Bonus:
			++BonusResources;
			break;
		case EHexResourceCategory::Luxury:
			++LuxuryResources;
			UniqueLuxuryResources.Add(ResourceDefinition->ResourceId);
			break;
		case EHexResourceCategory::Strategic:
			++StrategicResources;
			UniqueStrategicResources.Add(ResourceDefinition->ResourceId);
			break;
		default:
			break;
		}
	}

	const float PoorTerrainDensity = CheckedTiles > 0
		? static_cast<float>(PoorTerrainTiles) / static_cast<float>(CheckedTiles)
		: 1.0f;
	Score -= PoorTerrainDensity * 450.0f;

	constexpr int32 TargetBonusResources = 3;
	constexpr int32 TargetLuxuryResources = 2;
	constexpr int32 TargetStrategicResources = 1;

	Score -= FMath::Abs(BonusResources - TargetBonusResources) * 30.0f;
	Score -= FMath::Abs(LuxuryResources - TargetLuxuryResources) * 55.0f;
	Score -= FMath::Abs(StrategicResources - TargetStrategicResources) * 40.0f;

	if (BonusResources > 0)
	{
		Score += 45.0f;
	}
	if (UniqueLuxuryResources.Num() > 0)
	{
		Score += 80.0f;
	}
	if (UniqueStrategicResources.Num() > 0)
	{
		Score += 55.0f;
	}

	if (LuxuryResources > TargetLuxuryResources + 2)
	{
		Score -= static_cast<float>(LuxuryResources - TargetLuxuryResources - 2) * 80.0f;
	}
	if (StrategicResources > TargetStrategicResources + 2)
	{
		Score -= static_cast<float>(StrategicResources - TargetStrategicResources - 2) * 65.0f;
	}

	return Score;
}

bool AConquestGameMode::IsTileInPlayerStartRegion(
	const AConquestGameState& ConquestGS,
	int32 PlayerId,
	const FIntPoint& Coord
) const
{
	const FHexGridModel* GridModel = ConquestGS.GetHexGridModel();
	if (!GridModel)
	{
		return false;
	}

	FConquestPlayerStartRegion StartRegion;
	if (!ConquestGS.GetStartRegionForPlayer(PlayerId, StartRegion))
	{
		return false;
	}

	const FHexTileData* Tile = GridModel->GetTile(Coord);
	return
		Tile &&
		Tile->Gameplay.OwnerPlayerId == INDEX_NONE &&
		!GridModel->IsWaterTileType(Tile->TileType) &&
		Tile->TileType != EHexTileType::Mountain &&
		GridModel->GetHexDistance(StartRegion.Center, Coord) <= StartRegion.RegionRadius;
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
	return ConquestGS.CanPlayerEndTurnFromState(PlayerId, OutBlockReason);
}

int32 AConquestGameMode::GetParticipatingPlayerCount(const AConquestGameState& ConquestGS) const
{
	TSet<int32> PlayerIds;
	for (const FConquestLobbyPlayerSlot& Slot : ConquestGS.LobbyPlayerSlots)
	{
		if (Slot.PlayerId != INDEX_NONE && Slot.SlotType != EConquestLobbySlotType::Closed)
		{
			PlayerIds.Add(Slot.PlayerId);
		}
	}

	for (const FConquestPlayerEmpireState& PlayerEmpire : ConquestGS.PlayerEmpires)
	{
		if (PlayerEmpire.PlayerId != INDEX_NONE)
		{
			PlayerIds.Add(PlayerEmpire.PlayerId);
		}
	}

	return PlayerIds.Num();
}

bool AConquestGameMode::CheckCityOwnershipVictory(AConquestGameState& ConquestGS)
{
	if (ConquestGS.bGameEnded || !ConquestGS.CityManager || GetParticipatingPlayerCount(ConquestGS) <= 1)
	{
		return false;
	}

	int32 WinningPlayerId = INDEX_NONE;
	for (const FCityState& City : ConquestGS.CityManager->Cities)
	{
		if (City.OwnerPlayerId == INDEX_NONE)
		{
			return false;
		}

		if (WinningPlayerId == INDEX_NONE)
		{
			WinningPlayerId = City.OwnerPlayerId;
		}
		else if (WinningPlayerId != City.OwnerPlayerId)
		{
			return false;
		}
	}

	if (WinningPlayerId == INDEX_NONE)
	{
		return false;
	}

	ConquestGS.bGameEnded = true;
	ConquestGS.WinningPlayerId = WinningPlayerId;
	ConquestGS.PushReplicatedState();
	ConquestGS.BroadcastStateChanged();
	return true;
}

void AConquestGameMode::ResetGameToMainMenu()
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS)
	{
		return;
	}

	ConquestGS->ClearUnitVisuals();

	if (ConquestGS->ActiveGridActor)
	{
		ConquestGS->ActiveGridActor->Destroy();
		ConquestGS->ActiveGridActor = nullptr;
	}

	if (ConquestGS->CityManager)
	{
		ConquestGS->CityManager->ResetCities();
	}

	if (ConquestGS->TurnManager)
	{
		ConquestGS->TurnManager->CurrentTurn = 1;
		ConquestGS->TurnManager->CurrentPhase = EConquestTurnPhase::None;
	}

	ReadyPlayerIds.Reset();
	ConquestGS->LobbyPlayerSlots.Reset();
	ConquestGS->PlayerCivilisations.Reset();
	ConquestGS->PlayerStartRegions.Reset();
	ConquestGS->PlayerEmpires.Reset();
	ConquestGS->HumanPlayer = FConquestPlayerEmpireState();
	ConquestGS->HumanPlayer.PlayerId = 0;
	ConquestGS->EnsurePlayerEmpire(0);
	ConquestGS->bGameEnded = false;
	ConquestGS->WinningPlayerId = INDEX_NONE;
	ConquestGS->ReplicatedConquestState = FConquestReplicatedGameState();
	ConquestGS->PushReplicatedState();
	ConquestGS->BroadcastStateChanged();
	ConquestGS->MulticastReturnToMainMenu();
}
