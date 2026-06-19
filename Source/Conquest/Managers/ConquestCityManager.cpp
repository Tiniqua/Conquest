#include "Conquest/Managers/ConquestCityManager.h"

#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexTileTypes.h"

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
	NewCity.FoodStored = 0.0f;
	NewCity.CachedFoodRequiredForNextPopulation = GetFoodRequiredForNextPopulation(NewCity);
	NewCity.bNeedsProductionChoice = true;

	GrantStartingBuildings(NewCity);
	ClaimTileForCity(NewCity, TileCoord);
	AutoAssignWorkedTiles(NewCity);
	RecalculateCityYields(NewCity);

	Cities.Add(NewCity);

	FConquestPlayerEmpireState& Player = GameStateRef->GetHumanPlayerMutable();
	Player.CityIds.Add(NewCity.CityId);

	if (GameStateRef->ActiveGridActor)
	{
		UStaticMesh* CityMesh = nullptr;
		UMaterialInterface* CityMaterial = nullptr;
		UMaterialInterface* CivilisationThemeMaterial = nullptr;
		FLinearColor CivilisationThemeColor = FLinearColor::White;
		bool bOverrideCityScale = false;
		FVector CityScale = FVector::OneVector;
		if (GameStateRef->HumanPlayer.PlayerId == PlayerId && GameStateRef->HumanCivilisation)
		{
			CityMesh = GameStateRef->HumanCivilisation->CityMesh;
			CityMaterial = GameStateRef->HumanCivilisation->CityMeshMaterialOverride;
			CivilisationThemeMaterial = GameStateRef->HumanCivilisation->CityLabelMaterial
				? GameStateRef->HumanCivilisation->CityLabelMaterial
				: GameStateRef->HumanCivilisation->BorderMaterial;
			CivilisationThemeColor = GameStateRef->HumanCivilisation->ThemeColor;
			bOverrideCityScale = GameStateRef->HumanCivilisation->bOverrideCityMeshScale;
			CityScale = GameStateRef->HumanCivilisation->CityMeshScaleOverride;
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
		const int32 ApproxDistance =
			FMath::Abs(City.CenterTile.X - TileCoord.X) +
			FMath::Abs(City.CenterTile.Y - TileCoord.Y);

		if (ApproxDistance < 3)
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

void UConquestCityManager::AutoAssignWorkedTiles(FCityState& City)
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
	City.WorkedTiles = City.OwnedTiles;

	for (const FIntPoint& Coord : City.WorkedTiles)
	{
		if (FHexTileData* WorkedTile = GridModel->GetTileMutable(Coord))
		{
			WorkedTile->Gameplay.bIsWorked = true;
			WorkedTile->Gameplay.WorkedByCityId = City.CityId;
		}
	}
}

void UConquestCityManager::RecalculateCityYields(FCityState& City)
{
	if (!GameStateRef || !GameStateRef->YieldManager)
	{
		return;
	}

	City.CachedYieldPerTurn = GameStateRef->YieldManager->CalculateCityTotalYields(City);
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

		GameStateRef->YieldManager->RecalculateEmpireYieldPerTurn(PlayerId);
	}
}

void UConquestCityManager::UpdateOwnedTileVisuals(int32 PlayerId)
{
	if (!GameStateRef || !GameStateRef->ActiveGridActor)
	{
		return;
	}

	UMaterialInterface* BorderMaterial = nullptr;
	UMaterialInterface* BorderFillMaterial = nullptr;
	if (GameStateRef->HumanPlayer.PlayerId == PlayerId && GameStateRef->HumanCivilisation)
	{
		BorderMaterial = GameStateRef->HumanCivilisation->BorderMaterial;
		BorderFillMaterial = GameStateRef->HumanCivilisation->BorderFillMaterial;
	}

	TArray<FIntPoint> PlayerOwnedTiles;
	for (const FCityState& City : Cities)
	{
		if (City.OwnerPlayerId != PlayerId)
		{
			continue;
		}

		for (const FIntPoint& Coord : City.OwnedTiles)
		{
			PlayerOwnedTiles.AddUnique(Coord);
		}
	}

	GameStateRef->ActiveGridActor->RebuildCivilisationBordersForTiles(PlayerOwnedTiles, BorderMaterial, BorderFillMaterial);
}

void UConquestCityManager::UpdateCityWorldLabel(const FCityState& City)
{
	if (!GameStateRef || !GameStateRef->ActiveGridActor)
	{
		return;
	}

	UMaterialInterface* CivilisationThemeMaterial = nullptr;
	FLinearColor CivilisationThemeColor = FLinearColor::White;
	if (GameStateRef->HumanPlayer.PlayerId == City.OwnerPlayerId && GameStateRef->HumanCivilisation)
	{
		CivilisationThemeMaterial = GameStateRef->HumanCivilisation->CityLabelMaterial
			? GameStateRef->HumanCivilisation->CityLabelMaterial
			: GameStateRef->HumanCivilisation->BorderMaterial;
		CivilisationThemeColor = GameStateRef->HumanCivilisation->ThemeColor;
	}

	GameStateRef->ActiveGridActor->AddOrUpdateCityWorldLabel(
		City.CityId,
		City.CenterTile,
		City.CityName,
		City.Population,
		CivilisationThemeMaterial,
		CivilisationThemeColor
	);
}

FName UConquestCityManager::ResolveCityName(int32 PlayerId, FName RequestedCityName) const
{
	if (!RequestedCityName.IsNone())
	{
		return RequestedCityName;
	}

	const TArray<FName>* CivilisationCityNames = nullptr;
	if (
		GameStateRef &&
		GameStateRef->HumanPlayer.PlayerId == PlayerId &&
		GameStateRef->HumanCivilisation &&
		GameStateRef->HumanCivilisation->CityNames.Num() > 0
	)
	{
		CivilisationCityNames = &GameStateRef->HumanCivilisation->CityNames;
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
		RecalculateCityYields(City);
		ProcessCityGrowth(City);
		ProcessCityProduction(City);
		RecalculateCityYields(City);
		UpdateCityWorldLabel(City);

		OnCityChanged.Broadcast(City.CityId);
	}

	if (GameStateRef)
	{
		RecalculateEmpireYields(PlayerId);
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
	if (!City || City->PendingBorderExpansions <= 0 || !IsValidExpansionTileForCity(*City, Coord))
	{
		return false;
	}

	if (!ClaimTileForCity(*City, Coord))
	{
		return false;
	}

	City->PendingBorderExpansions = FMath::Max(0, City->PendingBorderExpansions - 1);
	AutoAssignWorkedTiles(*City);
	RecalculateCityYields(*City);
	RecalculateEmpireYields(City->OwnerPlayerId);
	UpdateOwnedTileVisuals(City->OwnerPlayerId);

	OnCityChanged.Broadcast(City->CityId);

	if (GameStateRef)
	{
		GameStateRef->BroadcastStateChanged();
	}

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

	City->CurrentProduction.Type = ECityProductionType::Building;
	City->CurrentProduction.ProductionId = BuildingRow->BuildingId;
	City->CurrentProduction.Progress = 0.0f;
	City->CurrentProduction.Cost = BuildingRow->ProductionCost;
	City->bNeedsProductionChoice = false;

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

	const FConquestPlayerEmpireState& Player = GameStateRef->GetHumanPlayer();

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

	const float RemainingCost = BuildingRow->ProductionCost;
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
