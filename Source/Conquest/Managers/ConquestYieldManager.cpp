#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexTileTypes.h"

namespace
{
	const FName DefaultCityCentreBuildingId(TEXT("CityCentre"));

	FHexYield MakeDefaultCityCentreYield()
	{
		FHexYield Yield;
		Yield.Food = 2;
		Yield.Production = 2;
		Yield.Science = 2;
		return Yield;
	}
}

void UConquestYieldManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

FHexYield UConquestYieldManager::CalculateTileYield(const FHexTileData& Tile) const
{
	return Tile.FinalYield;
}

FHexYield UConquestYieldManager::CalculateCityBuildingYields(const FCityState& City) const
{
	FHexYield Result;

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return Result;
	}

	for (const FName BuildingId : City.ConstructedBuildingIds)
	{
		const FConquestBuildingRow* BuildingRow = GameStateRef->ContentManager->FindBuilding(BuildingId);

		if (!BuildingRow)
		{
			if (BuildingId == DefaultCityCentreBuildingId)
			{
				Result += MakeDefaultCityCentreYield();
			}

			continue;
		}

		Result += BuildingRow->FlatCityYieldBonus;
	}

	return Result;
}

FHexYield UConquestYieldManager::CalculateCityTotalYields(const FCityState& City) const
{
	FHexYield Result;

	if (!GameStateRef)
	{
		return Result;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (!GridModel)
	{
		return Result;
	}

	for (const FIntPoint& Coord : City.WorkedTiles)
	{
		const FHexTileData* Tile = GridModel->GetTile(Coord);
		if (!Tile)
		{
			continue;
		}

		Result += CalculateTileYield(*Tile);
	}

	Result += CalculateCityBuildingYields(City);

	return Result;
}

FHexYield UConquestYieldManager::CalculateEmpireYieldPerTurn(int32 PlayerId) const
{
	FHexYield Result;

	if (!GameStateRef || !GameStateRef->CityManager)
	{
		return Result;
	}

	for (const FCityState& City : GameStateRef->CityManager->Cities)
	{
		if (City.OwnerPlayerId == PlayerId)
		{
			Result += City.CachedYieldPerTurn;
		}
	}

	return Result;
}

FHexYield UConquestYieldManager::RecalculateEmpireYieldPerTurn(int32 PlayerId) const
{
	FHexYield Result = CalculateEmpireYieldPerTurn(PlayerId);

	if (!GameStateRef)
	{
		return Result;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetHumanPlayerMutable();
	if (Player.PlayerId == PlayerId)
	{
		Player.CachedYieldPerTurn = Result;
	}

	return Result;
}

void UConquestYieldManager::CollectGlobalYieldIncome(int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetHumanPlayerMutable();
	if (Player.PlayerId != PlayerId)
	{
		return;
	}

	const FHexYield Income = RecalculateEmpireYieldPerTurn(PlayerId);

	Player.StoredYields.Gold += Income.Gold;
	Player.StoredYields.Culture += Income.Culture;
	Player.StoredYields.Faith += Income.Faith;
}
