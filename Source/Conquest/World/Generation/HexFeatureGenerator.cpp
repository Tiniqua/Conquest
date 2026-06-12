#include "HexFeatureGenerator.h"

void FHexFeatureGenerator::Generate(
	FHexGridModel& InModel,
	const UHexTileResourceData* InTileData,
	const FHexFeatureGenerationSettings& InSettings,
	int32 RandomSeed
)
{
	Model = &InModel;
	TileData = InTileData;
	Settings = InSettings;

	if (!Model || !InTileData || !Settings.bGenerateFeatures)
	{
		return;
	}

	ClearGeneratedFeatures();

	FRandomStream RandomStream(RandomSeed + 48119);

	for (const FHexFeatureDefinition& FeatureDefinition : InTileData->FeatureDefinitions)
	{
		if (!FeatureDefinition.bGenerateFeature || FeatureDefinition.FeatureType == EHexFeatureType::None)
		{
			continue;
		}

		const int32 TargetCount = ResolveTargetCount(FeatureDefinition);
		PlaceFeatureType(FeatureDefinition, TargetCount, RandomStream);
	}
}

void FHexFeatureGenerator::PlaceFeatureType(
	const FHexFeatureDefinition& FeatureDefinition,
	int32 TargetCount,
	FRandomStream& RandomStream
)
{
	if (!Model || TargetCount <= 0)
	{
		return;
	}

	int32 PlacedCount = 0;
	int32 Attempts = 0;
	const int32 MaxAttempts = FMath::Max(100, TargetCount * Settings.MaxPlacementAttemptsMultiplier);

	while (PlacedCount < TargetCount && Attempts < MaxAttempts)
	{
		++Attempts;

		const int32 SeedTileIndex = PickBestFeatureSeedTile(FeatureDefinition, RandomStream);
		if (SeedTileIndex == INDEX_NONE)
		{
			break;
		}

		int32 TargetClumpSize = GetClumpSizeForPattern(FeatureDefinition, RandomStream);
		TargetClumpSize = FMath::Min(TargetClumpSize, TargetCount - PlacedCount);

		const int32 NewPlacements = GrowFeatureClump(
			FeatureDefinition,
			SeedTileIndex,
			TargetClumpSize,
			RandomStream
		);

		PlacedCount += NewPlacements;
	}
}

int32 FHexFeatureGenerator::GetClumpSizeForPattern(
	const FHexFeatureDefinition& FeatureDefinition,
	FRandomStream& RandomStream
) const
{
	switch (FeatureDefinition.PlacementPattern)
	{
	case EHexFeaturePlacementPattern::RandomSparse:
		return 1;

	case EHexFeaturePlacementPattern::RandomIntermittent:
		return RandomStream.FRand() < 0.75f
			? 1
			: RandomStream.RandRange(2, 4);

	case EHexFeaturePlacementPattern::SmallClumps:
		return RandomStream.RandRange(
			FMath::Max(1, FeatureDefinition.MinClumpSize),
			FMath::Max(FeatureDefinition.MinClumpSize, FeatureDefinition.MaxClumpSize)
		);

	case EHexFeaturePlacementPattern::BigClumps:
		return RandomStream.RandRange(
			FMath::Max(4, FeatureDefinition.MinClumpSize),
			FMath::Max(FMath::Max(4, FeatureDefinition.MinClumpSize), FeatureDefinition.MaxClumpSize)
		);

	default:
		return 1;
	}
}

float FHexFeatureGenerator::ScoreTileForFeature(
	const FHexFeatureDefinition& FeatureDefinition,
	int32 TileIndex
) const
{
	if (!Model || !CanPlaceFeatureOnTile(FeatureDefinition, TileIndex))
	{
		return -100000.0f;
	}

	float Score = FMath::Max(0.01f, FeatureDefinition.PlacementWeight);

	const int32 SameAdjacentCount = CountAdjacentSameFeature(
		FeatureDefinition.FeatureType,
		TileIndex
	);

	Score += static_cast<float>(SameAdjacentCount) * FeatureDefinition.SameFeatureAdjacencyBias;

	if (SameAdjacentCount <= 0)
	{
		Score -= FeatureDefinition.IsolationPenalty;
	}

	return Score;
}

void FHexFeatureGenerator::ClearGeneratedFeatures()
{
	if (!Model)
	{
		return;
	}

	for (FHexTileData& Tile : Model->GetMutableTiles())
	{
		Tile.Features.Reset();
	}
}

int32 FHexFeatureGenerator::ResolveTargetCount(
	const FHexFeatureDefinition& FeatureDefinition
) const
{
	if (!Model)
	{
		return 0;
	}

	const TArray<FHexTileData>& Tiles = Model->GetTiles();

	int32 ValidTileCount = 0;
	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		if (CanPlaceFeatureOnTile(FeatureDefinition, TileIndex))
		{
			++ValidTileCount;
		}
	}

	if (ValidTileCount <= 0)
	{
		return 0;
	}

	if (Settings.TotalFeatureCount > 0)
	{
		return FMath::RoundToInt(
			static_cast<float>(Settings.TotalFeatureCount) * FeatureDefinition.PlacementWeight
		);
	}

	return FMath::RoundToInt(
		static_cast<float>(ValidTileCount) * FeatureDefinition.AutoDensity
	);
}

bool FHexFeatureGenerator::CanPlaceFeatureOnTile(
	const FHexFeatureDefinition& FeatureDefinition,
	int32 TileIndex
) const
{
	if (!Model)
	{
		return false;
	}

	const TArray<FHexTileData>& Tiles = Model->GetTiles();
	if (!Tiles.IsValidIndex(TileIndex))
	{
		return false;
	}

	const FHexTileData& Tile = Tiles[TileIndex];

	return FeatureDefinition.IsValidForTile(Tile);
}

bool FHexFeatureGenerator::PlaceFeatureOnTile(
	const FHexFeatureDefinition& FeatureDefinition,
	int32 TileIndex
)
{
	if (!Model || !CanPlaceFeatureOnTile(FeatureDefinition, TileIndex))
	{
		return false;
	}

	TArray<FHexTileData>& Tiles = Model->GetMutableTiles();
	if (!Tiles.IsValidIndex(TileIndex))
	{
		return false;
	}

	FHexTileData& Tile = Tiles[TileIndex];

	if (!Tile.Features.Contains(FeatureDefinition.FeatureType))
	{
		Tile.Features.Add(FeatureDefinition.FeatureType);
	}

	if (FeatureDefinition.bCountsAsFreshWater)
	{
		Tile.bHasFreshWater = true;
	}

	return true;
}

int32 FHexFeatureGenerator::GrowFeatureClump(
	const FHexFeatureDefinition& FeatureDefinition,
	int32 SeedTileIndex,
	int32 TargetSize,
	FRandomStream& RandomStream
)
{
	if (!Model || TargetSize <= 0)
	{
		return 0;
	}

	if (!CanPlaceFeatureOnTile(FeatureDefinition, SeedTileIndex))
	{
		return 0;
	}

	TArray<int32> Frontier;
	Frontier.Add(SeedTileIndex);

	int32 PlacedCount = 0;
	int32 Safety = 0;
	const int32 MaxSafety = FMath::Max(100, TargetSize * 20);

	while (Frontier.Num() > 0 && PlacedCount < TargetSize && Safety < MaxSafety)
	{
		++Safety;

		const int32 FrontierArrayIndex = PickBestFrontierTile(
			FeatureDefinition,
			Frontier,
			RandomStream
		);

		if (!Frontier.IsValidIndex(FrontierArrayIndex))
		{
			break;
		}

		const int32 TileIndex = Frontier[FrontierArrayIndex];
		Frontier.RemoveAtSwap(FrontierArrayIndex);

		if (!PlaceFeatureOnTile(FeatureDefinition, TileIndex))
		{
			continue;
		}

		++PlacedCount;

		AddValidNeighboursToFrontier(
			FeatureDefinition,
			TileIndex,
			Frontier
		);
	}

	return PlacedCount;
}

int32 FHexFeatureGenerator::PickBestFeatureSeedTile(
	const FHexFeatureDefinition& FeatureDefinition,
	FRandomStream& RandomStream
) const
{
	if (!Model)
	{
		return INDEX_NONE;
	}

	const TArray<FHexTileData>& Tiles = Model->GetTiles();
	if (Tiles.Num() <= 0)
	{
		return INDEX_NONE;
	}

	float BestScore = -100000.0f;
	int32 BestTileIndex = INDEX_NONE;

	const int32 Attempts = FMath::Max(1, Settings.SeedSelectionAttempts);

	for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
	{
		const int32 CandidateIndex = RandomStream.RandRange(0, Tiles.Num() - 1);

		const float Score = ScoreTileForFeature(
			FeatureDefinition,
			CandidateIndex
		);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTileIndex = CandidateIndex;
		}
	}

	if (BestTileIndex != INDEX_NONE && BestScore > -99999.0f)
	{
		return BestTileIndex;
	}

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		if (CanPlaceFeatureOnTile(FeatureDefinition, TileIndex))
		{
			return TileIndex;
		}
	}

	return INDEX_NONE;
}

int32 FHexFeatureGenerator::PickBestFrontierTile(
	const FHexFeatureDefinition& FeatureDefinition,
	const TArray<int32>& Frontier,
	FRandomStream& RandomStream
) const
{
	if (!Model || Frontier.Num() <= 0)
	{
		return INDEX_NONE;
	}

	TArray<float> Scores;
	Scores.SetNum(Frontier.Num());

	float TotalScore = 0.0f;

	for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
	{
		const int32 TileIndex = Frontier[FrontierIndex];

		float Score = ScoreTileForFeature(
			FeatureDefinition,
			TileIndex
		);

		if (Score <= -99999.0f)
		{
			Scores[FrontierIndex] = 0.0f;
			continue;
		}

		const float RandomScale = RandomStream.FRandRange(
			1.0f - FeatureDefinition.Randomness,
			1.0f + FeatureDefinition.Randomness
		);

		Score = FMath::Max(0.01f, Score * RandomScale);

		Scores[FrontierIndex] = Score;
		TotalScore += Score;
	}

	if (TotalScore <= 0.0f)
	{
		return INDEX_NONE;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalScore);

	for (int32 ScoreIndex = 0; ScoreIndex < Scores.Num(); ++ScoreIndex)
	{
		Roll -= Scores[ScoreIndex];

		if (Roll <= 0.0f)
		{
			return ScoreIndex;
		}
	}

	return Frontier.Num() - 1;
}

int32 FHexFeatureGenerator::CountAdjacentSameFeature(
	EHexFeatureType FeatureType,
	int32 TileIndex
) const
{
	if (!Model || !Model->GetTiles().IsValidIndex(TileIndex))
	{
		return 0;
	}

	int32 Q = 0;
	int32 R = 0;
	if (!Model->GetCoordFromIndex(TileIndex, Q, R))
	{
		return 0;
	}

	const TArray<FHexTileData>& Tiles = Model->GetTiles();

	int32 Count = 0;

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NQ = 0;
		int32 NR = 0;

		if (!Model->GetNeighbourCoord(Q, R, Direction, NQ, NR))
		{
			continue;
		}

		const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
		if (!Tiles.IsValidIndex(NeighbourIndex))
		{
			continue;
		}

		if (Tiles[NeighbourIndex].Features.Contains(FeatureType))
		{
			++Count;
		}
	}

	return Count;
}

void FHexFeatureGenerator::AddValidNeighboursToFrontier(
	const FHexFeatureDefinition& FeatureDefinition,
	int32 TileIndex,
	TArray<int32>& Frontier
) const
{
	if (!Model)
	{
		return;
	}

	int32 Q = 0;
	int32 R = 0;
	if (!Model->GetCoordFromIndex(TileIndex, Q, R))
	{
		return;
	}

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NQ = 0;
		int32 NR = 0;

		if (!Model->GetNeighbourCoord(Q, R, Direction, NQ, NR))
		{
			continue;
		}

		const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);

		if (!CanPlaceFeatureOnTile(FeatureDefinition, NeighbourIndex))
		{
			continue;
		}

		Frontier.AddUnique(NeighbourIndex);
	}
}