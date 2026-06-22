#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexResourceSetData.h"
#include "Conquest/World/Generation/HexResourceTypes.h"
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

	float GetTileCitizenYieldMultiplier(int32 Citizens)
	{
		switch (FMath::Clamp(Citizens, 1, 3))
		{
		case 1:
			return 1.0f;
		case 2:
			return 1.5f;
		default:
			return 2.0f;
		}
	}

	FHexYield ScaleYield(const FHexYield& Yield, float Multiplier)
	{
		FHexYield Result;
		Result.Food = FMath::RoundToInt(static_cast<float>(Yield.Food) * Multiplier);
		Result.Production = FMath::RoundToInt(static_cast<float>(Yield.Production) * Multiplier);
		Result.Gold = FMath::RoundToInt(static_cast<float>(Yield.Gold) * Multiplier);
		Result.Science = FMath::RoundToInt(static_cast<float>(Yield.Science) * Multiplier);
		Result.Culture = FMath::RoundToInt(static_cast<float>(Yield.Culture) * Multiplier);
		Result.Faith = FMath::RoundToInt(static_cast<float>(Yield.Faith) * Multiplier);
		return Result;
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

	if (City.WorkedTileAssignments.Num() > 0)
	{
		for (const FCityWorkedTileAssignment& Assignment : City.WorkedTileAssignments)
		{
			const FHexTileData* Tile = GridModel->GetTile(Assignment.Coord);
			if (!Tile)
			{
				continue;
			}

			Result += ScaleYield(
				CalculateTileYield(*Tile),
				GetTileCitizenYieldMultiplier(Assignment.Citizens)
			);
		}
	}
	else
	{
		for (const FIntPoint& Coord : City.WorkedTiles)
		{
			const FHexTileData* Tile = GridModel->GetTile(Coord);
			if (!Tile)
			{
				continue;
			}

			Result += CalculateTileYield(*Tile);
		}
	}

	Result += CalculateCityBuildingYields(City);

	return ApplyUnhappyYieldPenalty(Result, City.OwnerPlayerId);
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

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	if (Player.PlayerId == PlayerId)
	{
		for (const FConquestUnitState& Unit : Player.Units)
		{
			Result.Gold -= FMath::Max(0, Unit.CachedGoldMaintenancePerTurn);
		}
	}

	return Result;
}

FHexYield UConquestYieldManager::RecalculateEmpireYieldPerTurn(int32 PlayerId) const
{
	RecalculateEmpireHappiness(PlayerId);

	FHexYield Result = CalculateEmpireYieldPerTurn(PlayerId);

	if (!GameStateRef)
	{
		return Result;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	if (Player.PlayerId == PlayerId)
	{
		Player.CachedYieldPerTurn = Result;
	}

	return Result;
}

int32 UConquestYieldManager::CalculateEmpireHappiness(int32 PlayerId) const
{
	if (!GameStateRef || !GameStateRef->CityManager)
	{
		return 0;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	TSet<FName> WorkedLuxuryResourceIds;

	int32 CityCount = 0;
	int32 Population = 0;
	int32 BuildingHappiness = 0;
	int32 LuxuryHappiness = 0;

	for (const FCityState& City : GameStateRef->CityManager->Cities)
	{
		if (City.OwnerPlayerId != PlayerId)
		{
			continue;
		}

		++CityCount;
		Population += FMath::Max(0, City.Population - 1);

		for (const FName BuildingId : City.ConstructedBuildingIds)
		{
			const FConquestBuildingRow* BuildingRow = GameStateRef->ContentManager
				? GameStateRef->ContentManager->FindBuilding(BuildingId)
				: nullptr;

			if (BuildingRow)
			{
				BuildingHappiness += BuildingRow->HappinessBonus;
			}
		}

		if (!GridModel)
		{
			continue;
		}

		for (const FIntPoint& Coord : City.WorkedTiles)
		{
			const FHexTileData* Tile = GridModel->GetTile(Coord);
			if (Tile && !Tile->Resource.ResourceId.IsNone())
			{
				WorkedLuxuryResourceIds.Add(Tile->Resource.ResourceId);
			}
		}
	}

	for (const FName ResourceId : WorkedLuxuryResourceIds)
	{
		const FHexResourceDefinition* ResourceDefinition = GridModel
			? GridModel->FindResourceDefinition(ResourceId)
			: nullptr;

		if (ResourceDefinition && ResourceDefinition->Category == EHexResourceCategory::Luxury)
		{
			LuxuryHappiness += ResourceDefinition->Happiness;
		}
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	return Player.BaseHappiness + LuxuryHappiness + BuildingHappiness - (CityCount * 2) - Population;
}

int32 UConquestYieldManager::RecalculateEmpireHappiness(int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return 0;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	if (Player.PlayerId != PlayerId)
	{
		return 0;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	TSet<FName> WorkedLuxuryResourceIds;

	int32 CityCount = 0;
	int32 Population = 0;
	int32 BuildingHappiness = 0;
	int32 LuxuryHappiness = 0;

	if (GameStateRef->CityManager)
	{
		for (const FCityState& City : GameStateRef->CityManager->Cities)
		{
			if (City.OwnerPlayerId != PlayerId)
			{
				continue;
			}

			++CityCount;
			Population += FMath::Max(0, City.Population - 1);

			for (const FName BuildingId : City.ConstructedBuildingIds)
			{
				const FConquestBuildingRow* BuildingRow = GameStateRef->ContentManager
					? GameStateRef->ContentManager->FindBuilding(BuildingId)
					: nullptr;

				if (BuildingRow)
				{
					BuildingHappiness += BuildingRow->HappinessBonus;
				}
			}

			if (!GridModel)
			{
				continue;
			}

			for (const FIntPoint& Coord : City.WorkedTiles)
			{
				const FHexTileData* Tile = GridModel->GetTile(Coord);
				if (Tile && !Tile->Resource.ResourceId.IsNone())
				{
					WorkedLuxuryResourceIds.Add(Tile->Resource.ResourceId);
				}
			}
		}
	}

	for (const FName ResourceId : WorkedLuxuryResourceIds)
	{
		const FHexResourceDefinition* ResourceDefinition = GridModel
			? GridModel->FindResourceDefinition(ResourceId)
			: nullptr;

		if (ResourceDefinition && ResourceDefinition->Category == EHexResourceCategory::Luxury)
		{
			LuxuryHappiness += ResourceDefinition->Happiness;
		}
	}

	Player.CachedCityHappinessCost = CityCount * 2;
	Player.CachedPopulationHappinessCost = Population;
	Player.CachedBuildingHappiness = BuildingHappiness;
	Player.CachedLuxuryHappiness = LuxuryHappiness;
	Player.CachedHappiness =
		Player.BaseHappiness +
		Player.CachedLuxuryHappiness +
		Player.CachedBuildingHappiness -
		Player.CachedCityHappinessCost -
		Player.CachedPopulationHappinessCost;

	return Player.CachedHappiness;
}

bool UConquestYieldManager::IsEmpireUnhappy(int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return false;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	return Player.PlayerId == PlayerId && Player.CachedHappiness < 0;
}

FHexYield UConquestYieldManager::ApplyUnhappyYieldPenalty(const FHexYield& Yield, int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return Yield;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	if (Player.PlayerId != PlayerId || Player.CachedHappiness >= 0)
	{
		return Yield;
	}

	FHexYield Result = Yield;
	Result.Food = FMath::FloorToInt(static_cast<float>(Result.Food) * 0.75f);
	Result.Production = FMath::FloorToInt(static_cast<float>(Result.Production) * 0.75f);
	Result.Gold = FMath::FloorToInt(static_cast<float>(Result.Gold) * 0.75f);
	Result.Science = FMath::FloorToInt(static_cast<float>(Result.Science) * 0.75f);
	Result.Culture = FMath::FloorToInt(static_cast<float>(Result.Culture) * 0.75f);
	Result.Faith = FMath::FloorToInt(static_cast<float>(Result.Faith) * 0.75f);
	return Result;
}

void UConquestYieldManager::CollectGlobalYieldIncome(int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	if (Player.PlayerId != PlayerId)
	{
		return;
	}

	const FHexYield Income = RecalculateEmpireYieldPerTurn(PlayerId);

	Player.StoredYields.Gold += Income.Gold;
	Player.StoredYields.Culture += Income.Culture;
	Player.StoredYields.Faith += Income.Faith;
}
