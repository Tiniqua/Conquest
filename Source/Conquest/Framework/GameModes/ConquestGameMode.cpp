
#include "ConquestGameMode.h"
#include "ConquestGameState.h"
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

	ConquestGS->TurnManager->RequestEndTurn();
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