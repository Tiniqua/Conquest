#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexTileTypes.h"

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