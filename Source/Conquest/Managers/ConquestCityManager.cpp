#include "Conquest/Managers/ConquestCityManager.h"

#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Tech/ConquestTechTypes.h"
#include "Conquest/Units/ConquestUnitActor.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexImprovementTypes.h"
#include "Conquest/World/Generation/HexResourceSetData.h"
#include "Conquest/World/Generation/HexResourceTypes.h"
#include "Conquest/World/Generation/HexTileTypes.h"

namespace
{
	const FName ConquestProductionProjectFoodFocus(TEXT("FoodFocus"));
	const FName ConquestProductionProjectGoldFocus(TEXT("GoldFocus"));
	const FName ConquestProductionProjectCultureFocus(TEXT("CultureFocus"));
	const FName ConquestProductionProjectScienceFocus(TEXT("ScienceFocus"));
	const FName ConquestUnhappyAttackModifierId(TEXT("Happiness_Unhappy_Attack"));
	const FName ConquestUnhappyDefenseModifierId(TEXT("Happiness_Unhappy_Defense"));
	const FName ConquestTileDefenseModifierId(TEXT("Tile_Defense_Modifier"));
	constexpr int32 ConquestDefaultOwnedTileHealth = 100;
	constexpr int32 ConquestDefaultOwnedTileHealRate = 5;

	const TArray<FName>& GetConquestProductionProjectIds()
	{
		static const TArray<FName> ProjectIds =
		{
			ConquestProductionProjectFoodFocus,
			ConquestProductionProjectGoldFocus,
			ConquestProductionProjectCultureFocus,
			ConquestProductionProjectScienceFocus
		};

		return ProjectIds;
	}
}

void UConquestCityManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

bool UConquestCityManager::FoundCity(int32 PlayerId, const FIntPoint& TileCoord, FName CityName)
{
	if (!GameStateRef || !IsValidFoundCityTile(TileCoord))
	{
		return false;
	}

	FCityState NewCity;
	NewCity.CityId = NextCityId++;
	NewCity.OwnerPlayerId = PlayerId;
	NewCity.CityName = ResolveCityName(PlayerId, CityName);
	NewCity.CenterTile = TileCoord;
	NewCity.Population = 1;
	NewCity.MaxHealth = 100;
	NewCity.CurrentHealth = NewCity.MaxHealth;
	RefreshCityCombatStats(NewCity);
	NewCity.FoodStored = 0.0f;
	NewCity.CachedFoodRequiredForNextPopulation = GetFoodRequiredForNextPopulation(NewCity);
	NewCity.bNeedsProductionChoice = true;

	GrantStartingBuildings(NewCity);
	ClaimTileForCity(NewCity, TileCoord);
	FCityWorkedTileAssignment CenterAssignment;
	CenterAssignment.Coord = TileCoord;
	CenterAssignment.Citizens = 1;
	NewCity.WorkedTileAssignments.Add(CenterAssignment);
	AutoAssignWorkedTiles(NewCity);
	RecalculateCityYields(NewCity);

	Cities.Add(NewCity);

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	Player.CityIds.Add(NewCity.CityId);

	if (GameStateRef->ActiveGridActor)
	{
		UStaticMesh* CityMesh = nullptr;
		UMaterialInterface* CityMaterial = nullptr;
		UMaterialInterface* CivilisationThemeMaterial = nullptr;
		FLinearColor CivilisationThemeColor = FLinearColor::White;
		bool bOverrideCityScale = false;
		FVector CityScale = FVector::OneVector;
		if (const UConquestCivilisationData* Civilisation = GameStateRef->GetCivilisationForPlayer(PlayerId))
		{
			CityMesh = Civilisation->CityMesh;
			CityMaterial = Civilisation->CityMeshMaterialOverride;
			CivilisationThemeMaterial = Civilisation->CityLabelMaterial
				? Civilisation->CityLabelMaterial
				: Civilisation->BorderMaterial;
			CivilisationThemeColor = Civilisation->ThemeColor;
			bOverrideCityScale = Civilisation->bOverrideCityMeshScale;
			CityScale = Civilisation->CityMeshScaleOverride;
		}

		GameStateRef->ActiveGridActor->AddCityPlaceholder(
			NewCity.CityId,
			NewCity.CenterTile,
			CityMesh,
			CityMaterial,
			bOverrideCityScale,
			CityScale
		);

		GameStateRef->ActiveGridActor->AddOrUpdateCityWorldLabel(
			NewCity.CityId,
			NewCity.CenterTile,
			NewCity.CityName,
			NewCity.Population,
			NewCity.CurrentHealth,
			NewCity.MaxHealth,
			NewCity.CachedStrength,
			CivilisationThemeMaterial,
			CivilisationThemeColor
		);
	}

	OnCityFounded.Broadcast(NewCity.CityId, TileCoord);
	OnCityChanged.Broadcast(NewCity.CityId);
	RecalculateEmpireYields(PlayerId);
	UpdateOwnedTileVisuals(PlayerId);
	GameStateRef->BroadcastStateChanged();

	return true;
}

void UConquestCityManager::ResetCities()
{
	Cities.Reset();
	NextCityId = 1;
}

int32 UConquestCityManager::FindCityAtTile(const FIntPoint& Coord) const
{
	for (const FCityState& City : Cities)
	{
		if (City.CenterTile == Coord)
		{
			return City.CityId;
		}
	}

	return INDEX_NONE;
}

void UConquestCityManager::GrantStartingBuildings(FCityState& City)
{
	if (GameStateRef && GameStateRef->ContentManager)
	{
		TArray<FName> StartingBuildingIds;
		GameStateRef->ContentManager->GetStartingBuildingIdsForPlayer(
			City.OwnerPlayerId,
			StartingBuildingIds
		);

		for (const FName BuildingId : StartingBuildingIds)
		{
			if (!BuildingId.IsNone())
			{
				City.ConstructedBuildingIds.AddUnique(BuildingId);
			}
		}
	}

	if (City.ConstructedBuildingIds.Num() <= 0 && !DefaultCityCentreBuildingId.IsNone())
	{
		City.ConstructedBuildingIds.AddUnique(DefaultCityCentreBuildingId);
	}
}

bool UConquestCityManager::IsValidFoundCityTile(const FIntPoint& TileCoord) const
{
	if (!GameStateRef)
	{
		return false;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (!GridModel)
	{
		return false;
	}

	const FHexTileData* Tile = GridModel->GetTile(TileCoord);
	if (!Tile)
	{
		return false;
	}

	if (Tile->Gameplay.OwnerPlayerId != INDEX_NONE)
	{
		return false;
	}

	for (const FCityState& City : Cities)
	{
		if (City.CenterTile == TileCoord)
		{
			return false;
		}
	}

	// Reject water and mountains for now.
	if (
		GridModel->IsWaterTileType(Tile->TileType) ||
		Tile->TileType == EHexTileType::Mountain
	)
	{
		return false;
	}

	for (const FCityState& City : Cities)
	{
		if (GridModel->GetHexDistance(City.CenterTile, TileCoord) < 3)
		{
			return false;
		}
	}

	return true;
}

bool UConquestCityManager::ClaimTileForCity(FCityState& City, const FIntPoint& Coord)
{
	if (!GameStateRef)
	{
		return false;
	}

	FHexGridModel* GridModel = GameStateRef->GetHexGridModelMutable();
	if (!GridModel)
	{
		return false;
	}

	FHexTileData* Tile = GridModel->GetTileMutable(Coord);
	if (!Tile)
	{
		return false;
	}

	if (Tile->Gameplay.OwnerPlayerId != INDEX_NONE)
	{
		return false;
	}

	Tile->Gameplay.OwnerPlayerId = City.OwnerPlayerId;
	Tile->Gameplay.OwningCityId = City.CityId;

	City.OwnedTiles.AddUnique(Coord);
	EnsureOwnedTileCombatState(City, Coord);
	return true;
}

bool UConquestCityManager::IsValidExpansionTileForCity(const FCityState& City, const FIntPoint& Coord) const
{
	if (!GameStateRef)
	{
		return false;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (!GridModel)
	{
		return false;
	}

	const FHexTileData* Tile = GridModel->GetTile(Coord);
	if (!Tile || Tile->Gameplay.OwnerPlayerId != INDEX_NONE)
	{
		return false;
	}

	if (IsEnemyUnitOnTile(City.OwnerPlayerId, Coord))
	{
		return false;
	}

	if (GridModel->GetHexDistance(City.CenterTile, Coord) > MaxCityExpansionRange)
	{
		return false;
	}

	for (const FIntPoint& OwnedCoord : City.OwnedTiles)
	{
		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NeighborQ = INDEX_NONE;
			int32 NeighborR = INDEX_NONE;
			if (
				GridModel->GetNeighbourCoord(OwnedCoord.X, OwnedCoord.Y, Direction, NeighborQ, NeighborR) &&
				FIntPoint(NeighborQ, NeighborR) == Coord
			)
			{
				return true;
			}
		}
	}

	return false;
}

bool UConquestCityManager::IsValidPopulationAssignmentTile(const FCityState& City, const FIntPoint& Coord) const
{
	if (!GameStateRef)
	{
		return false;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (!GridModel)
	{
		return false;
	}

	const FHexTileData* Tile = GridModel->GetTile(Coord);
	if (!Tile)
	{
		return false;
	}

	if (City.OwnedTiles.Contains(Coord))
	{
		return
			Coord != City.CenterTile &&
			GetAssignedCitizensForTile(City, Coord) < 3 &&
			!IsEnemyUnitOnTile(City.OwnerPlayerId, Coord);
	}

	return IsValidExpansionTileForCity(City, Coord);
}

FCityOwnedTileCombatState& UConquestCityManager::EnsureOwnedTileCombatState(FCityState& City, const FIntPoint& Coord)
{
	if (FCityOwnedTileCombatState* ExistingState = GetOwnedTileCombatStateMutable(City, Coord))
	{
		RefreshOwnedTileCombatState(City, *ExistingState);
		return *ExistingState;
	}

	FCityOwnedTileCombatState& NewState = City.OwnedTileCombatStates.AddDefaulted_GetRef();
	NewState.Coord = Coord;
	NewState.CurrentHealth = ConquestDefaultOwnedTileHealth;
	NewState.MaxHealth = ConquestDefaultOwnedTileHealth;
	NewState.CombatStrength = 0;
	NewState.HealRatePerTurn = ConquestDefaultOwnedTileHealRate;
	NewState.DefenderModifier = 1.0f;
	RefreshOwnedTileCombatState(City, NewState);
	return NewState;
}

void UConquestCityManager::RefreshOwnedTileCombatState(
	FCityState& City,
	FCityOwnedTileCombatState& TileCombatState
)
{
	TileCombatState.MaxHealth = FMath::Max(1, TileCombatState.MaxHealth <= 0
		? ConquestDefaultOwnedTileHealth
		: TileCombatState.MaxHealth);
	TileCombatState.CurrentHealth = FMath::Clamp(TileCombatState.CurrentHealth, 0, TileCombatState.MaxHealth);
	TileCombatState.CombatStrength = 0;
	TileCombatState.HealRatePerTurn = ConquestDefaultOwnedTileHealRate;
	TileCombatState.DefenderModifier = 1.0f;

	const FHexGridModel* GridModel = GameStateRef ? GameStateRef->GetHexGridModel() : nullptr;
	const FHexTileData* Tile = GridModel ? GridModel->GetTile(TileCombatState.Coord) : nullptr;
	const FHexImprovementDefinition* Improvement = nullptr;
	if (GridModel && Tile && !Tile->ImprovementId.IsNone())
	{
		Improvement = GridModel->FindImprovementDefinition(Tile->ImprovementId);
	}

	if (Improvement)
	{
		TileCombatState.CombatStrength += FMath::Max(0, Improvement->TileCombatStrengthBonus);
		TileCombatState.HealRatePerTurn += FMath::Max(0, Improvement->TileHealRateBonus);
		TileCombatState.DefenderModifier += FMath::Max(0.0f, Improvement->UnitDefenderModifierBonus);
	}

	if (FHexGridModel* MutableGridModel = GameStateRef ? GameStateRef->GetHexGridModelMutable() : nullptr)
	{
		if (FHexTileData* MutableTile = MutableGridModel->GetTileMutable(TileCombatState.Coord))
		{
			MutableTile->Gameplay.CurrentHealth = TileCombatState.CurrentHealth;
			MutableTile->Gameplay.MaxHealth = TileCombatState.MaxHealth;
			MutableTile->Gameplay.CombatStrength = TileCombatState.CombatStrength;
			MutableTile->Gameplay.HealRatePerTurn = TileCombatState.HealRatePerTurn;
			MutableTile->Gameplay.DefenderModifier = TileCombatState.DefenderModifier;
		}
	}
}

FCityOwnedTileCombatState* UConquestCityManager::GetOwnedTileCombatStateMutable(
	FCityState& City,
	const FIntPoint& Coord
)
{
	return City.OwnedTileCombatStates.FindByPredicate([Coord](const FCityOwnedTileCombatState& TileCombatState)
	{
		return TileCombatState.Coord == Coord;
	});
}

const FCityOwnedTileCombatState* UConquestCityManager::GetOwnedTileCombatStateForCity(
	const FCityState& City,
	const FIntPoint& Coord
) const
{
	return City.OwnedTileCombatStates.FindByPredicate([Coord](const FCityOwnedTileCombatState& TileCombatState)
	{
		return TileCombatState.Coord == Coord;
	});
}

void UConquestCityManager::AutoAssignWorkedTiles(FCityState& City)
{
	if (City.WorkedTileAssignments.Num() <= 0)
	{
		FCityWorkedTileAssignment CenterAssignment;
		CenterAssignment.Coord = City.CenterTile;
		CenterAssignment.Citizens = 1;
		City.WorkedTileAssignments.Add(CenterAssignment);
	}

	SyncWorkedTilesFromAssignments(City);
}

void UConquestCityManager::SyncWorkedTilesFromAssignments(FCityState& City)
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

	for (const FIntPoint& Coord : City.WorkedTiles)
	{
		if (FHexTileData* OldTile = GridModel->GetTileMutable(Coord))
		{
			OldTile->Gameplay.bIsWorked = false;
			OldTile->Gameplay.WorkedByCityId = INDEX_NONE;
		}
	}

	City.WorkedTiles.Reset();

	for (int32 AssignmentIndex = City.WorkedTileAssignments.Num() - 1; AssignmentIndex >= 0; --AssignmentIndex)
	{
		FCityWorkedTileAssignment& Assignment = City.WorkedTileAssignments[AssignmentIndex];
		Assignment.Citizens = FMath::Clamp(Assignment.Citizens, 1, 3);

		if (!City.OwnedTiles.Contains(Assignment.Coord))
		{
			City.WorkedTileAssignments.RemoveAt(AssignmentIndex);
			continue;
		}

		City.WorkedTiles.AddUnique(Assignment.Coord);
	}

	for (int32 TileStateIndex = City.OwnedTileCombatStates.Num() - 1; TileStateIndex >= 0; --TileStateIndex)
	{
		FCityOwnedTileCombatState& TileCombatState = City.OwnedTileCombatStates[TileStateIndex];
		if (!City.OwnedTiles.Contains(TileCombatState.Coord))
		{
			City.OwnedTileCombatStates.RemoveAt(TileStateIndex);
			continue;
		}

		RefreshOwnedTileCombatState(City, TileCombatState);
	}

	for (const FIntPoint& OwnedCoord : City.OwnedTiles)
	{
		EnsureOwnedTileCombatState(City, OwnedCoord);
	}

	if (!City.WorkedTiles.Contains(City.CenterTile) && City.OwnedTiles.Contains(City.CenterTile))
	{
		FCityWorkedTileAssignment CenterAssignment;
		CenterAssignment.Coord = City.CenterTile;
		CenterAssignment.Citizens = 1;
		City.WorkedTileAssignments.Add(CenterAssignment);
		City.WorkedTiles.AddUnique(City.CenterTile);
	}

	for (const FIntPoint& Coord : City.WorkedTiles)
	{
		if (FHexTileData* WorkedTile = GridModel->GetTileMutable(Coord))
		{
			WorkedTile->Gameplay.bIsWorked = true;
			WorkedTile->Gameplay.WorkedByCityId = City.CityId;
		}
	}
}

int32 UConquestCityManager::GetAssignedCitizensForTile(const FCityState& City, const FIntPoint& Coord) const
{
	for (const FCityWorkedTileAssignment& Assignment : City.WorkedTileAssignments)
	{
		if (Assignment.Coord == Coord)
		{
			return Assignment.Citizens;
		}
	}

	return 0;
}

bool UConquestCityManager::AssignCitizenToTile(FCityState& City, const FIntPoint& Coord)
{
	for (FCityWorkedTileAssignment& Assignment : City.WorkedTileAssignments)
	{
		if (Assignment.Coord == Coord)
		{
			if (Coord == City.CenterTile || Assignment.Citizens >= 3)
			{
				return false;
			}

			Assignment.Citizens += 1;
			SyncWorkedTilesFromAssignments(City);
			return true;
		}
	}

	if (!City.OwnedTiles.Contains(Coord))
	{
		return false;
	}

	FCityWorkedTileAssignment NewAssignment;
	NewAssignment.Coord = Coord;
	NewAssignment.Citizens = 1;
	City.WorkedTileAssignments.Add(NewAssignment);
	SyncWorkedTilesFromAssignments(City);
	return true;
}

bool UConquestCityManager::IsTileImprovementUnlockedForPlayer(int32 PlayerId, FName ImprovementId) const
{
	if (ImprovementId.IsNone() || !GameStateRef || !GameStateRef->ContentManager)
	{
		return false;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	TSet<FName> GatedImprovementIds;
	TSet<FName> UnlockedImprovementIds;

	TArray<const FConquestTechRow*> TechRows;
	GameStateRef->ContentManager->GetAllTechs(TechRows);
	for (const FConquestTechRow* TechRow : TechRows)
	{
		if (!TechRow)
		{
			continue;
		}

		for (const FName UnlockedImprovementId : TechRow->UnlockedTileImprovementIds)
		{
			if (UnlockedImprovementId.IsNone())
			{
				continue;
			}

			GatedImprovementIds.Add(UnlockedImprovementId);
			if (Player.HasResearched(TechRow->TechId))
			{
				UnlockedImprovementIds.Add(UnlockedImprovementId);
			}
		}
	}

	for (const FCityState& City : Cities)
	{
		if (City.OwnerPlayerId != PlayerId)
		{
			continue;
		}

		for (const FName BuildingId : City.ConstructedBuildingIds)
		{
			const FConquestBuildingRow* BuildingRow = GameStateRef->ContentManager->FindBuilding(BuildingId);
			if (!BuildingRow)
			{
				continue;
			}

			for (const FName UnlockedImprovementId : BuildingRow->UnlockedTileImprovementIds)
			{
				if (UnlockedImprovementId.IsNone())
				{
					continue;
				}

				GatedImprovementIds.Add(UnlockedImprovementId);
				UnlockedImprovementIds.Add(UnlockedImprovementId);
			}
		}
	}

	return !GatedImprovementIds.Contains(ImprovementId) || UnlockedImprovementIds.Contains(ImprovementId);
}

int32 UConquestCityManager::GetCityStrength(const FCityState& City) const
{
	return FMath::Max(1, 10 + FMath::Max(0, City.Population - 1) * 2);
}

void UConquestCityManager::RefreshCityCombatStats(FCityState& City)
{
	City.MaxHealth = FMath::Max(1, City.MaxHealth);
	City.CachedStrength = GetCityStrength(City);
	City.CurrentHealth = FMath::Clamp(City.CurrentHealth, 0, City.MaxHealth);
}

void UConquestCityManager::HealCityAtStartOfTurn(FCityState& City)
{
	RefreshCityCombatStats(City);
	if (City.CurrentHealth < City.MaxHealth)
	{
		City.CurrentHealth = FMath::Min(City.MaxHealth, City.CurrentHealth + 10);
	}
}

void UConquestCityManager::HealOwnedTilesAtStartOfTurn(FCityState& City)
{
	for (FCityOwnedTileCombatState& TileCombatState : City.OwnedTileCombatStates)
	{
		RefreshOwnedTileCombatState(City, TileCombatState);
		if (TileCombatState.CurrentHealth < TileCombatState.MaxHealth)
		{
			TileCombatState.CurrentHealth = FMath::Min(
				TileCombatState.MaxHealth,
				TileCombatState.CurrentHealth + FMath::Max(0, TileCombatState.HealRatePerTurn)
			);
			RefreshOwnedTileCombatState(City, TileCombatState);
		}

		UpdateOwnedTileHealthBar(City, TileCombatState);
	}
}

bool UConquestCityManager::DamageCity(int32 CityId, int32 DamageAmount)
{
	FCityState* City = GetCityMutable(CityId);
	if (!City || DamageAmount < 0)
	{
		return false;
	}

	RefreshCityCombatStats(*City);
	City->CurrentHealth = FMath::Clamp(City->CurrentHealth - DamageAmount, 0, City->MaxHealth);
	UpdateCityWorldLabel(*City);
	OnCityChanged.Broadcast(City->CityId);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}

	return true;
}

bool UConquestCityManager::DamageOwnedTile(const FIntPoint& Coord, int32 DamageAmount)
{
	if (!GameStateRef || DamageAmount < 0)
	{
		return false;
	}

	FCityState* OwningCity = nullptr;
	for (FCityState& City : Cities)
	{
		if (City.OwnedTiles.Contains(Coord))
		{
			OwningCity = &City;
			break;
		}
	}

	if (!OwningCity || OwningCity->CenterTile == Coord)
	{
		return false;
	}

	FCityOwnedTileCombatState& TileCombatState = EnsureOwnedTileCombatState(*OwningCity, Coord);
	TileCombatState.CurrentHealth = FMath::Clamp(
		TileCombatState.CurrentHealth - DamageAmount,
		0,
		FMath::Max(1, TileCombatState.MaxHealth)
	);
	RefreshOwnedTileCombatState(*OwningCity, TileCombatState);
	UpdateOwnedTileHealthBar(*OwningCity, TileCombatState);
	OnCityTileChanged.Broadcast(OwningCity->CityId, Coord);
	OnCityChanged.Broadcast(OwningCity->CityId);
	GameStateRef->BroadcastStateChanged();
	return true;
}

bool UConquestCityManager::DestroyOwnedTile(const FIntPoint& Coord, int32 AttackerPlayerId)
{
	if (!GameStateRef)
	{
		return false;
	}

	FCityState* OwningCity = nullptr;
	for (FCityState& City : Cities)
	{
		if (City.OwnedTiles.Contains(Coord))
		{
			OwningCity = &City;
			break;
		}
	}

	if (!OwningCity || OwningCity->CenterTile == Coord || OwningCity->OwnerPlayerId == AttackerPlayerId)
	{
		return false;
	}

	const int32 PreviousOwnerPlayerId = OwningCity->OwnerPlayerId;
	const int32 LostCitizens = FMath::Max(1, GetAssignedCitizensForTile(*OwningCity, Coord));

	OwningCity->OwnedTiles.Remove(Coord);
	OwningCity->WorkedTiles.Remove(Coord);
	OwningCity->WorkedTileAssignments.RemoveAll([Coord](const FCityWorkedTileAssignment& Assignment)
	{
		return Assignment.Coord == Coord;
	});
	OwningCity->OwnedTileCombatStates.RemoveAll([Coord](const FCityOwnedTileCombatState& TileCombatState)
	{
		return TileCombatState.Coord == Coord;
	});
	OwningCity->Population = FMath::Max(1, OwningCity->Population - LostCitizens);
	OwningCity->PendingBorderExpansions = FMath::Max(0, OwningCity->PendingBorderExpansions - LostCitizens);
	OwningCity->CachedFoodRequiredForNextPopulation = GetFoodRequiredForNextPopulation(*OwningCity);

	if (FHexGridModel* GridModel = GameStateRef->GetHexGridModelMutable())
	{
		if (FHexTileData* Tile = GridModel->GetTileMutable(Coord))
		{
			Tile->Gameplay.OwnerPlayerId = INDEX_NONE;
			Tile->Gameplay.OwningCityId = INDEX_NONE;
			Tile->Gameplay.bIsWorked = false;
			Tile->Gameplay.WorkedByCityId = INDEX_NONE;
			Tile->Gameplay.CurrentHealth = ConquestDefaultOwnedTileHealth;
			Tile->Gameplay.MaxHealth = ConquestDefaultOwnedTileHealth;
			Tile->Gameplay.CombatStrength = 0;
			Tile->Gameplay.HealRatePerTurn = ConquestDefaultOwnedTileHealRate;
			Tile->Gameplay.DefenderModifier = 1.0f;
		}
	}

	if (GameStateRef->ActiveGridActor)
	{
		GameStateRef->ActiveGridActor->SetTileImprovement(Coord.X, Coord.Y, NAME_None);
		GameStateRef->ActiveGridActor->RemoveTileHealthBar(Coord);
	}

	AutoAssignWorkedTiles(*OwningCity);
	RefreshCityCombatStats(*OwningCity);
	RecalculateCityYields(*OwningCity);
	RecalculateEmpireYields(PreviousOwnerPlayerId);
	RecalculateStrategicResourceEconomy(PreviousOwnerPlayerId);
	UpdateOwnedTileVisuals(PreviousOwnerPlayerId);
	UpdateCityWorldLabel(*OwningCity);
	OnCityTileChanged.Broadcast(OwningCity->CityId, Coord);
	OnCityChanged.Broadcast(OwningCity->CityId);
	GameStateRef->BroadcastStateChanged();
	return true;
}

bool UConquestCityManager::GetOwnedTileCombatState(
	const FIntPoint& Coord,
	FCityOwnedTileCombatState& OutTileCombatState
) const
{
	for (const FCityState& City : Cities)
	{
		if (const FCityOwnedTileCombatState* TileCombatState = GetOwnedTileCombatStateForCity(City, Coord))
		{
			OutTileCombatState = *TileCombatState;
			return true;
		}
	}

	OutTileCombatState = FCityOwnedTileCombatState();
	return false;
}

bool UConquestCityManager::IsEnemyUnitOnTile(int32 PlayerId, const FIntPoint& Coord) const
{
	if (!GameStateRef)
	{
		return false;
	}

	for (const FConquestPlayerEmpireState& Player : GameStateRef->PlayerEmpires)
	{
		if (Player.PlayerId == PlayerId)
		{
			continue;
		}

		for (const FConquestUnitState& Unit : Player.Units)
		{
			if (Unit.TileCoord == Coord)
			{
				return true;
			}
		}
	}

	return false;
}

bool UConquestCityManager::CaptureCity(int32 CityId, int32 NewOwnerPlayerId)
{
	FCityState* City = GetCityMutable(CityId);
	if (!GameStateRef || !City || NewOwnerPlayerId == INDEX_NONE || City->OwnerPlayerId == NewOwnerPlayerId)
	{
		return false;
	}

	const int32 PreviousOwnerPlayerId = City->OwnerPlayerId;
	FConquestPlayerEmpireState& PreviousOwner = GameStateRef->GetPlayerEmpireMutable(PreviousOwnerPlayerId);
	PreviousOwner.CityIds.Remove(CityId);

	FConquestPlayerEmpireState& NewOwner = GameStateRef->GetPlayerEmpireMutable(NewOwnerPlayerId);
	NewOwner.CityIds.AddUnique(CityId);

	City->OwnerPlayerId = NewOwnerPlayerId;
	City->CurrentHealth = 0;
	City->ProductionProgressCache.Reset();
	RefreshCityCombatStats(*City);

	if (FHexGridModel* GridModel = GameStateRef->GetHexGridModelMutable())
	{
		for (const FIntPoint& Coord : City->OwnedTiles)
		{
			if (FHexTileData* Tile = GridModel->GetTileMutable(Coord))
			{
				Tile->Gameplay.OwnerPlayerId = NewOwnerPlayerId;
				Tile->Gameplay.OwningCityId = City->CityId;
				if (Tile->Gameplay.WorkedByCityId == City->CityId)
				{
					Tile->Gameplay.bIsWorked = true;
				}
			}
		}
	}

	AutoAssignWorkedTiles(*City);
	RecalculateCityYields(*City);
	RecalculateEmpireYields(PreviousOwnerPlayerId);
	RecalculateEmpireYields(NewOwnerPlayerId);
	UpdateOwnedTileVisuals(PreviousOwnerPlayerId);
	UpdateOwnedTileVisuals(NewOwnerPlayerId);
	UpdateCityWorldLabel(*City);
	OnCityChanged.Broadcast(City->CityId);
	GameStateRef->BroadcastStateChanged();
	return true;
}

void UConquestCityManager::RecalculateCityYields(FCityState& City)
{
	if (!GameStateRef || !GameStateRef->YieldManager)
	{
		return;
	}

	City.CachedYieldPerTurn = GameStateRef->YieldManager->CalculateCityTotalYields(City);
	City.CachedYieldPerTurn += GetProductionProjectYieldBonus(City);
}

FHexYield UConquestCityManager::GetProductionProjectYieldBonus(const FCityState& City) const
{
	FHexYield Result;
	if (City.CurrentProduction.Type != ECityProductionType::Project)
	{
		return Result;
	}

	const int32 ProductionPerTurn = FMath::Max(0, City.CachedYieldPerTurn.Production);
	if (ProductionPerTurn <= 0)
	{
		return Result;
	}

	if (City.CurrentProduction.ProductionId == ConquestProductionProjectFoodFocus)
	{
		Result.Food = ProductionPerTurn;
	}
	else if (City.CurrentProduction.ProductionId == ConquestProductionProjectGoldFocus)
	{
		Result.Gold = ProductionPerTurn;
	}
	else if (City.CurrentProduction.ProductionId == ConquestProductionProjectCultureFocus)
	{
		Result.Culture = ProductionPerTurn;
	}
	else if (City.CurrentProduction.ProductionId == ConquestProductionProjectScienceFocus)
	{
		Result.Science = ProductionPerTurn;
	}

	return Result;
}

void UConquestCityManager::CacheCurrentProductionProgress(FCityState& City) const
{
	if (
		!City.CurrentProduction.IsValid() ||
		City.CurrentProduction.Type == ECityProductionType::Project ||
		City.CurrentProduction.Progress <= 0.0f
	)
	{
		return;
	}

	FCityProductionProgressCacheEntry* CacheEntry =
		City.ProductionProgressCache.FindByPredicate([&City](const FCityProductionProgressCacheEntry& Entry)
		{
			return Entry.Matches(City.CurrentProduction.Type, City.CurrentProduction.ProductionId);
		});

	if (!CacheEntry)
	{
		CacheEntry = &City.ProductionProgressCache.AddDefaulted_GetRef();
		CacheEntry->Type = City.CurrentProduction.Type;
		CacheEntry->ProductionId = City.CurrentProduction.ProductionId;
	}

	CacheEntry->Cost = City.CurrentProduction.Cost;
	CacheEntry->Progress = FMath::Clamp(City.CurrentProduction.Progress, 0.0f, FMath::Max(0.0f, City.CurrentProduction.Cost));
}

float UConquestCityManager::GetCachedProductionProgress(
	const FCityState& City,
	ECityProductionType ProductionType,
	FName ProductionId,
	float Cost
) const
{
	const FCityProductionProgressCacheEntry* CacheEntry =
		City.ProductionProgressCache.FindByPredicate([ProductionType, ProductionId](const FCityProductionProgressCacheEntry& Entry)
		{
			return Entry.Matches(ProductionType, ProductionId);
		});

	if (!CacheEntry || !CacheEntry->IsValid())
	{
		return 0.0f;
	}

	return FMath::Clamp(CacheEntry->Progress, 0.0f, FMath::Max(0.0f, Cost));
}

void UConquestCityManager::ClearCachedProductionProgress(
	FCityState& City,
	ECityProductionType ProductionType,
	FName ProductionId
) const
{
	City.ProductionProgressCache.RemoveAll([ProductionType, ProductionId](const FCityProductionProgressCacheEntry& Entry)
	{
		return Entry.Matches(ProductionType, ProductionId);
	});
}

void UConquestCityManager::RecalculateEmpireYields(int32 PlayerId)
{
	if (GameStateRef && GameStateRef->YieldManager)
	{
		GameStateRef->YieldManager->RecalculateEmpireHappiness(PlayerId);

		for (FCityState& City : Cities)
		{
			if (City.OwnerPlayerId == PlayerId)
			{
				RecalculateCityYields(City);
			}
		}

		FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
		if (Player.PlayerId == PlayerId)
		{
			for (FConquestUnitState& Unit : Player.Units)
			{
				RecalculateUnitStats(Unit);
			}
		}

		GameStateRef->YieldManager->RecalculateEmpireYieldPerTurn(PlayerId);
		RecalculateStrategicResourceEconomy(PlayerId);
	}
}

void UConquestCityManager::RecalculateStrategicResourceEconomy(int32 PlayerId)
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

	for (FConquestStrategicResourceStockpile& Stockpile : Player.StrategicResources)
	{
		Stockpile.PerTurn = 0;
		Stockpile.Cap = 0;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (GridModel)
	{
		for (const FCityState& City : Cities)
		{
			if (City.OwnerPlayerId != PlayerId)
			{
				continue;
			}

			for (const FIntPoint& Coord : City.WorkedTiles)
			{
				const FHexTileData* Tile = GridModel->GetTile(Coord);
				if (!Tile || !Tile->Resource.HasResource() || Tile->ImprovementId.IsNone())
				{
					continue;
				}

				const FHexResourceDefinition* ResourceDefinition =
					GridModel->FindResourceDefinition(Tile->Resource.ResourceId);

				if (!ResourceDefinition || ResourceDefinition->Category != EHexResourceCategory::Strategic)
				{
					continue;
				}

				FConquestStrategicResourceStockpile* Stockpile = Player.FindStrategicResource(Tile->Resource.ResourceId);
				if (!Stockpile)
				{
					Stockpile = &Player.StrategicResources.AddDefaulted_GetRef();
					Stockpile->ResourceId = Tile->Resource.ResourceId;
				}

				Stockpile->PerTurn += FMath::Max(1, Tile->Resource.Quantity);
				Stockpile->Cap = FMath::Max(Stockpile->Cap, ResourceDefinition->StartingStorageCap);
			}
		}
	}

	if (GameStateRef->ContentManager)
	{
		for (const FCityState& City : Cities)
		{
			if (City.OwnerPlayerId != PlayerId)
			{
				continue;
			}

			for (const FName BuildingId : City.ConstructedBuildingIds)
			{
				const FConquestBuildingRow* BuildingRow = GameStateRef->ContentManager->FindBuilding(BuildingId);
				if (!BuildingRow)
				{
					continue;
				}

				for (const FConquestStrategicResourceCapBonus& CapBonus : BuildingRow->StrategicResourceCapBonuses)
				{
					if (CapBonus.ResourceId.IsNone() || CapBonus.CapBonus <= 0)
					{
						continue;
					}

					FConquestStrategicResourceStockpile* Stockpile = Player.FindStrategicResource(CapBonus.ResourceId);
					if (!Stockpile)
					{
						Stockpile = &Player.StrategicResources.AddDefaulted_GetRef();
						Stockpile->ResourceId = CapBonus.ResourceId;
					}

					Stockpile->Cap += CapBonus.CapBonus;
				}
			}
		}
	}

	for (FConquestStrategicResourceStockpile& Stockpile : Player.StrategicResources)
	{
		Stockpile.Stored = FMath::Clamp(Stockpile.Stored, 0, FMath::Max(0, Stockpile.Cap));
	}
}

void UConquestCityManager::AccumulateStrategicResourceIncome(int32 PlayerId)
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

	RecalculateStrategicResourceEconomy(PlayerId);

	for (FConquestStrategicResourceStockpile& Stockpile : Player.StrategicResources)
	{
		Stockpile.Stored = FMath::Clamp(
			Stockpile.Stored + Stockpile.PerTurn,
			0,
			FMath::Max(0, Stockpile.Cap)
		);
	}
}

void UConquestCityManager::RecalculateUnitStats(FConquestUnitState& Unit) const
{
	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return;
	}

	const FConquestUnitRow* UnitRow = GameStateRef->ContentManager->FindUnit(Unit.UnitId);
	if (!UnitRow)
	{
		return;
	}

	Unit.CachedStrength = UnitRow->Strength;
	Unit.CachedAttackRange = UnitRow->AttackRange;
	Unit.CachedMaxHealth = UnitRow->MaxHealth;
	Unit.CachedHealthRegenPerTurn = UnitRow->HealthRegenPerTurn;
	Unit.CachedMovementPoints = UnitRow->MovementPoints;
	Unit.CachedGoldMaintenancePerTurn = UnitRow->GoldMaintenancePerTurn;
	Unit.CombatModifiers.RemoveAll([](const FConquestUnitCombatModifier& Modifier)
	{
		return
			Modifier.ModifierId == ConquestUnhappyAttackModifierId ||
			Modifier.ModifierId == ConquestUnhappyDefenseModifierId ||
			Modifier.ModifierId == ConquestTileDefenseModifierId;
	});

	for (const FConquestUnitAugmentState& Augment : Unit.Augments)
	{
		switch (Augment.ModifiedStat)
		{
		case EConquestUnitAugmentStat::Strength:
			Unit.CachedStrength += Augment.FlatBonus;
			break;
		case EConquestUnitAugmentStat::AttackRange:
			Unit.CachedAttackRange += Augment.FlatBonus;
			break;
		case EConquestUnitAugmentStat::MaxHealth:
			Unit.CachedMaxHealth += Augment.FlatBonus;
			break;
		case EConquestUnitAugmentStat::HealthRegen:
			Unit.CachedHealthRegenPerTurn += Augment.FlatBonus;
			break;
		case EConquestUnitAugmentStat::Movement:
			Unit.CachedMovementPoints += Augment.FlatBonus;
			break;
		case EConquestUnitAugmentStat::AttackMultiplier:
		case EConquestUnitAugmentStat::DefenseMultiplier:
			break;
		default:
			break;
		}
	}

	Unit.CachedAttackRange = FMath::Max(1, Unit.CachedAttackRange);
	Unit.CachedMaxHealth = FMath::Max(1, Unit.CachedMaxHealth);
	Unit.CachedMovementPoints = FMath::Max(0, Unit.CachedMovementPoints);

	const FConquestPlayerEmpireState& OwnerPlayer = GameStateRef->GetPlayerEmpire(Unit.OwnerPlayerId);
	const float HappinessCombatMultiplier = OwnerPlayer.PlayerId == Unit.OwnerPlayerId
		? ConquestHappiness::GetPenaltyMultiplier(OwnerPlayer.CachedHappiness)
		: 1.0f;
	if (HappinessCombatMultiplier < 1.0f)
	{
		FConquestUnitCombatModifier AttackModifier;
		AttackModifier.ModifierId = ConquestUnhappyAttackModifierId;
		AttackModifier.ModifierType = EConquestUnitCombatModifierType::Attack;
		AttackModifier.Multiplier = HappinessCombatMultiplier;
		Unit.CombatModifiers.Add(AttackModifier);

		FConquestUnitCombatModifier DefenseModifier;
		DefenseModifier.ModifierId = ConquestUnhappyDefenseModifierId;
		DefenseModifier.ModifierType = EConquestUnitCombatModifierType::Defense;
		DefenseModifier.Multiplier = HappinessCombatMultiplier;
		Unit.CombatModifiers.Add(DefenseModifier);
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	const FHexTileData* Tile = GridModel ? GridModel->GetTile(Unit.TileCoord) : nullptr;
	if (
		Tile &&
		Tile->Gameplay.OwnerPlayerId == Unit.OwnerPlayerId &&
		Tile->Gameplay.DefenderModifier > 1.0f
	)
	{
		FConquestUnitCombatModifier TileDefenseModifier;
		TileDefenseModifier.ModifierId = ConquestTileDefenseModifierId;
		TileDefenseModifier.ModifierType = EConquestUnitCombatModifierType::Defense;
		TileDefenseModifier.Multiplier = Tile->Gameplay.DefenderModifier;
		Unit.CombatModifiers.Add(TileDefenseModifier);
	}

	Unit.CurrentHealth = Unit.CurrentHealth <= 0
		? Unit.CachedMaxHealth
		: FMath::Clamp(Unit.CurrentHealth, 1, Unit.CachedMaxHealth);
	Unit.CurrentMovementPoints = FMath::Clamp(
		Unit.CurrentMovementPoints,
		0,
		Unit.CachedMovementPoints
	);
}

int32 UConquestCityManager::CreateUnitFromProduction(const FCityState& City, FName UnitId)
{
	if (!GameStateRef || !GameStateRef->ContentManager || UnitId.IsNone())
	{
		return INDEX_NONE;
	}

	const FConquestUnitRow* UnitRow = GameStateRef->ContentManager->FindUnit(UnitId);
	if (!UnitRow)
	{
		return INDEX_NONE;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(City.OwnerPlayerId);
	if (Player.PlayerId != City.OwnerPlayerId)
	{
		return INDEX_NONE;
	}

	FConquestUnitState NewUnit;
	NewUnit.UnitInstanceId = Player.NextUnitInstanceId++;
	NewUnit.OwnerPlayerId = City.OwnerPlayerId;
	NewUnit.UnitId = UnitRow->UnitId;
	NewUnit.SourceCityId = City.CityId;
	NewUnit.TileCoord = City.CenterTile;
	NewUnit.CurrentHealth = UnitRow->MaxHealth;

	RecalculateUnitStats(NewUnit);
	NewUnit.CurrentMovementPoints = NewUnit.CachedMovementPoints;

	Player.Units.Add(NewUnit);
	SpawnUnitActorForState(NewUnit);
	return NewUnit.UnitInstanceId;
}

void UConquestCityManager::ProcessUnitsAtStartOfTurn(int32 PlayerId)
{
	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	if (Player.PlayerId != PlayerId)
	{
		return;
	}

	for (FConquestUnitState& Unit : Player.Units)
	{
		if (Unit.OwnerPlayerId != PlayerId)
		{
			continue;
		}

		RecalculateUnitStats(Unit);
		Unit.CurrentMovementPoints = Unit.CachedMovementPoints;

		if (Unit.bIsFortified)
		{
			constexpr int32 FortifyHealPerTurn = 10;
			Unit.CurrentHealth = FMath::Clamp(
				Unit.CurrentHealth + FortifyHealPerTurn,
				1,
				FMath::Max(1, Unit.CachedMaxHealth)
			);
		}

		if (TObjectPtr<AConquestUnitActor>* UnitActorPtr =
			GameStateRef->UnitActorsByInstanceId.Find(Unit.UnitInstanceId))
		{
			if (AConquestUnitActor* UnitActor = UnitActorPtr->Get())
			{
				if (const FConquestUnitRow* UnitRow = GameStateRef->ContentManager->FindUnit(Unit.UnitId))
				{
					const FText UnitName = !UnitRow->DisplayName.IsEmpty()
						? UnitRow->DisplayName
						: FText::FromName(Unit.UnitId);
					FText CivilisationName = FText::GetEmpty();
					FLinearColor UnitDisplayColor = FLinearColor::White;
					UMaterialInterface* UnitIconMaterial = nullptr;
					if (const UConquestCivilisationData* Civilisation = GameStateRef->GetCivilisationForPlayer(Unit.OwnerPlayerId))
					{
						CivilisationName = Civilisation->CivilisationName;
						UnitDisplayColor = Civilisation->ThemeColor;
						UnitIconMaterial = Civilisation->CityLabelMaterial
							? Civilisation->CityLabelMaterial.Get()
							: Civilisation->BorderMaterial.Get();
					}

					UnitActor->RefreshUnitVisuals(
						Unit,
						*UnitRow,
						UnitName,
						CivilisationName,
						UnitDisplayColor,
						UnitIconMaterial
					);
				}
			}
		}
	}
}

void UConquestCityManager::SpawnUnitActorForState(const FConquestUnitState& UnitState)
{
	if (!GameStateRef || !GameStateRef->ContentManager || !GameStateRef->ActiveGridActor)
	{
		return;
	}

	const FConquestUnitRow* UnitRow = GameStateRef->ContentManager->FindUnit(UnitState.UnitId);
	if (!UnitRow)
	{
		return;
	}

	TSubclassOf<AConquestUnitActor> ActorClass = GameStateRef->UnitActorClass;
	if (!ActorClass)
	{
		ActorClass = AConquestUnitActor::StaticClass();
	}

	UWorld* World = GameStateRef->GetWorld();
	if (!World)
	{
		return;
	}

	AConquestUnitActor* UnitActor = World->SpawnActor<AConquestUnitActor>(
		ActorClass,
		GameStateRef->ActiveGridActor->GetActorTransform()
	);

	if (!UnitActor)
	{
		return;
	}

	if (GameStateRef->ActiveGridActor->UnitWorldIconWidgetClass)
	{
		UnitActor->SetUnitWorldIconWidgetClass(GameStateRef->ActiveGridActor->UnitWorldIconWidgetClass);
	}

	const FText UnitName = !UnitRow->DisplayName.IsEmpty()
		? UnitRow->DisplayName
		: FText::FromName(UnitState.UnitId);
	FText CivilisationName = FText::GetEmpty();
	FLinearColor UnitDisplayColor = FLinearColor::White;
	UMaterialInterface* UnitIconMaterial = nullptr;
	if (const UConquestCivilisationData* Civilisation = GameStateRef->GetCivilisationForPlayer(UnitState.OwnerPlayerId))
	{
		CivilisationName = Civilisation->CivilisationName;
		UnitDisplayColor = Civilisation->ThemeColor;
		UnitIconMaterial = Civilisation->CityLabelMaterial
			? Civilisation->CityLabelMaterial.Get()
			: Civilisation->BorderMaterial.Get();
	}

	UnitActor->InitializeUnit(
		UnitState,
		*UnitRow,
		GameStateRef->ActiveGridActor,
		UnitName,
		CivilisationName,
		UnitDisplayColor,
		UnitIconMaterial
	);
	GameStateRef->UnitActorsByInstanceId.Add(UnitState.UnitInstanceId, UnitActor);
}

void UConquestCityManager::UpdateOwnedTileVisuals(int32 PlayerId)
{
	if (!GameStateRef || !GameStateRef->ActiveGridActor)
	{
		return;
	}

	if (PlayerId == INDEX_NONE)
	{
		return;
	}

	GameStateRef->RebuildCityVisualsFromReplicatedState();
}

void UConquestCityManager::UpdateCityWorldLabel(const FCityState& City)
{
	if (!GameStateRef || !GameStateRef->ActiveGridActor)
	{
		return;
	}

	UMaterialInterface* CivilisationThemeMaterial = nullptr;
	FLinearColor CivilisationThemeColor = FLinearColor::White;
	if (const UConquestCivilisationData* Civilisation = GameStateRef->GetCivilisationForPlayer(City.OwnerPlayerId))
	{
		CivilisationThemeMaterial = Civilisation->CityLabelMaterial
			? Civilisation->CityLabelMaterial
			: Civilisation->BorderMaterial;
		CivilisationThemeColor = Civilisation->ThemeColor;
	}

	GameStateRef->ActiveGridActor->AddOrUpdateCityWorldLabel(
		City.CityId,
		City.CenterTile,
		City.CityName,
		City.Population,
		City.CurrentHealth,
		City.MaxHealth,
		City.CachedStrength,
		CivilisationThemeMaterial,
		CivilisationThemeColor
	);
}

void UConquestCityManager::UpdateOwnedTileHealthBar(
	const FCityState& City,
	const FCityOwnedTileCombatState& TileCombatState
)
{
	if (!GameStateRef || !GameStateRef->ActiveGridActor)
	{
		return;
	}

	if (!City.OwnedTiles.Contains(TileCombatState.Coord) || TileCombatState.Coord == City.CenterTile)
	{
		GameStateRef->ActiveGridActor->RemoveTileHealthBar(TileCombatState.Coord);
		return;
	}

	GameStateRef->ActiveGridActor->AddOrUpdateTileHealthBar(
		TileCombatState.Coord,
		TileCombatState.CurrentHealth,
		TileCombatState.MaxHealth,
		TileCombatState.CombatStrength
	);
}

FName UConquestCityManager::ResolveCityName(int32 PlayerId, FName RequestedCityName) const
{
	if (!RequestedCityName.IsNone())
	{
		return RequestedCityName;
	}

	const TArray<FName>* CivilisationCityNames = nullptr;
	if (GameStateRef)
	{
		if (const UConquestCivilisationData* Civilisation = GameStateRef->GetCivilisationForPlayer(PlayerId))
		{
			if (Civilisation->CityNames.Num() > 0)
			{
				CivilisationCityNames = &Civilisation->CityNames;
			}
		}
	}

	if (!CivilisationCityNames)
	{
		return FName(TEXT("New City"));
	}

	TSet<FName> ExistingCityNames;
	int32 ExistingCityCount = 0;
	for (const FCityState& City : Cities)
	{
		if (City.OwnerPlayerId != PlayerId)
		{
			continue;
		}

		++ExistingCityCount;
		ExistingCityNames.Add(City.CityName);
	}

	for (const FName CandidateName : *CivilisationCityNames)
	{
		if (!CandidateName.IsNone() && !ExistingCityNames.Contains(CandidateName))
		{
			return CandidateName;
		}
	}

	const int32 NameCount = CivilisationCityNames->Num();
	for (int32 Attempt = 0; Attempt < NameCount * 100; ++Attempt)
	{
		const int32 NameIndex = (ExistingCityCount + Attempt) % NameCount;
		const FName BaseName = (*CivilisationCityNames)[NameIndex];
		if (BaseName.IsNone())
		{
			continue;
		}

		const int32 Suffix = ((ExistingCityCount + Attempt) / NameCount) + 1;
		const FName CandidateName(*FString::Printf(
			TEXT("%s_%d"),
			*BaseName.ToString(),
			FMath::Max(2, Suffix)
		));

		if (!ExistingCityNames.Contains(CandidateName))
		{
			return CandidateName;
		}
	}

	return FName(*FString::Printf(TEXT("New City_%d"), ExistingCityCount + 1));
}

void UConquestCityManager::ProcessCitiesAtStartOfTurn(int32 PlayerId)
{
	for (FCityState& City : Cities)
	{
		if (City.OwnerPlayerId != PlayerId)
		{
			continue;
		}

		AutoAssignWorkedTiles(City);
		HealCityAtStartOfTurn(City);
		HealOwnedTilesAtStartOfTurn(City);
		RecalculateCityYields(City);
		ProcessCityGrowth(City);
		RefreshCityCombatStats(City);
		ProcessCityProduction(City);
		RecalculateCityYields(City);
		UpdateCityWorldLabel(City);

		OnCityChanged.Broadcast(City.CityId);
	}

	if (GameStateRef)
	{
		RecalculateEmpireYields(PlayerId);
		AccumulateStrategicResourceIncome(PlayerId);
		GameStateRef->BroadcastStateChanged();
	}
}

void UConquestCityManager::ProcessCityGrowth(FCityState& City)
{
	const int32 FoodUpkeep = City.Population * 2;
	const int32 FoodSurplus = City.CachedYieldPerTurn.Food - FoodUpkeep;

	if (FoodSurplus <= 0)
	{
		return;
	}

	City.FoodStored += FoodSurplus;

	const int32 GrowthCost = GetFoodRequiredForNextPopulation(City);
	City.CachedFoodRequiredForNextPopulation = GrowthCost;

	if (City.FoodStored >= GrowthCost)
	{
		City.FoodStored -= GrowthCost;
		City.Population += 1;
		City.PendingBorderExpansions += 1;
		City.CachedFoodRequiredForNextPopulation = GetFoodRequiredForNextPopulation(City);
		RefreshCityCombatStats(City);
		OnCityNeedsBorderExpansion.Broadcast(City.CityId);
		AutoAssignWorkedTiles(City);
	}
}

int32 UConquestCityManager::GetFoodRequiredForNextPopulation(const FCityState& City) const
{
	constexpr float BaseFoodCost = 10.0f;
	constexpr float GrowthExponent = 1.35f;
	const float PopulationStep = FMath::Max(0.0f, static_cast<float>(City.Population - 1));
	return FMath::Max(1, FMath::RoundToInt(BaseFoodCost * FMath::Pow(GrowthExponent, PopulationStep)));
}

void UConquestCityManager::ProcessCityProduction(FCityState& City)
{
	if (!City.CurrentProduction.IsValid())
	{
		City.bNeedsProductionChoice = true;
		return;
	}

	if (City.CurrentProduction.Type == ECityProductionType::Project)
	{
		City.bNeedsProductionChoice = false;
		return;
	}

	const int32 ProductionPerTurn = FMath::Max(0, City.CachedYieldPerTurn.Production);
	if (ProductionPerTurn <= 0)
	{
		return;
	}

	City.CurrentProduction.Progress += ProductionPerTurn;

	if (City.CurrentProduction.Progress < City.CurrentProduction.Cost)
	{
		return;
	}

	if (City.CurrentProduction.Type == ECityProductionType::Building &&	!City.CurrentProduction.ProductionId.IsNone())
	{
		City.ConstructedBuildingIds.AddUnique(City.CurrentProduction.ProductionId);
		ClearCachedProductionProgress(City, City.CurrentProduction.Type, City.CurrentProduction.ProductionId);
	}
	else if (City.CurrentProduction.Type == ECityProductionType::Unit && !City.CurrentProduction.ProductionId.IsNone())
	{
		CreateUnitFromProduction(City, City.CurrentProduction.ProductionId);
		ClearCachedProductionProgress(City, City.CurrentProduction.Type, City.CurrentProduction.ProductionId);
	}

	City.CurrentProduction.Clear();
	City.bNeedsProductionChoice = true;
}

TArray<FIntPoint> UConquestCityManager::GetExpansionCandidateTiles(int32 CityId) const
{
	TArray<FIntPoint> Result;

	const FCityState* City = GetCity(CityId);
	if (!City || !GameStateRef)
	{
		return Result;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (!GridModel)
	{
		return Result;
	}

	for (const FIntPoint& OwnedCoord : City->OwnedTiles)
	{
		if (IsValidPopulationAssignmentTile(*City, OwnedCoord))
		{
			Result.AddUnique(OwnedCoord);
		}

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NeighborQ = INDEX_NONE;
			int32 NeighborR = INDEX_NONE;
			if (!GridModel->GetNeighbourCoord(OwnedCoord.X, OwnedCoord.Y, Direction, NeighborQ, NeighborR))
			{
				continue;
			}

			const FIntPoint Candidate(NeighborQ, NeighborR);
			if (IsValidExpansionTileForCity(*City, Candidate))
			{
				Result.AddUnique(Candidate);
			}
		}
	}

	Result.Sort([this, City](const FIntPoint& A, const FIntPoint& B)
	{
		return ScoreTileForExpansion(*City, A) > ScoreTileForExpansion(*City, B);
	});

	return Result;
}

bool UConquestCityManager::ClaimExpansionTileForCity(int32 CityId, const FIntPoint& Coord)
{
	FCityState* City = GetCityMutable(CityId);
	if (!City || City->PendingBorderExpansions <= 0 || !IsValidPopulationAssignmentTile(*City, Coord))
	{
		return false;
	}

	const bool bWasAlreadyOwned = City->OwnedTiles.Contains(Coord);
	if (!bWasAlreadyOwned && !ClaimTileForCity(*City, Coord))
	{
		return false;
	}

	if (!AssignCitizenToTile(*City, Coord))
	{
		return false;
	}

	City->PendingBorderExpansions = FMath::Max(0, City->PendingBorderExpansions - 1);
	RecalculateCityYields(*City);
	RecalculateEmpireYields(City->OwnerPlayerId);

	if (!bWasAlreadyOwned)
	{
		UpdateOwnedTileVisuals(City->OwnerPlayerId);
	}

	OnCityChanged.Broadcast(City->CityId);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}

	return true;
}

TArray<FName> UConquestCityManager::GetAvailableTileImprovementIdsForPlayer(int32 PlayerId, const FIntPoint& Coord) const
{
	TArray<FName> Result;

	if (!GameStateRef)
	{
		return Result;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (!GridModel)
	{
		return Result;
	}

	const FHexTileData* Tile = GridModel->GetTile(Coord);
	if (!Tile || Tile->Gameplay.OwnerPlayerId != PlayerId || !Tile->ImprovementId.IsNone())
	{
		return Result;
	}

	TArray<FName> PossibleImprovementIds;
	GridModel->GetPossibleImprovementIdsForTile(Coord.X, Coord.Y, PossibleImprovementIds);
	for (const FName ImprovementId : PossibleImprovementIds)
	{
		if (IsTileImprovementUnlockedForPlayer(PlayerId, ImprovementId))
		{
			Result.Add(ImprovementId);
		}
	}
	return Result;
}

bool UConquestCityManager::PurchaseTileImprovementForPlayer(int32 PlayerId, const FIntPoint& Coord, FName ImprovementId)
{
	if (!GameStateRef || !GameStateRef->ActiveGridActor || ImprovementId.IsNone())
	{
		return false;
	}

	FHexGridModel* GridModel = GameStateRef->GetHexGridModelMutable();
	if (!GridModel)
	{
		return false;
	}

	FHexTileData* Tile = GridModel->GetTileMutable(Coord);
	if (!Tile || Tile->Gameplay.OwnerPlayerId != PlayerId || !Tile->ImprovementId.IsNone())
	{
		return false;
	}

	TArray<const FHexImprovementDefinition*> PossibleImprovements;
	GridModel->GetPossibleImprovementsForTile(Coord.X, Coord.Y, PossibleImprovements);

	const FHexImprovementDefinition* ChosenImprovement = nullptr;
	for (const FHexImprovementDefinition* Improvement : PossibleImprovements)
	{
		if (Improvement && Improvement->ImprovementId == ImprovementId)
		{
			ChosenImprovement = Improvement;
			break;
		}
	}

	if (!ChosenImprovement)
	{
		return false;
	}

	if (!IsTileImprovementUnlockedForPlayer(PlayerId, ImprovementId))
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	if (Player.PlayerId != PlayerId)
	{
		return false;
	}

	const int32 GoldCost = FMath::Max(0, ChosenImprovement->PurchaseGoldCost);
	if (Player.StoredYields.Gold < GoldCost)
	{
		return false;
	}

	Player.StoredYields.Gold -= GoldCost;

	const bool bApplied = GameStateRef->ActiveGridActor->SetTileImprovement(
		Coord.X,
		Coord.Y,
		ImprovementId
	);

	if (!bApplied)
	{
		Player.StoredYields.Gold += GoldCost;
		return false;
	}

	if (Tile->Gameplay.OwningCityId != INDEX_NONE)
	{
		RefreshCityYields(Tile->Gameplay.OwningCityId);
	}

	RecalculateEmpireYields(PlayerId);
	RecalculateStrategicResourceEconomy(PlayerId);
	GameStateRef->BroadcastStateChanged();

	return true;
}

bool UConquestCityManager::RefreshCityYields(int32 CityId)
{
	FCityState* City = GetCityMutable(CityId);
	if (!City)
	{
		return false;
	}

	AutoAssignWorkedTiles(*City);
	RecalculateCityYields(*City);
	RecalculateEmpireYields(City->OwnerPlayerId);

	OnCityChanged.Broadcast(City->CityId);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}

	return true;
}

float UConquestCityManager::ScoreTileForExpansion(const FCityState& City, const FIntPoint& Coord) const
{
	if (!GameStateRef || !GameStateRef->YieldManager)
	{
		return 0.0f;
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	if (!GridModel)
	{
		return 0.0f;
	}

	const FHexTileData* Tile = GridModel->GetTile(Coord);
	if (!Tile)
	{
		return 0.0f;
	}

	const FHexYield TileYield = GameStateRef->YieldManager->CalculateTileYield(*Tile);

	float Score = TileYield.GetWeightedScore(3.0f, 3.0f, 1.5f, 2.0f, 2.0f, 1.0f);
	if (City.OwnedTiles.Contains(Coord))
	{
		const int32 AssignedCitizens = GetAssignedCitizensForTile(City, Coord);
		Score *= AssignedCitizens <= 0 ? 1.0f : 0.5f;
	}

	const int32 Distance =
		FMath::Abs(City.CenterTile.X - Coord.X) +
		FMath::Abs(City.CenterTile.Y - Coord.Y);

	Score -= Distance * 0.25f;

	return Score;
}

bool UConquestCityManager::SetCityProductionBuildingById(int32 CityId, FName BuildingId)
{
	if (BuildingId.IsNone())
	{
		return false;
	}

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return false;
	}

	FCityState* City = GetCityMutable(CityId);
	if (!City)
	{
		return false;
	}

	const FConquestBuildingRow* BuildingRow =
		GameStateRef->ContentManager->FindBuilding(BuildingId);

	if (!BuildingRow)
	{
		return false;
	}

	if (BuildingRow->bHideFromProductionChoices || BuildingRow->bGrantedOnCityFounding)
	{
		return false;
	}

	// Prevent directly building something already constructed.
	if (City->ConstructedBuildingIds.Contains(BuildingId))
	{
		return false;
	}

	// If this is a replacement building, check against the base building.
	const FName BaseBuildingId = !BuildingRow->ReplacesBuildingId.IsNone()
		? BuildingRow->ReplacesBuildingId
		: BuildingRow->BuildingId;

	if (CityHasBuildingOrReplacement(*City, BaseBuildingId))
	{
		return false;
	}

	CacheCurrentProductionProgress(*City);
	City->CurrentProduction.Type = ECityProductionType::Building;
	City->CurrentProduction.ProductionId = BuildingRow->BuildingId;
	City->CurrentProduction.Cost = BuildingRow->ProductionCost;
	City->CurrentProduction.Progress = GetCachedProductionProgress(
		*City,
		City->CurrentProduction.Type,
		City->CurrentProduction.ProductionId,
		City->CurrentProduction.Cost
	);
	City->bNeedsProductionChoice = false;

	OnCityChanged.Broadcast(CityId);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}

	return true;
}

bool UConquestCityManager::SetCityProductionUnitById(int32 CityId, FName UnitId)
{
	if (UnitId.IsNone())
	{
		return false;
	}

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return false;
	}

	FCityState* City = GetCityMutable(CityId);
	if (!City)
	{
		return false;
	}

	const FConquestUnitRow* UnitRow = GameStateRef->ContentManager->FindUnit(UnitId);
	if (!UnitRow)
	{
		return false;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(City->OwnerPlayerId);
	if (!UnitRow->RequiredTechId.IsNone() && !Player.HasResearched(UnitRow->RequiredTechId))
	{
		return false;
	}

	CacheCurrentProductionProgress(*City);
	City->CurrentProduction.Type = ECityProductionType::Unit;
	City->CurrentProduction.ProductionId = UnitRow->UnitId;
	City->CurrentProduction.Cost = UnitRow->ProductionCost;
	City->CurrentProduction.Progress = GetCachedProductionProgress(
		*City,
		City->CurrentProduction.Type,
		City->CurrentProduction.ProductionId,
		City->CurrentProduction.Cost
	);
	City->bNeedsProductionChoice = false;

	OnCityChanged.Broadcast(CityId);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}

	return true;
}

bool UConquestCityManager::SetCityProductionProjectById(int32 CityId, FName ProjectId)
{
	if (ProjectId.IsNone() || !GetConquestProductionProjectIds().Contains(ProjectId))
	{
		return false;
	}

	FCityState* City = GetCityMutable(CityId);
	if (!City)
	{
		return false;
	}

	CacheCurrentProductionProgress(*City);
	City->CurrentProduction.Type = ECityProductionType::Project;
	City->CurrentProduction.ProductionId = ProjectId;
	City->CurrentProduction.Progress = 0.0f;
	City->CurrentProduction.Cost = 1.0f;
	City->bNeedsProductionChoice = false;

	RecalculateCityYields(*City);
	RecalculateEmpireYields(City->OwnerPlayerId);
	OnCityChanged.Broadcast(CityId);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}

	return true;
}

TArray<FName> UConquestCityManager::GetAvailableProductionBuildingIdsForCity(int32 CityId) const
{
	TArray<FName> Result;

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return Result;
	}

	const FCityState* City = GetCity(CityId);
	if (!City)
	{
		return Result;
	}

	TArray<const FConquestBuildingRow*> BaseBuildings;
	GameStateRef->ContentManager->GetAllBaseBuildings(BaseBuildings);

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(City->OwnerPlayerId);

	for (const FConquestBuildingRow* BaseRow : BaseBuildings)
	{
		if (!BaseRow)
		{
			continue;
		}

		const FName BaseBuildingId = BaseRow->BuildingId;

		if (BaseRow->bHideFromProductionChoices || BaseRow->bGrantedOnCityFounding)
		{
			continue;
		}

		if (CityHasBuildingOrReplacement(*City, BaseBuildingId))
		{
			continue;
		}

		const FConquestBuildingRow* ResolvedRow =
			GameStateRef->ContentManager->ResolveBuildingForPlayer(City->OwnerPlayerId, BaseBuildingId);

		if (!ResolvedRow)
		{
			continue;
		}

		if (!ResolvedRow->RequiredTechId.IsNone() && !Player.HasResearched(ResolvedRow->RequiredTechId))
		{
			continue;
		}

		if (ResolvedRow->RequiredPhilosophyTenets > Player.AdoptedPhilosophyTenets)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("Available building: Base=%s Resolved=%s"),
	*BaseBuildingId.ToString(),
	*ResolvedRow->BuildingId.ToString()
);
		
		Result.Add(ResolvedRow->BuildingId);
	}

	return Result;
}

TArray<FName> UConquestCityManager::GetAvailableProductionUnitIdsForCity(int32 CityId) const
{
	TArray<FName> Result;

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return Result;
	}

	const FCityState* City = GetCity(CityId);
	if (!City)
	{
		return Result;
	}

	TArray<const FConquestUnitRow*> BaseUnits;
	GameStateRef->ContentManager->GetAllBaseUnits(BaseUnits);

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(City->OwnerPlayerId);

	for (const FConquestUnitRow* BaseRow : BaseUnits)
	{
		if (!BaseRow)
		{
			continue;
		}

		const FName BaseUnitId = BaseRow->UnitId;
		const FConquestUnitRow* ResolvedRow =
			GameStateRef->ContentManager->ResolveUnitForPlayer(City->OwnerPlayerId, BaseUnitId);

		if (!ResolvedRow)
		{
			continue;
		}

		if (!ResolvedRow->RequiredTechId.IsNone() && !Player.HasResearched(ResolvedRow->RequiredTechId))
		{
			continue;
		}

		Result.Add(ResolvedRow->UnitId);
	}

	return Result;
}

TArray<FName> UConquestCityManager::GetAvailableProductionProjectIdsForCity(int32 CityId) const
{
	TArray<FName> Result;
	if (!GetCity(CityId))
	{
		return Result;
	}

	Result = GetConquestProductionProjectIds();
	return Result;
}

int32 UConquestCityManager::EstimateTurnsToBuildById(int32 CityId, FName BuildingId) const
{
	if (BuildingId.IsNone())
	{
		return INDEX_NONE;
	}

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return INDEX_NONE;
	}

	const FCityState* City = GetCity(CityId);
	if (!City)
	{
		return INDEX_NONE;
	}

	const FConquestBuildingRow* BuildingRow =
		GameStateRef->ContentManager->FindBuilding(BuildingId);

	if (!BuildingRow)
	{
		return INDEX_NONE;
	}

	const int32 ProductionPerTurn = FMath::Max(0, City->CachedYieldPerTurn.Production);
	if (ProductionPerTurn <= 0)
	{
		return INDEX_NONE;
	}

	const float Cost = static_cast<float>(BuildingRow->ProductionCost);
	const float CurrentProgress =
		City->CurrentProduction.Type == ECityProductionType::Building &&
		City->CurrentProduction.ProductionId == BuildingRow->BuildingId
			? City->CurrentProduction.Progress
			: GetCachedProductionProgress(*City, ECityProductionType::Building, BuildingRow->BuildingId, Cost);
	const float RemainingCost = FMath::Max(0.0f, Cost - CurrentProgress);
	return FMath::Max(1, FMath::CeilToInt(RemainingCost / ProductionPerTurn));
}

int32 UConquestCityManager::EstimateTurnsToTrainUnitById(int32 CityId, FName UnitId) const
{
	if (UnitId.IsNone())
	{
		return INDEX_NONE;
	}

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return INDEX_NONE;
	}

	const FCityState* City = GetCity(CityId);
	if (!City)
	{
		return INDEX_NONE;
	}

	const FConquestUnitRow* UnitRow = GameStateRef->ContentManager->FindUnit(UnitId);
	if (!UnitRow)
	{
		return INDEX_NONE;
	}

	const int32 ProductionPerTurn = FMath::Max(0, City->CachedYieldPerTurn.Production);
	if (ProductionPerTurn <= 0)
	{
		return INDEX_NONE;
	}

	const float Cost = static_cast<float>(UnitRow->ProductionCost);
	const float CurrentProgress =
		City->CurrentProduction.Type == ECityProductionType::Unit &&
		City->CurrentProduction.ProductionId == UnitRow->UnitId
			? City->CurrentProduction.Progress
			: GetCachedProductionProgress(*City, ECityProductionType::Unit, UnitRow->UnitId, Cost);
	const float RemainingCost = FMath::Max(0.0f, Cost - CurrentProgress);
	return FMath::Max(1, FMath::CeilToInt(RemainingCost / ProductionPerTurn));
}

int32 UConquestCityManager::EstimateTurnsToGrowth(int32 CityId) const
{
	const FCityState* City = GetCity(CityId);
	if (!City)
	{
		return INDEX_NONE;
	}

	const int32 FoodUpkeep = City->Population * 2;
	const int32 FoodSurplus = City->CachedYieldPerTurn.Food - FoodUpkeep;
	if (FoodSurplus <= 0)
	{
		return INDEX_NONE;
	}

	const int32 GrowthCost = GetFoodRequiredForNextPopulation(*City);
	const float RemainingFood = FMath::Max(
		0.0f,
		static_cast<float>(GrowthCost) - City->FoodStored
	);

	return FMath::Max(1, FMath::CeilToInt(RemainingFood / FoodSurplus));
}

FCityState UConquestCityManager::GetCityCopy(int32 CityId) const
{
	if (const FCityState* City = GetCity(CityId))
	{
		return *City;
	}

	return FCityState();
}

FCityState* UConquestCityManager::GetCityMutable(int32 CityId)
{
	for (FCityState& City : Cities)
	{
		if (City.CityId == CityId)
		{
			return &City;
		}
	}

	return nullptr;
}

const FCityState* UConquestCityManager::GetCity(int32 CityId) const
{
	for (const FCityState& City : Cities)
	{
		if (City.CityId == CityId)
		{
			return &City;
		}
	}

	return nullptr;
}

bool UConquestCityManager::CityHasBuildingOrReplacement(const FCityState& City, FName BaseBuildingId) const
{
	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return false;
	}

	if (City.ConstructedBuildingIds.Contains(BaseBuildingId))
	{
		return true;
	}

	const FName ResolvedId = GameStateRef->ContentManager->ResolveBuildingIdForPlayer(City.OwnerPlayerId, BaseBuildingId);

	if (City.ConstructedBuildingIds.Contains(ResolvedId))
	{
		return true;
	}

	for (const FName BuiltId : City.ConstructedBuildingIds)
	{
		const FConquestBuildingRow* BuiltRow = GameStateRef->ContentManager->FindBuilding(BuiltId);

		if (BuiltRow && BuiltRow->ReplacesBuildingId == BaseBuildingId)
		{
			return true;
		}
	}

	return false;
}
