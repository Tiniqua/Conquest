#include "Conquest/Managers/ConquestTurnManager.h"

#include "ConquestCityManager.h"
#include "ConquestTechManager.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestYieldManager.h"


void UConquestTurnManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

void UConquestTurnManager::BeginGameSetup()
{
	CurrentTurn = 1;
	OnTurnChanged.Broadcast(CurrentTurn);
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
		GameStateRef->BroadcastStateChangedWithVisuals(EConquestStateVisualDirtyFlags::None);
	}
}

void UConquestTurnManager::StartTurnProcessing()
{
	if (GameStateRef)
	{
		GameStateRef->BeginStateChangeBatch();
	}

	SetPhase(EConquestTurnPhase::StartTurnProcessing);

	if (!GameStateRef)
	{
		return;
	}

	for (const FConquestPlayerEmpireState& PlayerEmpire : GameStateRef->PlayerEmpires)
	{
		const int32 PlayerId = PlayerEmpire.PlayerId;
		if (PlayerId == INDEX_NONE)
		{
			continue;
		}

		if (GameStateRef->CityManager)
		{
			GameStateRef->CityManager->ProcessCitiesAtStartOfTurn(PlayerId);
			GameStateRef->CityManager->ProcessUnitsAtStartOfTurn(PlayerId);
		}

		if (GameStateRef->YieldManager)
		{
			GameStateRef->YieldManager->CollectGlobalYieldIncome(PlayerId);
		}

		if (GameStateRef->TechManager)
		{
			GameStateRef->TechManager->ProcessResearchAtStartOfTurn(PlayerId);
		}
	}

	EnterPlayerActions();

	if (GameStateRef)
	{
		GameStateRef->EndStateChangeBatch(EConquestStateVisualDirtyFlags::All);
	}
}

void UConquestTurnManager::EnterPlayerActions()
{
	SetPhase(EConquestTurnPhase::PlayerActions);
}

void UConquestTurnManager::EndTurnProcessing()
{
	if (GameStateRef)
	{
		GameStateRef->BeginStateChangeBatch();
	}

	SetPhase(EConquestTurnPhase::EndTurnProcessing);

	++CurrentTurn;
	OnTurnChanged.Broadcast(CurrentTurn);

	StartTurnProcessing();

	if (GameStateRef)
	{
		GameStateRef->EndStateChangeBatch(EConquestStateVisualDirtyFlags::All);
	}
}
