#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestModifierManager.h"
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
		Result.Food = FMath::CeilToInt(static_cast<float>(Yield.Food) * Multiplier);
		Result.Production = FMath::CeilToInt(static_cast<float>(Yield.Production) * Multiplier);
		Result.Gold = FMath::CeilToInt(static_cast<float>(Yield.Gold) * Multiplier);
		Result.Science = FMath::CeilToInt(static_cast<float>(Yield.Science) * Multiplier);
		Result.Culture = FMath::CeilToInt(static_cast<float>(Yield.Culture) * Multiplier);
		return Result;
	}

	void AddWorkedLuxuryResourceIds(
		const FCityState& City,
		const FHexGridModel& GridModel,
		TSet<FName>& OutWorkedLuxuryResourceIds
	)
	{
		auto AddTileResource = [&GridModel, &OutWorkedLuxuryResourceIds](const FIntPoint& Coord)
		{
			const FHexTileData* Tile = GridModel.GetTile(Coord);
			if (Tile && !Tile->Resource.ResourceId.IsNone())
			{
				OutWorkedLuxuryResourceIds.Add(Tile->Resource.ResourceId);
			}
		};

		if (City.WorkedTileAssignments.Num() > 0)
		{
			for (const FCityWorkedTileAssignment& Assignment : City.WorkedTileAssignments)
			{
				AddTileResource(Assignment.Coord);
			}
			return;
		}

		for (const FIntPoint& Coord : City.WorkedTiles)
		{
			AddTileResource(Coord);
		}
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
	}

	return Result;
}

void UConquestYieldManager::ApplyCityBuildingYieldsToCenterTile(const FCityState& City) const
{
	if (!GameStateRef)
	{
		return;
	}

	FHexGridModel* GridModel = GameStateRef->GetHexGridModelMutable();
	if (!GridModel)
	{
		return;
	}

	FHexTileData* CenterTile = GridModel->GetTileMutable(City.CenterTile);
	if (!CenterTile)
	{
		return;
	}

	CenterTile->FinalYield = CenterTile->Gameplay.BaseYields + CalculateCityBuildingYields(City);
	CenterTile->Gameplay.CachedFinalYields = CenterTile->FinalYield;

	if (GameStateRef->ActiveGridActor)
	{
		GameStateRef->ActiveGridActor->UpdateTileYieldOverlayForTile(City.CenterTile);
	}
}

FHexYield UConquestYieldManager::CalculateCityTotalYieldsBeforeUnhappyPenalty(const FCityState& City) const
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

			FHexYield TileYield = CalculateTileYield(*Tile);
			if (GameStateRef->ModifierManager)
			{
				FConquestModifierContext TileContext;
				TileContext.Scope = EConquestModifierTargetScope::Tile;
				TileContext.PlayerId = City.OwnerPlayerId;
				TileContext.CityId = City.CityId;
				TileContext.TileCoord = Assignment.Coord;
				TileContext.bHasTileCoord = true;
				TileContext.bIgnoreHappinessPenalty = true;
				TileYield = GameStateRef->ModifierManager->ApplyYieldModifiers(TileContext, TileYield);
			}

			Result += ScaleYield(TileYield, GetTileCitizenYieldMultiplier(Assignment.Citizens));
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

			FHexYield TileYield = CalculateTileYield(*Tile);
			if (GameStateRef->ModifierManager)
			{
				FConquestModifierContext TileContext;
				TileContext.Scope = EConquestModifierTargetScope::Tile;
				TileContext.PlayerId = City.OwnerPlayerId;
				TileContext.CityId = City.CityId;
				TileContext.TileCoord = Coord;
				TileContext.bHasTileCoord = true;
				TileContext.bIgnoreHappinessPenalty = true;
				TileYield = GameStateRef->ModifierManager->ApplyYieldModifiers(TileContext, TileYield);
			}

			Result += TileYield;
		}
	}

	if (GameStateRef->ModifierManager)
	{
		FConquestModifierContext CityContext;
		CityContext.Scope = EConquestModifierTargetScope::City;
		CityContext.PlayerId = City.OwnerPlayerId;
		CityContext.CityId = City.CityId;
		CityContext.bIgnoreHappinessPenalty = true;
		Result = GameStateRef->ModifierManager->ApplyYieldModifiers(CityContext, Result);
	}

	return Result;
}

FHexYield UConquestYieldManager::CalculateCityTotalYields(const FCityState& City) const
{
	FHexYield Result = CalculateCityTotalYieldsBeforeUnhappyPenalty(City);
	if (GameStateRef && GameStateRef->ModifierManager)
	{
		FConquestModifierContext CityContext;
		CityContext.Scope = EConquestModifierTargetScope::City;
		CityContext.PlayerId = City.OwnerPlayerId;
		CityContext.CityId = City.CityId;
		CityContext.bOnlyHappinessPenalty = true;
		return GameStateRef->ModifierManager->ApplyYieldModifiers(CityContext, Result);
	}

	return ApplyUnhappyYieldPenalty(Result, City.OwnerPlayerId);
}

FHexYield UConquestYieldManager::CalculateEmpireYieldPerTurnBeforeUnhappyPenalty(int32 PlayerId) const
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
			Result += CalculateCityTotalYieldsBeforeUnhappyPenalty(City);
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
		Population += FMath::Max(0, City.Population);

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

		AddWorkedLuxuryResourceIds(City, *GridModel, WorkedLuxuryResourceIds);
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
	const int32 BaseValue =
		Player.BaseHappiness -
		(CityCount * ConquestHappiness::GetCityCost()) -
		(Population * ConquestHappiness::GetPopulationCost());

	if (GameStateRef->ModifierManager)
	{
		FConquestModifierContext Context;
		Context.Scope = EConquestModifierTargetScope::Empire;
		Context.PlayerId = PlayerId;
		return GameStateRef->ModifierManager->CalculateModifiedIntValue(
			Context,
			EConquestModifierAttribute::Happiness,
			static_cast<float>(BaseValue)
		);
	}

	return BaseValue + LuxuryHappiness + BuildingHappiness;
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
			Population += FMath::Max(0, City.Population);

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

			AddWorkedLuxuryResourceIds(City, *GridModel, WorkedLuxuryResourceIds);
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

	Player.CachedCityHappinessCost = CityCount * ConquestHappiness::GetCityCost();
	Player.CachedPopulationHappinessCost = Population * ConquestHappiness::GetPopulationCost();
	Player.CachedBuildingHappiness = BuildingHappiness;
	Player.CachedLuxuryHappiness = LuxuryHappiness;
	const int32 BaseValue =
		Player.BaseHappiness -
		Player.CachedCityHappinessCost -
		Player.CachedPopulationHappinessCost;

	if (GameStateRef->ModifierManager)
	{
		FConquestModifierContext Context;
		Context.Scope = EConquestModifierTargetScope::Empire;
		Context.PlayerId = PlayerId;
		Player.CachedHappiness = GameStateRef->ModifierManager->CalculateModifiedIntValue(
			Context,
			EConquestModifierAttribute::Happiness,
			static_cast<float>(BaseValue)
		);
	}
	else
	{
		Player.CachedHappiness =
			BaseValue +
			Player.CachedLuxuryHappiness +
			Player.CachedBuildingHappiness;
	}

	return Player.CachedHappiness;
}

bool UConquestYieldManager::IsEmpireUnhappy(int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return false;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	return Player.PlayerId == PlayerId && ConquestHappiness::IsUnhappy(Player.CachedHappiness);
}

FHexYield UConquestYieldManager::ApplyUnhappyYieldPenalty(const FHexYield& Yield, int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return Yield;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	const float PenaltyMultiplier = ConquestHappiness::GetPenaltyMultiplier(Player.CachedHappiness);
	if (Player.PlayerId != PlayerId || PenaltyMultiplier >= 1.0f)
	{
		return Yield;
	}

	FHexYield Result = Yield;
	Result.Food = FMath::CeilToInt(static_cast<float>(Result.Food) * PenaltyMultiplier);
	Result.Production = FMath::CeilToInt(static_cast<float>(Result.Production) * PenaltyMultiplier);
	Result.Gold = FMath::CeilToInt(static_cast<float>(Result.Gold) * PenaltyMultiplier);
	Result.Science = FMath::CeilToInt(static_cast<float>(Result.Science) * PenaltyMultiplier);
	Result.Culture = FMath::CeilToInt(static_cast<float>(Result.Culture) * PenaltyMultiplier);
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
}
