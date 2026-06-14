#include "Conquest/Managers/ConquestTurnManager.h"

#include "ConquestCityManager.h"
#include "ConquestTechManager.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"


void UConquestTurnManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

void UConquestTurnManager::BeginGameSetup()
{
	CurrentTurn = 0;
	SetPhase(EConquestTurnPhase::GameSetup);
}

void UConquestTurnManager::EnterAwaitingFirstCity()
{
	SetPhase(EConquestTurnPhase::AwaitingFirstCity);
}

void UConquestTurnManager::BeginFirstTurn()
{
	CurrentTurn = 1;
	OnTurnChanged.Broadcast(CurrentTurn);
	StartTurnProcessing();
}

void UConquestTurnManager::RequestEndTurn()
{
	if (CurrentPhase != EConquestTurnPhase::PlayerActions)
	{
		return;
	}

	EndTurnProcessing();
}

void UConquestTurnManager::SetPhase(EConquestTurnPhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnTurnPhaseChanged.Broadcast(NewPhase);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}
}

void UConquestTurnManager::StartTurnProcessing()
{
	SetPhase(EConquestTurnPhase::StartTurnProcessing);

	if (!GameStateRef)
	{
		return;
	}

	if (GameStateRef->CityManager)
	{
		GameStateRef->CityManager->ProcessCitiesAtStartOfTurn(0);
	}

	if (GameStateRef->TechManager)
	{
		GameStateRef->TechManager->ProcessResearchAtStartOfTurn(0);
	}

	EnterPlayerActions();
}

void UConquestTurnManager::EnterPlayerActions()
{
	SetPhase(EConquestTurnPhase::PlayerActions);
}

void UConquestTurnManager::EndTurnProcessing()
{
	SetPhase(EConquestTurnPhase::EndTurnProcessing);

	++CurrentTurn;
	OnTurnChanged.Broadcast(CurrentTurn);

	StartTurnProcessing();
}