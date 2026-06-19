
#include "ConquestContentManager.h"

#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Philosophies/ConquestPhilosophyTypes.h"
#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Tech/ConquestTechTypes.h"
#include "Conquest/Units/ConquestUnitTypes.h"

void UConquestContentManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

const FConquestBuildingRow* UConquestContentManager::FindBuilding(FName BuildingId) const
{
	if (!GameStateRef || !GameStateRef->BuildingTable || BuildingId.IsNone())
	{
		return nullptr;
	}

	return GameStateRef->BuildingTable->FindRow<FConquestBuildingRow>(
		BuildingId,
		TEXT("FindBuilding")
	);
}

const FConquestTechRow* UConquestContentManager::FindTech(FName TechId) const
{
	if (!GameStateRef || !GameStateRef->TechTable || TechId.IsNone())
	{
		return nullptr;
	}

	return GameStateRef->TechTable->FindRow<FConquestTechRow>(
		TechId,
		TEXT("FindTech")
	);
}

const FConquestUnitRow* UConquestContentManager::FindUnit(FName UnitId) const
{
	if (!GameStateRef || !GameStateRef->UnitTable || UnitId.IsNone())
	{
		return nullptr;
	}

	return GameStateRef->UnitTable->FindRow<FConquestUnitRow>(
		UnitId,
		TEXT("FindUnit")
	);
}

const FConquestUnitAugmentRow* UConquestContentManager::FindUnitAugment(FName AugmentId) const
{
	if (!GameStateRef || !GameStateRef->UnitAugmentTable || AugmentId.IsNone())
	{
		return nullptr;
	}

	return GameStateRef->UnitAugmentTable->FindRow<FConquestUnitAugmentRow>(
		AugmentId,
		TEXT("FindUnitAugment")
	);
}

const FConquestPhilosophyRow* UConquestContentManager::FindPhilosophy(FName PhilosophyId) const
{
	if (!GameStateRef || !GameStateRef->PhilosophyTable || PhilosophyId.IsNone())
	{
		return nullptr;
	}

	return GameStateRef->PhilosophyTable->FindRow<FConquestPhilosophyRow>(
		PhilosophyId,
		TEXT("FindPhilosophy")
	);
}

FName UConquestContentManager::ResolveBuildingIdForPlayer(int32 PlayerId, FName BaseBuildingId) const
{
	if (!GameStateRef || !GameStateRef->HumanCivilisation)
	{
		return BaseBuildingId;
	}

	// Single-player for now. Later use PlayerId to get the correct player's civ.
	for (const FConquestContentOverride& Override : GameStateRef->HumanCivilisation->BuildingOverrides)
	{
		if (Override.ReplacesId == BaseBuildingId && !Override.ReplacementId.IsNone())
		{
			return Override.ReplacementId;
		}
	}

	return BaseBuildingId;
}

FName UConquestContentManager::ResolveUnitIdForPlayer(int32 PlayerId, FName BaseUnitId) const
{
	if (!GameStateRef || !GameStateRef->HumanCivilisation)
	{
		return BaseUnitId;
	}

	for (const FConquestContentOverride& Override : GameStateRef->HumanCivilisation->UnitOverrides)
	{
		if (Override.ReplacesId == BaseUnitId && !Override.ReplacementId.IsNone())
		{
			return Override.ReplacementId;
		}
	}

	return BaseUnitId;
}

const FConquestBuildingRow* UConquestContentManager::ResolveBuildingForPlayer(int32 PlayerId, FName BaseBuildingId) const
{
	const FName ResolvedId = ResolveBuildingIdForPlayer(PlayerId, BaseBuildingId);
	return FindBuilding(ResolvedId);
}

const FConquestUnitRow* UConquestContentManager::ResolveUnitForPlayer(int32 PlayerId, FName BaseUnitId) const
{
	const FName ResolvedId = ResolveUnitIdForPlayer(PlayerId, BaseUnitId);
	return FindUnit(ResolvedId);
}

void UConquestContentManager::GetAllBaseBuildings(TArray<const FConquestBuildingRow*>& OutRows) const
{
	OutRows.Reset();

	if (!GameStateRef || !GameStateRef->BuildingTable)
	{
		return;
	}

	TArray<FConquestBuildingRow*> AllRows;
	GameStateRef->BuildingTable->GetAllRows<FConquestBuildingRow>(
		TEXT("GetAllBaseBuildings"),
		AllRows
	);

	for (const FConquestBuildingRow* Row : AllRows)
	{
		if (!Row)
		{
			continue;
		}

		// Important: only expose base buildings in generic lists.
		// Unique replacements appear only through ResolveBuildingForPlayer.
		if (!Row->ReplacesBuildingId.IsNone())
		{
			continue;
		}

		OutRows.Add(Row);
	}
}

void UConquestContentManager::GetStartingBuildingIdsForPlayer(int32 PlayerId, TArray<FName>& OutBuildingIds) const
{
	OutBuildingIds.Reset();

	TArray<const FConquestBuildingRow*> BaseBuildings;
	GetAllBaseBuildings(BaseBuildings);

	for (const FConquestBuildingRow* BaseRow : BaseBuildings)
	{
		if (!BaseRow || !BaseRow->bGrantedOnCityFounding)
		{
			continue;
		}

		const FName ResolvedBuildingId = ResolveBuildingIdForPlayer(PlayerId, BaseRow->BuildingId);
		if (!ResolvedBuildingId.IsNone())
		{
			OutBuildingIds.AddUnique(ResolvedBuildingId);
		}
	}
}

void UConquestContentManager::GetAllBaseUnits(TArray<const FConquestUnitRow*>& OutRows) const
{
	OutRows.Reset();

	if (!GameStateRef || !GameStateRef->UnitTable)
	{
		return;
	}

	TArray<FConquestUnitRow*> AllRows;
	GameStateRef->UnitTable->GetAllRows<FConquestUnitRow>(
		TEXT("GetAllBaseUnits"),
		AllRows
	);

	for (const FConquestUnitRow* Row : AllRows)
	{
		if (!Row)
		{
			continue;
		}

		if (!Row->ReplacesUnitId.IsNone())
		{
			continue;
		}

		OutRows.Add(Row);
	}
}

void UConquestContentManager::GetAllTechs(TArray<const FConquestTechRow*>& OutRows) const
{
	OutRows.Reset();

	if (!GameStateRef || !GameStateRef->TechTable)
	{
		return;
	}

	TArray<FConquestTechRow*> AllRows;
	GameStateRef->TechTable->GetAllRows<FConquestTechRow>(
		TEXT("GetAllTechs"),
		AllRows
	);

	for (const FConquestTechRow* Row : AllRows)
	{
		if (Row)
		{
			OutRows.Add(Row);
		}
	}
}

void UConquestContentManager::GetAllUnitAugments(TArray<const FConquestUnitAugmentRow*>& OutRows) const
{
	OutRows.Reset();

	if (!GameStateRef || !GameStateRef->UnitAugmentTable)
	{
		return;
	}

	TArray<FConquestUnitAugmentRow*> AllRows;
	GameStateRef->UnitAugmentTable->GetAllRows<FConquestUnitAugmentRow>(
		TEXT("GetAllUnitAugments"),
		AllRows
	);

	for (const FConquestUnitAugmentRow* Row : AllRows)
	{
		if (Row)
		{
			OutRows.Add(Row);
		}
	}
}
