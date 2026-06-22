#include "ConquestGameState.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Player/ConquestPlayerController.h"
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
}

void AConquestGameState::BroadcastStateChanged()
{
	if (HasAuthority())
	{
		PushReplicatedState();
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

	if (TurnManager)
	{
		ReplicatedConquestState.CurrentTurn = TurnManager->CurrentTurn;
		ReplicatedConquestState.CurrentPhase = TurnManager->CurrentPhase;
	}

	if (CityManager)
	{
		ReplicatedConquestState.Cities = CityManager->Cities;
	}
}

void AConquestGameState::OnRep_ReplicatedConquestState()
{
	PlayerEmpires = ReplicatedConquestState.PlayerEmpires;
	LobbyPlayerSlots = ReplicatedConquestState.LobbyPlayerSlots;
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
