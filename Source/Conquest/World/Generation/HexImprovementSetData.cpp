#include "HexImprovementSetData.h"

const FHexImprovementDefinition* UHexImprovementSetData::FindImprovement(FName ImprovementId) const
{
	if (ImprovementId.IsNone())
	{
		return nullptr;
	}

	return Improvements.FindByPredicate([ImprovementId](const FHexImprovementDefinition& Improvement)
	{
		return Improvement.ImprovementId == ImprovementId;
	});
}

void UHexImprovementSetData::GetPossibleImprovementIdsForTile(const FHexTileData& Tile, bool bTileIsWater, TArray<FName>& OutImprovementIds) const
{
	OutImprovementIds.Reset();

	for (const FHexImprovementDefinition& Improvement : Improvements)
	{
		if (Improvement.IsValidForTile(Tile, bTileIsWater))
		{
			OutImprovementIds.Add(Improvement.ImprovementId);
		}
	}
}

void UHexImprovementSetData::GetPossibleImprovementsForTile(const FHexTileData& Tile, bool bTileIsWater, TArray<const FHexImprovementDefinition*>& OutImprovements) const
{
	OutImprovements.Reset();

	for (const FHexImprovementDefinition& Improvement : Improvements)
	{
		if (Improvement.IsValidForTile(Tile, bTileIsWater))
		{
			OutImprovements.Add(&Improvement);
		}
	}
}
