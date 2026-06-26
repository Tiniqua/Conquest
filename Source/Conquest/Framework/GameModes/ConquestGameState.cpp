#include "ConquestGameState.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestModifierManager.h"
#include "Conquest/Managers/ConquestPhilosophyManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/UI/ConquestHUD.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

namespace
{
	void RebuildPlayerCivilisationsFromLobby(AConquestGameState& GameState)
	{
		GameState.PlayerCivilisations.Reset();

		for (const FConquestLobbyPlayerSlot& Slot : GameState.LobbyPlayerSlots)
		{
			if (Slot.PlayerId != INDEX_NONE && Slot.SlotType != EConquestLobbySlotType::Closed && Slot.Civilisation)
			{
				GameState.PlayerCivilisations.Add(Slot.PlayerId, Slot.Civilisation);
			}
		}

		if (const TObjectPtr<UConquestCivilisationData>* HostCivilisation =
			GameState.PlayerCivilisations.Find(GameState.HumanPlayer.PlayerId))
		{
			GameState.HumanCivilisation = HostCivilisation->Get();
		}
	}

	void BuildReplicatedTileImprovements(const AConquestGameState& GameState, TArray<FConquestReplicatedTileImprovement>& OutTileImprovements)
	{
		OutTileImprovements.Reset();

		const AModularHexGridActor* GridActor = GameState.ActiveGridActor;
		if (!GridActor)
		{
			return;
		}

		const FHexGridModel& GridModel = GridActor->GetGridModel();
		const TArray<FHexTileData>& Tiles = GridModel.GetTiles();
		for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
		{
			const FHexTileData& Tile = Tiles[TileIndex];
			if (Tile.ImprovementId.IsNone())
			{
				continue;
			}

			int32 Q = INDEX_NONE;
			int32 R = INDEX_NONE;
			if (!GridModel.GetCoordFromIndex(TileIndex, Q, R))
			{
				continue;
			}

			FConquestReplicatedTileImprovement& ReplicatedImprovement = OutTileImprovements.AddDefaulted_GetRef();
			ReplicatedImprovement.Coord = FIntPoint(Q, R);
			ReplicatedImprovement.ImprovementId = Tile.ImprovementId;
		}
	}

	void ApplyReplicatedTileImprovementsToGrid(AConquestGameState& GameState)
	{
		AModularHexGridActor* GridActor = GameState.ActiveGridActor;
		if (!GridActor)
		{
			return;
		}

		FHexGridModel& GridModel = GridActor->GetMutableGridModel();
		bool bChangedAnyImprovement = false;
		TArray<FHexTileData>& Tiles = GridModel.GetMutableTiles();
		for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
		{
			FHexTileData& Tile = Tiles[TileIndex];
			if (Tile.ImprovementId.IsNone())
			{
				continue;
			}

			int32 Q = INDEX_NONE;
			int32 R = INDEX_NONE;
			if (GridModel.GetCoordFromIndex(TileIndex, Q, R))
			{
				bChangedAnyImprovement |= GridModel.SetTileImprovement(Q, R, NAME_None);
			}
		}

		for (const FConquestReplicatedTileImprovement& ReplicatedImprovement : GameState.ReplicatedConquestState.TileImprovements)
		{
			if (!ReplicatedImprovement.ImprovementId.IsNone())
			{
				bChangedAnyImprovement |= GridModel.SetTileImprovement(
					ReplicatedImprovement.Coord.X,
					ReplicatedImprovement.Coord.Y,
					ReplicatedImprovement.ImprovementId
				);
			}
		}

		if (bChangedAnyImprovement)
		{
			GridActor->RefreshPlacedTileVisualMeshes();
		}
	}
}


AConquestGameState::AConquestGameState()
{
	PrimaryActorTick.bCanEverTick = false;
	UnitActorClass = AConquestUnitActor::StaticClass();
}

void AConquestGameState::BeginPlay()
{
	Super::BeginPlay();

	HumanPlayer.PlayerId = 0;
	EnsurePlayerEmpire(HumanPlayer.PlayerId);

	// hopefully happens at right time
	if (!ActiveGridActor)
	{
		ActiveGridActor = Cast<AModularHexGridActor>(UGameplayStatics::GetActorOfClass(this, AModularHexGridActor::StaticClass()));
	}

	TurnManager = NewObject<UConquestTurnManager>(this);
	CityManager = NewObject<UConquestCityManager>(this);
	YieldManager = NewObject<UConquestYieldManager>(this);
	TechManager = NewObject<UConquestTechManager>(this);
	ModifierManager = NewObject<UConquestModifierManager>(this);
	PhilosophyManager = NewObject<UConquestPhilosophyManager>(this);
	ContentManager = NewObject<UConquestContentManager>(this);

	if (ContentManager)
	{
		ContentManager->Initialize(this);
	}
	
	if (TurnManager)
	{
		TurnManager->Initialize(this);
	}

	if (CityManager)
	{
		CityManager->Initialize(this);
	}

	if (YieldManager)
	{
		YieldManager->Initialize(this);
	}

	if (TechManager)
	{
		TechManager->Initialize(this);
	}

	if (ModifierManager)
	{
		ModifierManager->Initialize(this);
	}

	if (PhilosophyManager)
	{
		PhilosophyManager->Initialize(this);
	}

	UE_LOG(LogTemp, Warning, TEXT("GameState class at runtime: %s"), *GetClass()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("BuildingTable: %s"), *GetNameSafe(BuildingTable));
	UE_LOG(LogTemp, Warning, TEXT("UnitTable: %s"), *GetNameSafe(UnitTable));
	UE_LOG(LogTemp, Warning, TEXT("UnitAugmentTable: %s"), *GetNameSafe(UnitAugmentTable));
	UE_LOG(LogTemp, Warning, TEXT("TechTable: %s"), *GetNameSafe(TechTable));
	UE_LOG(LogTemp, Warning, TEXT("PhilosophyTable: %s"), *GetNameSafe(PhilosophyTable));
	UE_LOG(LogTemp, Warning, TEXT("HumanCivilisation: %s"), *GetNameSafe(HumanCivilisation));

	if (!HasAuthority() && ReplicatedConquestState.PlayerEmpires.Num() > 0)
	{
		OnRep_ReplicatedConquestState();
	}
}

void AConquestGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AConquestGameState, ReplicatedConquestState);
	DOREPLIFETIME(AConquestGameState, ActiveGridActor);
}

void AConquestGameState::BroadcastStateChanged()
{
	if (HasAuthority())
	{
		PushReplicatedState();
		RebuildUnitVisualsFromReplicatedState();
	}

	OnConquestStateChanged.Broadcast();
}

void AConquestGameState::PushReplicatedState()
{
	if (!HasAuthority())
	{
		return;
	}

	ReplicatedConquestState.PlayerEmpires = PlayerEmpires;
	ReplicatedConquestState.LobbyPlayerSlots = LobbyPlayerSlots;
	ReplicatedConquestState.AvailableCivilisations = AvailableCivilisations;
	ReplicatedConquestState.PlayerStartRegions = PlayerStartRegions;
	ReplicatedConquestState.bGameEnded = bGameEnded;
	ReplicatedConquestState.WinningPlayerId = WinningPlayerId;
	BuildReplicatedTileImprovements(*this, ReplicatedConquestState.TileImprovements);

	if (TurnManager)
	{
		ReplicatedConquestState.CurrentTurn = TurnManager->CurrentTurn;
		ReplicatedConquestState.CurrentPhase = TurnManager->CurrentPhase;
	}

	if (CityManager)
	{
		ReplicatedConquestState.Cities = CityManager->Cities;
	}

	ForceNetUpdate();
}

void AConquestGameState::ApplyReplicatedTileImprovements()
{
	ApplyReplicatedTileImprovementsToGrid(*this);
}

void AConquestGameState::OnRep_ReplicatedConquestState()
{
	PlayerEmpires = ReplicatedConquestState.PlayerEmpires;
	LobbyPlayerSlots = ReplicatedConquestState.LobbyPlayerSlots;
	AvailableCivilisations = ReplicatedConquestState.AvailableCivilisations;
	PlayerStartRegions = ReplicatedConquestState.PlayerStartRegions;
	bGameEnded = ReplicatedConquestState.bGameEnded;
	WinningPlayerId = ReplicatedConquestState.WinningPlayerId;
	RebuildPlayerCivilisationsFromLobby(*this);

	if (PlayerEmpires.Num() > 0)
	{
		HumanPlayer = GetPlayerEmpire(GetLocalPlayerId());
		if (const TObjectPtr<UConquestCivilisationData>* LocalCivilisation = PlayerCivilisations.Find(HumanPlayer.PlayerId))
		{
			HumanCivilisation = LocalCivilisation->Get();
		}
	}

	if (TurnManager)
	{
		TurnManager->CurrentTurn = ReplicatedConquestState.CurrentTurn;
		TurnManager->CurrentPhase = ReplicatedConquestState.CurrentPhase;
	}

	if (CityManager)
	{
		CityManager->Cities = ReplicatedConquestState.Cities;
	}

	ApplyReplicatedTileImprovements();
	RebuildCityVisualsFromReplicatedState();
	RebuildUnitVisualsFromReplicatedState();

	OnConquestStateChanged.Broadcast();
}

void AConquestGameState::OnRep_ActiveGridActor()
{
	ApplyReplicatedTileImprovements();
	RebuildCityVisualsFromReplicatedState();
	RebuildUnitVisualsFromReplicatedState();
	OnConquestStateChanged.Broadcast();
}

FConquestPlayerEmpireState& AConquestGameState::GetHumanPlayerMutable()
{
	return GetPlayerEmpireMutable(GetLocalPlayerId());
}

const FConquestPlayerEmpireState& AConquestGameState::GetHumanPlayer() const
{
	return GetPlayerEmpire(GetLocalPlayerId());
}

FConquestPlayerEmpireState& AConquestGameState::GetPlayerEmpireMutable(int32 PlayerId)
{
	EnsurePlayerEmpire(PlayerId);

	for (FConquestPlayerEmpireState& PlayerEmpire : PlayerEmpires)
	{
		if (PlayerEmpire.PlayerId == PlayerId)
		{
			HumanPlayer = PlayerEmpire.PlayerId == GetLocalPlayerId() ? PlayerEmpire : HumanPlayer;
			return PlayerEmpire;
		}
	}

	return HumanPlayer;
}

const FConquestPlayerEmpireState& AConquestGameState::GetPlayerEmpire(int32 PlayerId) const
{
	for (const FConquestPlayerEmpireState& PlayerEmpire : PlayerEmpires)
	{
		if (PlayerEmpire.PlayerId == PlayerId)
		{
			return PlayerEmpire;
		}
	}

	return HumanPlayer;
}

int32 AConquestGameState::GetLocalPlayerId() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return HumanPlayer.PlayerId;
	}

	const AConquestPlayerController* ConquestPC =
		Cast<AConquestPlayerController>(World->GetFirstPlayerController());
	return ConquestPC ? ConquestPC->GetAssignedPlayerId() : HumanPlayer.PlayerId;
}

bool AConquestGameState::GetStartRegionForPlayer(int32 PlayerId, FConquestPlayerStartRegion& OutStartRegion) const
{
	for (const FConquestPlayerStartRegion& StartRegion : PlayerStartRegions)
	{
		if (StartRegion.PlayerId == PlayerId)
		{
			OutStartRegion = StartRegion;
			return true;
		}
	}

	OutStartRegion = FConquestPlayerStartRegion();
	return false;
}

void AConquestGameState::EnsurePlayerEmpire(int32 PlayerId)
{
	if (PlayerId == INDEX_NONE)
	{
		return;
	}

	for (FConquestPlayerEmpireState& PlayerEmpire : PlayerEmpires)
	{
		if (PlayerEmpire.PlayerId == PlayerId)
		{
			return;
		}
	}

	FConquestPlayerEmpireState NewEmpire;
	NewEmpire.PlayerId = PlayerId;
	NewEmpire.NextUnitInstanceId = PlayerId > 0 ? (PlayerId * 100000) + 1 : 1;
	PlayerEmpires.Add(NewEmpire);

	if (PlayerId == HumanPlayer.PlayerId)
	{
		HumanPlayer = NewEmpire;
	}
}

void AConquestGameState::ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings)
{
	LobbyPlayerSlots = SetupSettings.PlayerSlots;
	ReplicatedConquestState.TurnMode = SetupSettings.TurnMode;
	RebuildPlayerCivilisationsFromLobby(*this);

	for (const FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.PlayerId == INDEX_NONE || Slot.SlotType == EConquestLobbySlotType::Closed || !Slot.Civilisation)
		{
			continue;
		}

		EnsurePlayerEmpire(Slot.PlayerId);
	}

	if (!PlayerCivilisations.Contains(HumanPlayer.PlayerId) && HumanCivilisation)
	{
		PlayerCivilisations.Add(HumanPlayer.PlayerId, HumanCivilisation);
	}

	BroadcastStateChanged();
}

void AConquestGameState::SetAvailableCivilisations(const TArray<UConquestCivilisationData*>& InAvailableCivilisations)
{
	if (!HasAuthority())
	{
		return;
	}

	AvailableCivilisations.Reset();
	for (UConquestCivilisationData* Civilisation : InAvailableCivilisations)
	{
		if (Civilisation)
		{
			AvailableCivilisations.Add(Civilisation);
		}
	}

	BroadcastStateChanged();
}

void AConquestGameState::SetLobbyPlayerSlots(const TArray<FConquestLobbyPlayerSlot>& InLobbyPlayerSlots)
{
	if (!HasAuthority())
	{
		return;
	}

	LobbyPlayerSlots = InLobbyPlayerSlots;
	RebuildPlayerCivilisationsFromLobby(*this);
	BroadcastStateChanged();
}

bool AConquestGameState::SetLobbySlotCivilisationForPlayer(
	int32 RequestingPlayerId,
	int32 SlotIndex,
	UConquestCivilisationData* Civilisation
)
{
	if (!HasAuthority() || !LobbyPlayerSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FConquestLobbyPlayerSlot& Slot = LobbyPlayerSlots[SlotIndex];
	const bool bRequesterIsHost = RequestingPlayerId == 0;
	const bool bRequesterOwnsSlot =
		Slot.SlotType == EConquestLobbySlotType::Human &&
		Slot.PlayerId == RequestingPlayerId;
	const bool bHostCanEditSlot =
		bRequesterIsHost &&
		(
			Slot.SlotType != EConquestLobbySlotType::Human ||
			Slot.PlayerId == RequestingPlayerId
		);

	if (!bRequesterOwnsSlot && !bHostCanEditSlot)
	{
		return false;
	}

	Slot.Civilisation = Civilisation;
	Slot.bIsReady = false;
	ReplicatedConquestState.ReadyPlayerIds.Remove(Slot.PlayerId);
	RebuildPlayerCivilisationsFromLobby(*this);
	BroadcastStateChanged();
	return true;
}

void AConquestGameState::SetLobbyPlayerReady(int32 PlayerId, bool bReady)
{
	if (!HasAuthority())
	{
		return;
	}

	for (FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.SlotType == EConquestLobbySlotType::Human && Slot.PlayerId == PlayerId)
		{
			if (bReady && !Slot.Civilisation)
			{
				return;
			}
			Slot.bIsReady = bReady;
			break;
		}
	}

	if (bReady)
	{
		ReplicatedConquestState.ReadyPlayerIds.AddUnique(PlayerId);
	}
	else
	{
		ReplicatedConquestState.ReadyPlayerIds.Remove(PlayerId);
	}

	BroadcastStateChanged();
}

int32 AConquestGameState::GetReadyHumanPlayerCount() const
{
	int32 ReadyCount = 0;
	for (const FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.SlotType == EConquestLobbySlotType::Human && Slot.bIsReady)
		{
			++ReadyCount;
		}
	}
	return ReadyCount;
}

int32 AConquestGameState::GetRequiredReadyHumanPlayerCount() const
{
	int32 RequiredCount = 0;
	for (const FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.SlotType == EConquestLobbySlotType::Human)
		{
			++RequiredCount;
		}
	}
	return RequiredCount;
}

bool AConquestGameState::IsPlayerWaitingForOtherPlayers(int32 PlayerId) const
{
	if (
		!TurnManager ||
		TurnManager->CurrentPhase != EConquestTurnPhase::PlayerActions ||
		!ReplicatedConquestState.ReadyPlayerIds.Contains(PlayerId)
	)
	{
		return false;
	}

	int32 HumanPlayerCount = 0;
	for (const FConquestLobbyPlayerSlot& LobbySlot : LobbyPlayerSlots)
	{
		if (LobbySlot.PlayerId != INDEX_NONE && LobbySlot.SlotType == EConquestLobbySlotType::Human)
		{
			++HumanPlayerCount;
		}
	}

	if (HumanPlayerCount <= 0)
	{
		for (const FConquestLobbyPlayerSlot& LobbySlot : ReplicatedConquestState.LobbyPlayerSlots)
		{
			if (LobbySlot.PlayerId != INDEX_NONE && LobbySlot.SlotType == EConquestLobbySlotType::Human)
			{
				++HumanPlayerCount;
			}
		}
	}

	return HumanPlayerCount > 1;
}

UConquestCivilisationData* AConquestGameState::GetCivilisationForPlayer(int32 PlayerId) const
{
	if (const TObjectPtr<UConquestCivilisationData>* FoundCivilisation = PlayerCivilisations.Find(PlayerId))
	{
		return FoundCivilisation->Get();
	}

	for (const FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.PlayerId == PlayerId && Slot.SlotType != EConquestLobbySlotType::Closed && Slot.Civilisation)
		{
			return Slot.Civilisation;
		}
	}

	if (PlayerId == HumanPlayer.PlayerId)
	{
		return HumanCivilisation;
	}

	return nullptr;
}

bool AConquestGameState::GetEndTurnBlockerForPlayer(int32 PlayerId, FConquestEndTurnBlocker& OutBlocker) const
{
	OutBlocker = FConquestEndTurnBlocker();

	if (!TurnManager)
	{
		OutBlocker.Type = EConquestEndTurnBlockType::GameNotReady;
		OutBlocker.Message = NSLOCTEXT("Conquest", "EndTurnBlockedNoGameState", "Game is not ready");
		return true;
	}

	if (TurnManager->CurrentPhase != EConquestTurnPhase::PlayerActions)
	{
		OutBlocker.Type = EConquestEndTurnBlockType::WrongPhase;
		if (TurnManager->CurrentPhase == EConquestTurnPhase::AwaitingFirstCity)
		{
			const bool bPlayerHasStartingCity =
				CityManager &&
				CityManager->Cities.ContainsByPredicate(
					[PlayerId](const FCityState& City)
					{
						return City.OwnerPlayerId == PlayerId;
					}
				);
			OutBlocker.Message = bPlayerHasStartingCity
				? NSLOCTEXT("Conquest", "EndTurnBlockedWaitingForStartingCities", "Waiting for Players")
				: NSLOCTEXT("Conquest", "EndTurnBlockedPickStartingHex", "Pick your Starting Hex");
		}
		else
		{
			OutBlocker.Message = NSLOCTEXT("Conquest", "EndTurnBlockedWrongPhase", "It is not currently the player action phase");
		}
		return true;
	}

	const FConquestPlayerEmpireState& Player = GetPlayerEmpire(PlayerId);
	if (Player.CurrentResearchId.IsNone())
	{
		OutBlocker.Type = EConquestEndTurnBlockType::Research;
		OutBlocker.Message = NSLOCTEXT("Conquest", "EndTurnBlockedResearch", "Select research");
		return true;
	}

	if (PhilosophyManager)
	{
		const int32 NextPhilosophyCost = PhilosophyManager->GetNextPhilosophyCultureCost(PlayerId);
		if (
			Player.StoredYields.Culture >= NextPhilosophyCost &&
			PhilosophyManager->GetAvailablePhilosophyIds(PlayerId).Num() > 0
		)
		{
			OutBlocker.Type = EConquestEndTurnBlockType::Philosophy;
			OutBlocker.Message = NSLOCTEXT("Conquest", "EndTurnBlockedPhilosophy", "Choose Philosophy");
			return true;
		}
	}

	if (CityManager)
	{
		for (const FCityState& City : CityManager->Cities)
		{
			if (City.OwnerPlayerId == PlayerId && !City.CurrentProduction.IsValid())
			{
				OutBlocker.Type = EConquestEndTurnBlockType::CityProduction;
				OutBlocker.CityId = City.CityId;
				OutBlocker.Message = FText::Format(
					NSLOCTEXT("Conquest", "EndTurnBlockedCityProduction", "{0} needs production"),
					FText::FromName(City.CityName)
				);
				return true;
			}
		}

		for (const FCityState& City : CityManager->Cities)
		{
			if (City.OwnerPlayerId != PlayerId || City.PendingBorderExpansions <= 0)
			{
				continue;
			}

			if (CityManager->GetExpansionCandidateTiles(City.CityId).Num() <= 0)
			{
				continue;
			}

			OutBlocker.Type = EConquestEndTurnBlockType::CityGrowth;
			OutBlocker.CityId = City.CityId;
			OutBlocker.Message = FText::Format(
				NSLOCTEXT("Conquest", "EndTurnBlockedCityGrowth", "{0} has grown"),
				FText::FromName(City.CityName)
			);
			return true;
		}
	}

	for (const FConquestUnitState& Unit : Player.Units)
	{
		if (Unit.OwnerPlayerId == PlayerId && !Unit.bIsSleeping && Unit.CurrentMovementPoints > 0)
		{
			OutBlocker.Type = EConquestEndTurnBlockType::UnitOrders;
			OutBlocker.UnitInstanceId = Unit.UnitInstanceId;
			OutBlocker.Message = NSLOCTEXT("Conquest", "EndTurnBlockedUnitOrders", "Unit pending orders");
			return true;
		}
	}

	return false;
}

bool AConquestGameState::CanPlayerEndTurnFromState(int32 PlayerId, FText& OutBlockReason) const
{
	FConquestEndTurnBlocker Blocker;
	if (GetEndTurnBlockerForPlayer(PlayerId, Blocker))
	{
		OutBlockReason = Blocker.Message;
		return false;
	}

	OutBlockReason = FText::GetEmpty();
	return true;
}

void AConquestGameState::RebuildCityVisualsFromReplicatedState()
{
	if (!ActiveGridActor || !CityManager)
	{
		return;
	}

	ActiveGridActor->ClearCityPlaceholders();
	ActiveGridActor->ClearCivilisationBorders();

	FHexGridModel* GridModel = GetHexGridModelMutable();
	if (GridModel)
	{
		for (FHexTileData& Tile : GridModel->GetMutableTiles())
		{
			Tile.Gameplay.OwnerPlayerId = INDEX_NONE;
			Tile.Gameplay.OwningCityId = INDEX_NONE;
			Tile.Gameplay.bIsWorked = false;
			Tile.Gameplay.WorkedByCityId = INDEX_NONE;
			Tile.Gameplay.CurrentHealth = 100;
			Tile.Gameplay.MaxHealth = 100;
			Tile.Gameplay.CombatStrength = 0;
			Tile.Gameplay.HealRatePerTurn = 5;
			Tile.Gameplay.DefenderModifier = 1.0f;
		}
	}

	for (const FCityState& City : CityManager->Cities)
	{
		if (GridModel)
		{
			for (const FIntPoint& Coord : City.OwnedTiles)
			{
				if (FHexTileData* Tile = GridModel->GetTileMutable(Coord))
				{
					Tile->Gameplay.OwnerPlayerId = City.OwnerPlayerId;
					Tile->Gameplay.OwningCityId = City.CityId;

					const FCityOwnedTileCombatState* TileCombatState =
						City.OwnedTileCombatStates.FindByPredicate([Coord](const FCityOwnedTileCombatState& Candidate)
						{
							return Candidate.Coord == Coord;
						});
					if (TileCombatState)
					{
						Tile->Gameplay.CurrentHealth = TileCombatState->CurrentHealth;
						Tile->Gameplay.MaxHealth = TileCombatState->MaxHealth;
						Tile->Gameplay.CombatStrength = TileCombatState->CombatStrength;
						Tile->Gameplay.HealRatePerTurn = TileCombatState->HealRatePerTurn;
						Tile->Gameplay.DefenderModifier = TileCombatState->DefenderModifier;
					}
				}
			}

			for (const FIntPoint& Coord : City.WorkedTiles)
			{
				if (FHexTileData* Tile = GridModel->GetTileMutable(Coord))
				{
					Tile->Gameplay.bIsWorked = true;
					Tile->Gameplay.WorkedByCityId = City.CityId;
				}
			}
		}

		UStaticMesh* CityMesh = nullptr;
		UMaterialInterface* CityMaterial = nullptr;
		UMaterialInterface* CivilisationThemeMaterial = nullptr;
		FLinearColor CivilisationThemeColor = FLinearColor::White;
		bool bOverrideCityScale = false;
		FVector CityScale = FVector::OneVector;
		if (const UConquestCivilisationData* Civilisation = GetCivilisationForPlayer(City.OwnerPlayerId))
		{
			CityMesh = Civilisation->CityMesh;
			CityMaterial = Civilisation->CityMeshMaterialOverride;
			CivilisationThemeMaterial = Civilisation->CityLabelMaterial
				? Civilisation->CityLabelMaterial
				: Civilisation->BorderMaterial;
			CivilisationThemeColor = Civilisation->ThemeColor;
			bOverrideCityScale = Civilisation->bOverrideCityMeshScale;
			CityScale = Civilisation->CityMeshScaleOverride;
		}

		ActiveGridActor->AddCityPlaceholder(
			City.CityId,
			City.CenterTile,
			CityMesh,
			CityMaterial,
			bOverrideCityScale,
			CityScale
		);
		ActiveGridActor->AddOrUpdateCityWorldLabel(
			City.CityId,
			City.CenterTile,
			City.CityName,
			City.Population,
			City.CurrentHealth,
			City.MaxHealth,
			City.CachedStrength,
			FMath::Clamp(
				City.FoodStored / static_cast<float>(FMath::Max(1, City.CachedFoodRequiredForNextPopulation)),
				0.0f,
				1.0f
			),
			CivilisationThemeMaterial,
			CivilisationThemeColor
		);

		for (const FCityOwnedTileCombatState& TileCombatState : City.OwnedTileCombatStates)
		{
			if (TileCombatState.Coord != City.CenterTile && City.OwnedTiles.Contains(TileCombatState.Coord))
			{
				ActiveGridActor->AddOrUpdateTileHealthBar(
					TileCombatState.Coord,
					TileCombatState.CurrentHealth,
					TileCombatState.MaxHealth,
					TileCombatState.CombatStrength
				);
			}
		}
	}

	for (const TPair<int32, TObjectPtr<UConquestCivilisationData>>& Pair : PlayerCivilisations)
	{
		if (Pair.Value)
		{
			TArray<FIntPoint> OwnedTiles;
			for (const FCityState& City : CityManager->Cities)
			{
				if (City.OwnerPlayerId == Pair.Key)
				{
					OwnedTiles.Append(City.OwnedTiles);
				}
			}

			ActiveGridActor->RebuildCivilisationBordersForTiles(
				OwnedTiles,
				Pair.Value->BorderMaterial,
				Pair.Value->BorderFillMaterial
			);
		}
	}
}

void AConquestGameState::RebuildUnitVisualsFromReplicatedState()
{
	if (!ActiveGridActor || !ContentManager)
	{
		return;
	}

	TSet<int32> ReplicatedUnitIds;

	for (const FConquestPlayerEmpireState& PlayerEmpire : PlayerEmpires)
	{
		for (const FConquestUnitState& UnitState : PlayerEmpire.Units)
		{
			if (UnitState.UnitInstanceId == INDEX_NONE)
			{
				continue;
			}

			const FConquestUnitRow* UnitRow = ContentManager->FindUnit(UnitState.UnitId);
			if (!UnitRow)
			{
				continue;
			}

			ReplicatedUnitIds.Add(UnitState.UnitInstanceId);

			AConquestUnitActor* UnitActor = nullptr;
			if (TObjectPtr<AConquestUnitActor>* ExistingActorPtr = UnitActorsByInstanceId.Find(UnitState.UnitInstanceId))
			{
				UnitActor = ExistingActorPtr->Get();
			}

			if (!UnitActor)
			{
				TSubclassOf<AConquestUnitActor> ActorClass = UnitActorClass;
				if (!ActorClass)
				{
					ActorClass = AConquestUnitActor::StaticClass();
				}

				UnitActor = GetWorld()
					? GetWorld()->SpawnActor<AConquestUnitActor>(ActorClass, ActiveGridActor->GetActorTransform())
					: nullptr;
				if (!UnitActor)
				{
					continue;
				}

				UnitActorsByInstanceId.Add(UnitState.UnitInstanceId, UnitActor);
			}

			if (ActiveGridActor->UnitWorldIconWidgetClass)
			{
				UnitActor->SetUnitWorldIconWidgetClass(ActiveGridActor->UnitWorldIconWidgetClass);
			}

			const FText UnitName = !UnitRow->DisplayName.IsEmpty()
				? UnitRow->DisplayName
				: FText::FromName(UnitState.UnitId);
			FText CivilisationName = FText::GetEmpty();
			FLinearColor UnitDisplayColor = FLinearColor::White;
			UMaterialInterface* UnitIconMaterial = nullptr;
			if (const UConquestCivilisationData* Civilisation = GetCivilisationForPlayer(UnitState.OwnerPlayerId))
			{
				CivilisationName = Civilisation->CivilisationName;
				UnitDisplayColor = Civilisation->ThemeColor;
				UnitIconMaterial = Civilisation->CityLabelMaterial
					? Civilisation->CityLabelMaterial.Get()
					: Civilisation->BorderMaterial.Get();
			}

			UnitActor->InitializeUnit(
				UnitState,
				*UnitRow,
				ActiveGridActor,
				UnitName,
				CivilisationName,
				UnitDisplayColor,
				UnitIconMaterial
			);
		}
	}

	for (auto It = UnitActorsByInstanceId.CreateIterator(); It; ++It)
	{
		if (!ReplicatedUnitIds.Contains(It.Key()))
		{
			if (AConquestUnitActor* UnitActor = It.Value().Get())
			{
				UnitActor->Destroy();
			}
			It.RemoveCurrent();
		}
	}
}

void AConquestGameState::ClearUnitVisuals()
{
	for (TPair<int32, TObjectPtr<AConquestUnitActor>>& Pair : UnitActorsByInstanceId)
	{
		if (AConquestUnitActor* UnitActor = Pair.Value.Get())
		{
			UnitActor->Destroy();
		}
	}

	UnitActorsByInstanceId.Reset();
	SelectedUnitInstanceId = INDEX_NONE;
}

FHexGridModel* AConquestGameState::GetHexGridModelMutable()
{
	return ActiveGridActor ? &ActiveGridActor->GetMutableGridModel() : nullptr;
}

const FHexGridModel* AConquestGameState::GetHexGridModel() const
{
	return ActiveGridActor ? &ActiveGridActor->GetGridModel() : nullptr;
}

void AConquestGameState::MulticastNotifyUnitMoved_Implementation(
	int32 UnitInstanceId,
	int32 PlayerId,
	FIntPoint FromCoord,
	FIntPoint ToCoord
)
{
	if (!HasAuthority())
	{
		if (TObjectPtr<AConquestUnitActor>* UnitActorPtr = UnitActorsByInstanceId.Find(UnitInstanceId))
		{
			if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
			{
				UnitActor->MoveToTile(ToCoord);
			}
		}
	}

	OnConquestUnitMoved.Broadcast(UnitInstanceId, PlayerId, FromCoord, ToCoord);
}

void AConquestGameState::MulticastNotifyUnitAction_Implementation(
	int32 UnitInstanceId,
	int32 PlayerId,
	FName ActionId
)
{
	OnConquestUnitAction.Broadcast(UnitInstanceId, PlayerId, ActionId);
}

void AConquestGameState::MulticastReturnToMainMenu_Implementation()
{
	ClearUnitVisuals();

	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PlayerController->GetHUD()))
		{
			ConquestHUD->ShowMainMenu();
		}
	}
}
