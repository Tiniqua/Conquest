
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
	if (!ConquestGS)
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

bool AConquestGameMode::ApplyUnitAugmentForPlayer(int32 PlayerId, int32 UnitInstanceId, FName AugmentId)
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->ContentManager || !ConquestGS->CityManager || AugmentId.IsNone())
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
	if (!ConquestGS || !ConquestGS->GetHexGridModel())
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
	Attacker->CurrentMovementPoints = FMath::Max(0, Attacker->CurrentMovementPoints - 1);
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

	ConquestGS->MulticastNotifyUnitAction(AttackerUnitInstanceId, PlayerId, FName(TEXT("Attack")));
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
