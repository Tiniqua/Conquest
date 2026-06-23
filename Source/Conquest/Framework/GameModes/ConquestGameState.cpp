#include "ConquestGameState.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


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

	UE_LOG(LogTemp, Warning, TEXT("GameState class at runtime: %s"), *GetClass()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("BuildingTable: %s"), *GetNameSafe(BuildingTable));
	UE_LOG(LogTemp, Warning, TEXT("UnitTable: %s"), *GetNameSafe(UnitTable));
	UE_LOG(LogTemp, Warning, TEXT("UnitAugmentTable: %s"), *GetNameSafe(UnitAugmentTable));
	UE_LOG(LogTemp, Warning, TEXT("TechTable: %s"), *GetNameSafe(TechTable));
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

void AConquestGameState::OnRep_ReplicatedConquestState()
{
	PlayerEmpires = ReplicatedConquestState.PlayerEmpires;
	LobbyPlayerSlots = ReplicatedConquestState.LobbyPlayerSlots;
	AvailableCivilisations = ReplicatedConquestState.AvailableCivilisations;
	PlayerCivilisations.Reset();

	for (const FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.PlayerId != INDEX_NONE && Slot.Civilisation)
		{
			PlayerCivilisations.Add(Slot.PlayerId, Slot.Civilisation);
		}
	}

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

	RebuildCityVisualsFromReplicatedState();
	RebuildUnitVisualsFromReplicatedState();

	OnConquestStateChanged.Broadcast();
}

void AConquestGameState::OnRep_ActiveGridActor()
{
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
	PlayerCivilisations.Reset();
	ReplicatedConquestState.TurnMode = SetupSettings.TurnMode;

	for (const FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.PlayerId == INDEX_NONE || Slot.SlotType == EConquestLobbySlotType::Closed || !Slot.Civilisation)
		{
			continue;
		}

		PlayerCivilisations.Add(Slot.PlayerId, Slot.Civilisation);
		EnsurePlayerEmpire(Slot.PlayerId);
	}

	if (const TObjectPtr<UConquestCivilisationData>* HostCivilisation = PlayerCivilisations.Find(HumanPlayer.PlayerId))
	{
		HumanCivilisation = HostCivilisation->Get();
	}
	else if (HumanCivilisation)
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

UConquestCivilisationData* AConquestGameState::GetCivilisationForPlayer(int32 PlayerId) const
{
	if (const TObjectPtr<UConquestCivilisationData>* FoundCivilisation = PlayerCivilisations.Find(PlayerId))
	{
		return FoundCivilisation->Get();
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
		OutBlocker.Message = NSLOCTEXT("Conquest", "EndTurnBlockedWrongPhase", "It is not currently the player action phase");
		return true;
	}

	const FConquestPlayerEmpireState& Player = GetPlayerEmpire(PlayerId);
	if (Player.CurrentResearchId.IsNone())
	{
		OutBlocker.Type = EConquestEndTurnBlockType::Research;
		OutBlocker.Message = NSLOCTEXT("Conquest", "EndTurnBlockedResearch", "Select research");
		return true;
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
			CivilisationThemeMaterial,
			CivilisationThemeColor
		);
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
