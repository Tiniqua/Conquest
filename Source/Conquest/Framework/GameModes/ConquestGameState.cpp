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

FHexGridModel* AConquestGameState::GetHexGridModelMutable()
{
	return ActiveGridActor ? &ActiveGridActor->GetMutableGridModel() : nullptr;
}

const FHexGridModel* AConquestGameState::GetHexGridModel() const
{
	return ActiveGridActor ? &ActiveGridActor->GetGridModel() : nullptr;
}