#include "HexMapGenerator.h"

void FHexMapGenerator::Generate(FHexGridModel& InModel, const FHexGenerationSettings& InGenerationSettings)
{
	Model = &InModel;
	Settings = InGenerationSettings;

	TArray<FHexTileData>& Tiles = Model->GetMutableTiles();
	const int32 TotalTiles = Model->GetGridWidth() * Model->GetGridHeight();

	Tiles.Reset();
	Tiles.SetNum(TotalTiles);

	if (TotalTiles <= 0)
	{
		return;
	}

	if (Settings.GenerationRules.Num() <= 0)
	{
		SetupDefaultGenerationRules(Settings.GenerationRules);
	}

	TArray<bool> Assigned;
	Assigned.Init(false, TotalTiles);

	FRandomStream RandomStream(Settings.RandomSeed);
	TMap<EHexTileType, int32> DesiredCounts;
	BuildDesiredTileCounts(RandomStream, DesiredCounts);

	for (const FHexTileGenerationRule& Rule : Settings.GenerationRules)
	{
		int32 RemainingForType = DesiredCounts.FindRef(Rule.TileType);
		int32 FailedClumpAttempts = 0;

		while (RemainingForType > 0)
		{
			const int32 MinClump = FMath::Max(1, Rule.MinClumpSize);
			const int32 MaxClump = FMath::Max(MinClump, Rule.MaxClumpSize);
			const int32 TargetClumpSize = FMath::Min(RemainingForType, RandomStream.RandRange(MinClump, MaxClump));

			int32 SeedQ = 0;
			int32 SeedR = 0;
			if (!FindSeedTileForRule(Rule, RandomStream, Assigned, SeedQ, SeedR))
			{
				break;
			}

			const int32 PlacedCount = GrowClump(Rule, SeedQ, SeedR, TargetClumpSize, RandomStream, Assigned);
			if (PlacedCount <= 0)
			{
				++FailedClumpAttempts;
				if (FailedClumpAttempts >= 12) { break; }
				continue;
			}

			RemainingForType -= PlacedCount;
			FailedClumpAttempts = PlacedCount < TargetClumpSize ? FailedClumpAttempts + 1 : 0;
			if (FailedClumpAttempts >= 12) { break; }
		}
	}

	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		if (!Assigned[Index])
		{
			Tiles[Index].TileType = PickWeightedLandTileType(RandomStream);
			Assigned[Index] = true;
		}
	}
}

void FHexMapGenerator::SetupDefaultGenerationRules(TArray<FHexTileGenerationRule>& OutRules)
{
	OutRules.Reset();

	auto AddRule = [&OutRules](EHexTileType Type, float Weight, int32 Min, int32 Max, TArray<EHexTileType> Preferred, TArray<EHexTileType> Avoid, TArray<EHexTileType> Required, bool bSoft, bool bReject, float MinScore, float Height)
	{
		FHexTileGenerationRule Rule;
		Rule.TileType = Type;
		Rule.Weight = Weight;
		Rule.MinClumpSize = Min;
		Rule.MaxClumpSize = Max;
		Rule.PreferredAdjacentTypes = Preferred;
		Rule.AvoidAdjacentTypes = Avoid;
		Rule.RequiredAdjacentTypes = Required;
		Rule.bSoftCount = bSoft;
		Rule.bRejectBadAdjacency = bReject;
		Rule.MinPlacementScore = MinScore;
		Rule.HeightOffset = Height;
		OutRules.Add(Rule);
	};

	AddRule(EHexTileType::Ocean, 8.0f, 10, 28,
		{ EHexTileType::Ocean, EHexTileType::Coast },
		{ EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Desert, EHexTileType::Tundra, EHexTileType::Snow, EHexTileType::Mountain, EHexTileType::Lake },
		{}, true, true, 1.0f, -2.0f);

	AddRule(EHexTileType::Coast, 5.0f, 3, 10,
		{ EHexTileType::Ocean, EHexTileType::Grassland, EHexTileType::Plains },
		{ EHexTileType::Mountain, EHexTileType::Snow, EHexTileType::Lake },
		{}, true, true, 2.0f, -1.0f);

	AddRule(EHexTileType::Grassland, 31.0f, 8, 28,
		{ EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Coast, EHexTileType::Lake },
		{ EHexTileType::Ocean }, {}, false, false, 0.5f, 0.5f);

	AddRule(EHexTileType::Plains, 27.0f, 8, 26,
		{ EHexTileType::Plains, EHexTileType::Grassland, EHexTileType::Desert, EHexTileType::Lake },
		{ EHexTileType::Ocean }, {}, false, false, 0.5f, 1.0f);

	AddRule(EHexTileType::Desert, 10.0f, 5, 18,
		{ EHexTileType::Desert, EHexTileType::Plains },
		{ EHexTileType::Ocean, EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Lake }, {}, false, false, 0.5f, 2.0f);

	AddRule(EHexTileType::Tundra, 8.0f, 5, 16,
		{ EHexTileType::Tundra, EHexTileType::Snow, EHexTileType::Plains, EHexTileType::Lake },
		{ EHexTileType::Ocean, EHexTileType::Desert }, {}, false, false, 0.5f, 0.0f);

	AddRule(EHexTileType::Snow, 5.0f, 3, 12,
		{ EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Mountain },
		{ EHexTileType::Ocean, EHexTileType::Desert, EHexTileType::Coast, EHexTileType::Lake }, {}, false, false, 0.5f, 1.0f);

	AddRule(EHexTileType::Lake, 3.0f, 1, 4,
		{ EHexTileType::Lake, EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Tundra },
		{ EHexTileType::Ocean, EHexTileType::Coast },
		{ EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Tundra }, true, true, 1.0f, -1.0f);

	AddRule(EHexTileType::Mountain, 1.5f, 2, 8,
		{ EHexTileType::Mountain, EHexTileType::Snow, EHexTileType::Tundra },
		{ EHexTileType::Ocean, EHexTileType::Coast, EHexTileType::Lake }, {}, true, true, 0.5f, 5.0f);
}

void FHexMapGenerator::BuildDesiredTileCounts(FRandomStream& RandomStream, TMap<EHexTileType, int32>& OutDesiredCounts) const
{
	OutDesiredCounts.Reset();
	const int32 TotalTiles = Model->GetGridWidth() * Model->GetGridHeight();
	if (TotalTiles <= 0 || Settings.GenerationRules.Num() <= 0) { return; }

	TArray<float> AdjustedWeights;
	AdjustedWeights.SetNum(Settings.GenerationRules.Num());
	float TotalAdjustedWeight = 0.0f;

	for (int32 RuleIndex = 0; RuleIndex < Settings.GenerationRules.Num(); ++RuleIndex)
	{
		const FHexTileGenerationRule& Rule = Settings.GenerationRules[RuleIndex];
		const float RandomScale = RandomStream.FRandRange(1.0f - Settings.CountRandomness, 1.0f + Settings.CountRandomness);
		AdjustedWeights[RuleIndex] = FMath::Max(0.0f, Rule.Weight * RandomScale);
		TotalAdjustedWeight += AdjustedWeights[RuleIndex];
	}

	if (TotalAdjustedWeight <= 0.0f)
	{
		OutDesiredCounts.Add(EHexTileType::Grassland, TotalTiles);
		return;
	}

	int32 CountSoFar = 0;
	for (int32 RuleIndex = 0; RuleIndex < Settings.GenerationRules.Num(); ++RuleIndex)
	{
		const FHexTileGenerationRule& Rule = Settings.GenerationRules[RuleIndex];
		int32 Count = RuleIndex == Settings.GenerationRules.Num() - 1
			? TotalTiles - CountSoFar
			: FMath::RoundToInt(static_cast<float>(TotalTiles) * (AdjustedWeights[RuleIndex] / TotalAdjustedWeight));
		Count = FMath::Clamp(Count, 0, TotalTiles - CountSoFar);
		OutDesiredCounts.FindOrAdd(Rule.TileType) += Count;
		CountSoFar += Count;
	}
}

bool FHexMapGenerator::FindSeedTileForRule(const FHexTileGenerationRule& Rule, FRandomStream& RandomStream, const TArray<bool>& Assigned, int32& OutQ, int32& OutR) const
{
	const int32 TotalTiles = Model->GetGridWidth() * Model->GetGridHeight();
	float BestScore = -1.0f;
	int32 BestIndex = INDEX_NONE;

	for (int32 Attempt = 0; Attempt < Settings.SeedSelectionAttempts; ++Attempt)
	{
		const int32 CandidateIndex = RandomStream.RandRange(0, TotalTiles - 1);
		if (!Assigned.IsValidIndex(CandidateIndex) || Assigned[CandidateIndex]) { continue; }
		const int32 CandidateQ = CandidateIndex % Model->GetGridWidth();
		const int32 CandidateR = CandidateIndex / Model->GetGridWidth();
		const float Score = ScoreSeedTileForRule(Rule, CandidateQ, CandidateR, Assigned);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestIndex = CandidateIndex;
		}
	}

	if (BestIndex != INDEX_NONE && (!Rule.bRejectBadAdjacency || BestScore >= Rule.MinPlacementScore))
	{
		OutQ = BestIndex % Model->GetGridWidth();
		OutR = BestIndex / Model->GetGridWidth();
		return true;
	}

	for (int32 Index = 0; Index < Assigned.Num(); ++Index)
	{
		if (Assigned[Index]) { continue; }
		const int32 CandidateQ = Index % Model->GetGridWidth();
		const int32 CandidateR = Index / Model->GetGridWidth();
		if (!DoesTileSatisfyRequiredAdjacency(Rule, CandidateQ, CandidateR, Assigned)) { continue; }
		const float CandidateScore = ScoreSeedTileForRule(Rule, CandidateQ, CandidateR, Assigned);
		if (Rule.bRejectBadAdjacency && CandidateScore < Rule.MinPlacementScore) { continue; }
		OutQ = CandidateQ;
		OutR = CandidateR;
		return true;
	}

	return false;
}

float FHexMapGenerator::ScoreSeedTileForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const
{
	return ScoreTileForRuleAdjacency(Rule, Q, R, Assigned);
}

float FHexMapGenerator::ScoreTileForRuleAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const
{
	if (!DoesTileSatisfyRequiredAdjacency(Rule, Q, R, Assigned)) { return -100000.0f; }
	float Score = 1.0f;

	const TArray<FHexTileData>& Tiles = Model->GetTiles();
	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;
		if (!Model->GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR)) { continue; }
		const int32 NeighbourIndex = Model->GetTileIndex(NeighbourQ, NeighbourR);
		if (!Tiles.IsValidIndex(NeighbourIndex) || !Assigned.IsValidIndex(NeighbourIndex) || !Assigned[NeighbourIndex]) { continue; }
		const EHexTileType NeighbourType = Tiles[NeighbourIndex].TileType;
		if (NeighbourType == Rule.TileType) { Score += Settings.SameTypeAdjacencyBonus; }
		if (Rule.PreferredAdjacentTypes.Contains(NeighbourType)) { Score += Settings.PreferredAdjacencyBonus; }
		if (Rule.AvoidAdjacentTypes.Contains(NeighbourType))
		{
			Score -= Rule.TileType == EHexTileType::Ocean ? Settings.HardAvoidAdjacencyPenalty : Settings.AvoidAdjacencyPenalty;
		}
	}

	return Score;
}

bool FHexMapGenerator::DoesTileSatisfyRequiredAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const
{
	if (Rule.RequiredAdjacentTypes.Num() <= 0) { return true; }
	const TArray<FHexTileData>& Tiles = Model->GetTiles();

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;
		if (!Model->GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR)) { continue; }
		const int32 NeighbourIndex = Model->GetTileIndex(NeighbourQ, NeighbourR);
		if (!Tiles.IsValidIndex(NeighbourIndex) || !Assigned.IsValidIndex(NeighbourIndex) || !Assigned[NeighbourIndex]) { continue; }
		if (Rule.RequiredAdjacentTypes.Contains(Tiles[NeighbourIndex].TileType)) { return true; }
	}

	return false;
}

int32 FHexMapGenerator::GrowClump(const FHexTileGenerationRule& Rule, int32 SeedQ, int32 SeedR, int32 TargetSize, FRandomStream& RandomStream, TArray<bool>& Assigned)
{
	if (!Model->IsValidCoord(SeedQ, SeedR) || TargetSize <= 0) { return 0; }
	TArray<FIntPoint> Frontier;
	Frontier.Add(FIntPoint(SeedQ, SeedR));
	int32 PlacedCount = 0;
	TArray<FHexTileData>& Tiles = Model->GetMutableTiles();

	while (Frontier.Num() > 0 && PlacedCount < TargetSize)
	{
		const int32 ChosenFrontierIndex = PickBestFrontierIndex(Rule, Frontier, RandomStream, Assigned);
		if (!Frontier.IsValidIndex(ChosenFrontierIndex)) { break; }
		const FIntPoint Coord = Frontier[ChosenFrontierIndex];
		Frontier.RemoveAtSwap(ChosenFrontierIndex);
		if (!Model->IsValidCoord(Coord.X, Coord.Y)) { continue; }
		const int32 TileIndex = Model->GetTileIndex(Coord.X, Coord.Y);
		if (!Assigned.IsValidIndex(TileIndex) || Assigned[TileIndex]) { continue; }
		Tiles[TileIndex].TileType = Rule.TileType;
		Assigned[TileIndex] = true;
		++PlacedCount;

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NeighbourQ = 0;
			int32 NeighbourR = 0;
			if (!Model->GetNeighbourCoord(Coord.X, Coord.Y, Direction, NeighbourQ, NeighbourR)) { continue; }
			const int32 NeighbourIndex = Model->GetTileIndex(NeighbourQ, NeighbourR);
			if (Assigned.IsValidIndex(NeighbourIndex) && !Assigned[NeighbourIndex])
			{
				Frontier.AddUnique(FIntPoint(NeighbourQ, NeighbourR));
			}
		}
	}

	return PlacedCount;
}

int32 FHexMapGenerator::PickBestFrontierIndex(const FHexTileGenerationRule& Rule, const TArray<FIntPoint>& Frontier, FRandomStream& RandomStream, const TArray<bool>& Assigned) const
{
	if (Frontier.Num() <= 0) { return INDEX_NONE; }
	TArray<float> Scores;
	Scores.SetNum(Frontier.Num());
	float TotalScore = 0.0f;

	for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
	{
		const FIntPoint Coord = Frontier[FrontierIndex];
		float Score = ScoreTileForRuleAdjacency(Rule, Coord.X, Coord.Y, Assigned);
		Score = (Rule.bRejectBadAdjacency && Score < Rule.MinPlacementScore) ? 0.0f : FMath::Max(0.05f, Score);
		Scores[FrontierIndex] = Score;
		TotalScore += Score;
	}

	if (TotalScore <= 0.0f) { return INDEX_NONE; }
	float Roll = RandomStream.FRandRange(0.0f, TotalScore);
	for (int32 ScoreIndex = 0; ScoreIndex < Scores.Num(); ++ScoreIndex)
	{
		Roll -= Scores[ScoreIndex];
		if (Roll <= 0.0f) { return ScoreIndex; }
	}
	return Frontier.Num() - 1;
}

EHexTileType FHexMapGenerator::PickWeightedLandTileType(FRandomStream& RandomStream) const
{
	float TotalWeight = 0.0f;
	for (const FHexTileGenerationRule& Rule : Settings.GenerationRules)
	{
		if (!Model->IsWaterTileType(Rule.TileType)) { TotalWeight += FMath::Max(0.0f, Rule.Weight); }
	}
	if (TotalWeight <= 0.0f) { return EHexTileType::Grassland; }

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	for (const FHexTileGenerationRule& Rule : Settings.GenerationRules)
	{
		if (Model->IsWaterTileType(Rule.TileType)) { continue; }
		Roll -= FMath::Max(0.0f, Rule.Weight);
		if (Roll <= 0.0f) { return Rule.TileType; }
	}
	return EHexTileType::Grassland;
}
