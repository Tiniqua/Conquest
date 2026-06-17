#include "HexResourceGenerator.h"

void FHexResourceGenerator::Generate(FHexGridModel& InModel, const UHexResourceSetData* InResourceSet, const FHexResourceGenerationSettings& InSettings, int32 RandomSeed)
{
	Model = &InModel;
	ResourceSet = InResourceSet;
	Settings = InSettings;

	if (!Model || !InResourceSet || !Settings.bGenerateResources)
	{
		return;
	}

	for (FHexTileData& Tile : Model->GetMutableTiles())
	{
		Tile.Resource = FHexTileResourceInstance();
	}

	FRandomStream RandomStream(RandomSeed + 9173);
	PlaceCategory(EHexResourceCategory::Luxury, ResolveTargetCount(EHexResourceCategory::Luxury), RandomStream);
	PlaceCategory(EHexResourceCategory::Strategic, ResolveTargetCount(EHexResourceCategory::Strategic), RandomStream);
	PlaceCategory(EHexResourceCategory::Bonus, ResolveTargetCount(EHexResourceCategory::Bonus), RandomStream);
}

void FHexResourceGenerator::PlaceCategory(EHexResourceCategory Category, int32 TargetCount, FRandomStream& RandomStream)
{
	const UHexResourceSetData* Data = ResourceSet.Get();
	if (!Model || !Data || TargetCount <= 0)
	{
		return;
	}

	TArray<const FHexResourceDefinition*> Resources;
	Data->GetResourcesForCategory(Category, Resources);
	if (Resources.Num() <= 0)
	{
		return;
	}

	TArray<int32> CandidateTileIndices;
	const TArray<FHexTileData>& Tiles = Model->GetTiles();
	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		if (!Tiles[TileIndex].Resource.HasResource())
		{
			CandidateTileIndices.Add(TileIndex);
		}
	}

	int32 PlacedCount = 0;
	while (CandidateTileIndices.Num() > 0 && PlacedCount < TargetCount)
	{
		const int32 CandidateArrayIndex = RandomStream.RandRange(0, CandidateTileIndices.Num() - 1);
		const int32 TileIndex = CandidateTileIndices[CandidateArrayIndex];
		CandidateTileIndices.RemoveAtSwap(CandidateArrayIndex);

		if (!Model->GetMutableTiles().IsValidIndex(TileIndex))
		{
			continue;
		}

		FHexTileData& Tile = Model->GetMutableTiles()[TileIndex];
		if (IsTooCloseToExistingResource(TileIndex))
		{
			continue;
		}

		const FHexResourceDefinition* Resource = PickWeightedResourceForTile(Resources, Tile, RandomStream);
		if (!Resource)
		{
			continue;
		}

		Tile.Resource.ResourceId = Resource->ResourceId;
		Tile.Resource.Quantity = RollQuantity(*Resource, RandomStream);
		++PlacedCount;
	}
}

int32 FHexResourceGenerator::ResolveTargetCount(EHexResourceCategory Category) const
{
	const int32 TotalTiles = Model ? Model->GetTiles().Num() : 0;
	if (TotalTiles <= 0)
	{
		return 0;
	}

	switch (Category)
	{
	case EHexResourceCategory::Bonus:
		return Settings.BonusResourceCount > 0 ? Settings.BonusResourceCount : FMath::RoundToInt(static_cast<float>(TotalTiles) * Settings.AutoBonusDensity);
	case EHexResourceCategory::Luxury:
		return Settings.LuxuryResourceCount > 0 ? Settings.LuxuryResourceCount : FMath::RoundToInt(static_cast<float>(TotalTiles) * Settings.AutoLuxuryDensity);
	case EHexResourceCategory::Strategic:
		return Settings.StrategicResourceCount > 0 ? Settings.StrategicResourceCount : FMath::RoundToInt(static_cast<float>(TotalTiles) * Settings.AutoStrategicDensity);
	default:
		return 0;
	}
}

const FHexResourceDefinition* FHexResourceGenerator::PickWeightedResourceForTile(const TArray<const FHexResourceDefinition*>& Resources, const FHexTileData& Tile, FRandomStream& RandomStream) const
{
	float TotalWeight = 0.0f;
	for (const FHexResourceDefinition* Resource : Resources)
	{
		if (Resource && Resource->IsValidForTile(Tile))
		{
			TotalWeight += FMath::Max(0.0f, Resource->PlacementWeight);
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	for (const FHexResourceDefinition* Resource : Resources)
	{
		if (!Resource || !Resource->IsValidForTile(Tile))
		{
			continue;
		}

		Roll -= FMath::Max(0.0f, Resource->PlacementWeight);
		if (Roll <= 0.0f)
		{
			return Resource;
		}
	}

	return nullptr;
}

int32 FHexResourceGenerator::RollQuantity(const FHexResourceDefinition& Resource, FRandomStream& RandomStream) const
{
	if (Resource.Category != EHexResourceCategory::Strategic)
	{
		return 0;
	}

	const int32 MinQuantity = FMath::Max(0, Resource.MinStrategicQuantity);
	const int32 MaxQuantity = FMath::Max(MinQuantity, Resource.MaxStrategicQuantity);
	return RandomStream.RandRange(MinQuantity, MaxQuantity);
}

bool FHexResourceGenerator::IsTooCloseToExistingResource(int32 TileIndex) const
{
	const int32 Spacing = FMath::Max(0, Settings.ResourceMinSpacing);
	if (!Model || Spacing <= 0)
	{
		return false;
	}

	int32 Q = 0;
	int32 R = 0;
	if (!Model->GetCoordFromIndex(TileIndex, Q, R))
	{
		return false;
	}

	const TArray<FIntPoint> NearbyCoords = Model->GetCoordsInRange(FIntPoint(Q, R), Spacing);
	const TArray<FHexTileData>& Tiles = Model->GetTiles();
	for (const FIntPoint& Coord : NearbyCoords)
	{
		const int32 NearbyIndex = Model->GetTileIndex(Coord.X, Coord.Y);
		if (NearbyIndex == TileIndex)
		{
			continue;
		}

		if (Tiles.IsValidIndex(NearbyIndex) && Tiles[NearbyIndex].Resource.HasResource())
		{
			return true;
		}
	}

	return false;
}
