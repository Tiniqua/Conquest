
#include "ConquestGameMode.h"
#include "ConquestGameState.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"

AConquestGameMode::AConquestGameMode()
{
	GameStateClass = AConquestGameState::StaticClass();
}

void AConquestGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AConquestGameMode::StartSinglePlayerGame()
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->TurnManager)
	{
		return;
	}

	ConquestGS->TurnManager->BeginGameSetup();
	ConquestGS->TurnManager->EnterAwaitingFirstCity();
}

void AConquestGameMode::EndCurrentTurn()
{
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->TurnManager)
	{
		return;
	}

	FText BlockReason;
	if (!CanEndCurrentTurn(BlockReason))
	{
		return;
	}

	ConquestGS->TurnManager->RequestEndTurn();
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
	AConquestGameState* ConquestGS = GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->CityManager || !ConquestGS->TurnManager)
	{
		return false;
	}

	const bool bFounded = ConquestGS->CityManager->FoundCity(0, TileCoord, CityName);

	if (bFounded)
	{
		ConquestGS->TurnManager->BeginFirstTurn();
	}

	return bFounded;
}
