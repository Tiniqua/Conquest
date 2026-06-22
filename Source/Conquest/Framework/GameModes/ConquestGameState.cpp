#include "ConquestGameState.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Kismet/GameplayStatics.h"


AConquestGameState::AConquestGameState()
{
	PrimaryActorTick.bCanEverTick = false;
	UnitActorClass = AConquestUnitActor::StaticClass();
}

void AConquestGameState::BeginPlay()
{
	Super::BeginPlay();

	HumanPlayer.PlayerId = 0;

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
}

void AConquestGameState::BroadcastStateChanged()
{
	OnConquestStateChanged.Broadcast();
}

FConquestPlayerEmpireState& AConquestGameState::GetHumanPlayerMutable()
{
	return HumanPlayer;
}

const FConquestPlayerEmpireState& AConquestGameState::GetHumanPlayer() const
{
	return HumanPlayer;
}

void AConquestGameState::ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings)
{
	LobbyPlayerSlots = SetupSettings.PlayerSlots;
	PlayerCivilisations.Reset();

	for (const FConquestLobbyPlayerSlot& Slot : LobbyPlayerSlots)
	{
		if (Slot.PlayerId == INDEX_NONE || Slot.SlotType == EConquestLobbySlotType::Closed || !Slot.Civilisation)
		{
			continue;
		}

		PlayerCivilisations.Add(Slot.PlayerId, Slot.Civilisation);
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
