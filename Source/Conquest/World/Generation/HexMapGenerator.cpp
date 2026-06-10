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

	FRandomStream RandomStream(Settings.RandomSeed);

	TArray<bool> IsLand;
	BuildLandWaterMask(RandomStream, IsLand);

	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		const bool bIsLand = IsLand.IsValidIndex(Index) && IsLand[Index];
		Tiles[Index].TileType = bIsLand ? EHexTileType::Grassland : EHexTileType::Ocean;
	}

	ApplyCoastsFromLandMask(Tiles, IsLand);
	GenerateLandTerrain(RandomStream, IsLand);
	ApplyCoastsFromLandMask(Tiles, IsLand);
}

void FHexMapGenerator::BuildLandWaterMask(FRandomStream& RandomStream, TArray<bool>& OutIsLand) const
{
	const int32 TotalTiles = Model->GetGridWidth() * Model->GetGridHeight();
	OutIsLand.Init(false, TotalTiles);

	if (TotalTiles <= 0)
	{
		return;
	}

	const FHexMapShapeSettings& Shape = Settings.MapShapeSettings;
	const int32 TargetLandTiles = FMath::RoundToInt(static_cast<float>(TotalTiles) * FMath::Clamp(Shape.TargetLandRatio, 0.0f, 1.0f));
	const float IslandBudgetRatio = FMath::Clamp(Shape.IslandChainChance * 0.5f, 0.0f, 0.5f);
	const int32 MajorLandTarget = FMath::Clamp(FMath::RoundToInt(static_cast<float>(TargetLandTiles) * (1.0f - IslandBudgetRatio)), 0, TargetLandTiles);

	SeedMajorLandmasses(RandomStream, OutIsLand, MajorLandTarget);

	if (Shape.IslandChainChance > 0.0f)
	{
		AddIslandChains(RandomStream, OutIsLand, TargetLandTiles);
	}

	if (Shape.bUseInlandSea)
	{
		ApplyInlandSea(OutIsLand);
	}
}

void FHexMapGenerator::SeedMajorLandmasses(FRandomStream& RandomStream, TArray<bool>& InOutIsLand, int32 TargetLandTiles) const
{
	const int32 Width = Model->GetGridWidth();
	const int32 Height = Model->GetGridHeight();
	const int32 TotalTiles = Width * Height;

	if (TotalTiles <= 0 || TargetLandTiles <= 0)
	{
		return;
	}

	const FHexMapShapeSettings& Shape = Settings.MapShapeSettings;
	const int32 LandmassCount = FMath::Max(1, Shape.MajorLandmassCount);
	TArray<FIntPoint> Frontier;

	auto AddRandomSeed = [&]()
	{
		const int32 MinQ = FMath::Clamp(Width / 8, 0, FMath::Max(0, Width - 1));
		const int32 MaxQ = FMath::Clamp(Width - Width / 8 - 1, MinQ, FMath::Max(0, Width - 1));
		const int32 MinR = FMath::Clamp(Height / 8, 0, FMath::Max(0, Height - 1));
		const int32 MaxR = FMath::Clamp(Height - Height / 8 - 1, MinR, FMath::Max(0, Height - 1));

		const int32 SeedQ = RandomStream.RandRange(MinQ, MaxQ);
		const int32 SeedR = RandomStream.RandRange(MinR, MaxR);
		const int32 SeedIndex = Model->GetTileIndex(SeedQ, SeedR);

		if (InOutIsLand.IsValidIndex(SeedIndex) && !InOutIsLand[SeedIndex])
		{
			InOutIsLand[SeedIndex] = true;
			Frontier.Add(FIntPoint(SeedQ, SeedR));
			return true;
		}

		return false;
	};

	for (int32 LandmassIndex = 0; LandmassIndex < LandmassCount; ++LandmassIndex)
	{
		AddRandomSeed();
	}

	int32 CurrentLandTiles = 0;
	for (const bool bIsLand : InOutIsLand)
	{
		if (bIsLand)
		{
			++CurrentLandTiles;
		}
	}

	int32 Safety = 0;
	const int32 MaxSafety = FMath::Max(1000, TargetLandTiles * 50);

	while (CurrentLandTiles < TargetLandTiles && Safety < MaxSafety)
	{
		++Safety;

		if (Frontier.Num() <= 0)
		{
			if (AddRandomSeed())
			{
				++CurrentLandTiles;
			}
			continue;
		}

		const int32 FrontierIndex = RandomStream.RandRange(0, Frontier.Num() - 1);
		const FIntPoint Coord = Frontier[FrontierIndex];

		if (RandomStream.FRand() < Shape.LandmassFragmentation)
		{
			Frontier.RemoveAtSwap(FrontierIndex);
			continue;
		}

		TArray<FIntPoint> Candidates;
		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NQ = 0;
			int32 NR = 0;

			if (!Model->GetNeighbourCoord(Coord.X, Coord.Y, Direction, NQ, NR))
			{
				continue;
			}

			const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
			if (!InOutIsLand.IsValidIndex(NeighbourIndex) || InOutIsLand[NeighbourIndex])
			{
				continue;
			}

			Candidates.Add(FIntPoint(NQ, NR));
		}

		if (Candidates.Num() <= 0)
		{
			Frontier.RemoveAtSwap(FrontierIndex);
			continue;
		}

		const FIntPoint Chosen = Candidates[RandomStream.RandRange(0, Candidates.Num() - 1)];
		const int32 ChosenIndex = Model->GetTileIndex(Chosen.X, Chosen.Y);

		if (InOutIsLand.IsValidIndex(ChosenIndex) && !InOutIsLand[ChosenIndex])
		{
			InOutIsLand[ChosenIndex] = true;
			Frontier.Add(Chosen);
			++CurrentLandTiles;
		}

		if (RandomStream.FRand() > Shape.CoastlineNoise)
		{
			Frontier.RemoveAtSwap(FrontierIndex);
		}
	}
}

void FHexMapGenerator::AddIslandChains(FRandomStream& RandomStream, TArray<bool>& InOutIsLand, int32 TargetLandTiles) const
{
	int32 CurrentLandTiles = 0;
	for (const bool bIsLand : InOutIsLand)
	{
		if (bIsLand)
		{
			++CurrentLandTiles;
		}
	}

	const int32 IslandBudget = FMath::Max(0, TargetLandTiles - CurrentLandTiles);
	if (IslandBudget <= 0)
	{
		return;
	}

	const int32 Width = Model->GetGridWidth();
	const int32 Height = Model->GetGridHeight();
	int32 Placed = 0;
	int32 Attempts = 0;
	const int32 MaxAttempts = FMath::Max(100, IslandBudget * 20);

	while (Placed < IslandBudget && Attempts < MaxAttempts)
	{
		++Attempts;
		int32 Q = RandomStream.RandRange(0, Width - 1);
		int32 R = RandomStream.RandRange(0, Height - 1);
		const int32 ChainLength = RandomStream.RandRange(2, 8);

		for (int32 Step = 0; Step < ChainLength && Placed < IslandBudget; ++Step)
		{
			if (!Model->IsValidCoord(Q, R))
			{
				break;
			}

			const int32 Index = Model->GetTileIndex(Q, R);
			if (InOutIsLand.IsValidIndex(Index) && !InOutIsLand[Index])
			{
				InOutIsLand[Index] = true;
				++Placed;
			}

			const int32 Direction = RandomStream.RandRange(0, 5);
			int32 NQ = 0;
			int32 NR = 0;
			if (!Model->GetNeighbourCoord(Q, R, Direction, NQ, NR))
			{
				break;
			}

			Q = NQ;
			R = NR;
		}
	}
}

void FHexMapGenerator::ApplyInlandSea(TArray<bool>& InOutIsLand) const
{
	const FHexMapShapeSettings& Shape = Settings.MapShapeSettings;
	if (!Shape.bUseInlandSea)
	{
		return;
	}

	const int32 Width = Model->GetGridWidth();
	const int32 Height = Model->GetGridHeight();
	const FVector2D Center(static_cast<float>(Width - 1) * 0.5f, static_cast<float>(Height - 1) * 0.5f);
	const float MaxRadius = FMath::Min(static_cast<float>(Width), static_cast<float>(Height)) * Shape.InlandSeaRadiusRatio;

	for (int32 R = 0; R < Height; ++R)
	{
		for (int32 Q = 0; Q < Width; ++Q)
		{
			const FVector2D Coord(static_cast<float>(Q), static_cast<float>(R));
			if (FVector2D::Distance(Coord, Center) <= MaxRadius)
			{
				const int32 Index = Model->GetTileIndex(Q, R);
				if (InOutIsLand.IsValidIndex(Index))
				{
					InOutIsLand[Index] = false;
				}
			}
		}
	}
}

void FHexMapGenerator::ApplyCoastsFromLandMask(TArray<FHexTileData>& Tiles, const TArray<bool>& IsLand) const
{
	for (int32 R = 0; R < Model->GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < Model->GetGridWidth(); ++Q)
		{
			const int32 Index = Model->GetTileIndex(Q, R);
			if (!Tiles.IsValidIndex(Index) || !IsLand.IsValidIndex(Index))
			{
				continue;
			}

			if (IsLand[Index])
			{
				if (Tiles[Index].TileType == EHexTileType::Ocean || Tiles[Index].TileType == EHexTileType::Coast)
				{
					Tiles[Index].TileType = EHexTileType::Grassland;
				}
				continue;
			}

			bool bAdjacentToLand = false;
			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				int32 NQ = 0;
				int32 NR = 0;
				if (!Model->GetNeighbourCoord(Q, R, Direction, NQ, NR))
				{
					continue;
				}

				const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
				if (IsLand.IsValidIndex(NeighbourIndex) && IsLand[NeighbourIndex])
				{
					bAdjacentToLand = true;
					break;
				}
			}

			Tiles[Index].TileType = bAdjacentToLand ? EHexTileType::Coast : EHexTileType::Ocean;
		}
	}
}

void FHexMapGenerator::GenerateLandTerrain(FRandomStream& RandomStream, const TArray<bool>& IsLand)
{
	TArray<FHexTileData>& Tiles = Model->GetMutableTiles();
	const int32 TotalTiles = Model->GetGridWidth() * Model->GetGridHeight();

	TArray<bool> Assigned;
	Assigned.Init(false, TotalTiles);

	int32 LandTileCount = 0;
	for (int32 Index = 0; Index < TotalTiles; ++Index)
	{
		const bool bIsLand = IsLand.IsValidIndex(Index) && IsLand[Index];
		if (!bIsLand)
		{
			Assigned[Index] = true;
			continue;
		}
		++LandTileCount;
	}

	TMap<EHexTileType, int32> DesiredCounts;
	BuildDesiredTileCounts(RandomStream, DesiredCounts, LandTileCount);

	for (const FHexTileGenerationRule& Rule : Settings.GenerationRules)
	{
		if (Model->IsWaterTileType(Rule.TileType))
		{
			continue;
		}

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
				if (FailedClumpAttempts >= 12)
				{
					break;
				}
				continue;
			}

			RemainingForType -= PlacedCount;
			FailedClumpAttempts = PlacedCount < TargetClumpSize ? FailedClumpAttempts + 1 : 0;
			if (FailedClumpAttempts >= 12)
			{
				break;
			}
		}
	}

	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		const bool bIsLand = IsLand.IsValidIndex(Index) && IsLand[Index];
		if (!bIsLand || Assigned[Index])
		{
			continue;
		}

		const int32 Q = Index % Model->GetGridWidth();
		const int32 R = Index / Model->GetGridWidth();
		Tiles[Index].TileType = PickWeightedLandTileType(RandomStream, Q, R);
		Assigned[Index] = true;
	}
}

void FHexMapGenerator::SetupDefaultGenerationRules(TArray<FHexTileGenerationRule>& OutRules)
{
	OutRules.Reset();

	auto AddRule = [&OutRules](EHexTileType Type, float Weight, int32 Min, int32 Max, TArray<EHexTileType> Preferred, TArray<EHexTileType> Avoid, TArray<EHexTileType> Required, bool bSoft, bool bReject, float MinScore, float Height, float PreferredTemperature, float TemperatureTolerance, float TemperatureWeight, bool bRejectBadTemperature, float MinTemperatureSuitability)
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
		Rule.PreferredTemperature = PreferredTemperature;
		Rule.TemperatureTolerance = TemperatureTolerance;
		Rule.TemperatureWeight = TemperatureWeight;
		Rule.bRejectBadTemperature = bRejectBadTemperature;
		Rule.MinTemperatureSuitability = MinTemperatureSuitability;
		OutRules.Add(Rule);
	};

	AddRule(EHexTileType::Ocean, 8.0f, 10, 28, { EHexTileType::Ocean, EHexTileType::Coast }, { EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Desert, EHexTileType::Tundra, EHexTileType::Snow, EHexTileType::Mountain, EHexTileType::Lake }, {}, true, true, 1.0f, -2.0f, 0.5f, 1.0f, 0.0f, false, 0.0f);
	AddRule(EHexTileType::Coast, 5.0f, 3, 10, { EHexTileType::Ocean, EHexTileType::Grassland, EHexTileType::Plains }, { EHexTileType::Mountain, EHexTileType::Snow, EHexTileType::Lake }, {}, true, true, 2.0f, -1.0f, 0.55f, 0.75f, 0.25f, false, 0.0f);
	AddRule(EHexTileType::Mountain, 3.5f, 1, 6, { EHexTileType::Mountain, EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Grassland, EHexTileType::Plains }, { EHexTileType::Ocean, EHexTileType::Coast, EHexTileType::Lake }, {}, true, true, 0.5f, 5.0f, 0.45f, 0.90f, 0.35f, false, 0.0f);
	AddRule(EHexTileType::Snow, 8.0f, 4, 16, { EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Mountain }, { EHexTileType::Ocean, EHexTileType::Desert, EHexTileType::Coast, EHexTileType::Lake }, {}, false, false, 0.5f, 1.0f, 0.0f, 0.24f, 2.75f, true, 0.30f);
	AddRule(EHexTileType::Tundra, 13.0f, 5, 20, { EHexTileType::Tundra, EHexTileType::Snow, EHexTileType::Plains, EHexTileType::Mountain, EHexTileType::Lake }, { EHexTileType::Ocean, EHexTileType::Desert }, {}, false, false, 0.5f, 0.0f, 0.25f, 0.32f, 1.75f, true, 0.22f);
	AddRule(EHexTileType::Grassland, 28.0f, 8, 28, { EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Coast, EHexTileType::Lake, EHexTileType::Mountain }, { EHexTileType::Ocean, EHexTileType::Snow }, {}, false, false, 0.5f, 0.5f, 0.65f, 0.45f, 0.75f, false, 0.0f);
	AddRule(EHexTileType::Plains, 22.0f, 7, 24, { EHexTileType::Plains, EHexTileType::Grassland, EHexTileType::Desert, EHexTileType::Tundra, EHexTileType::Mountain, EHexTileType::Lake }, { EHexTileType::Ocean, EHexTileType::Snow }, {}, false, false, 0.5f, 1.0f, 0.72f, 0.42f, 1.0f, true, 0.12f);
	AddRule(EHexTileType::Desert, 9.0f, 5, 18, { EHexTileType::Desert, EHexTileType::Plains, EHexTileType::Mountain }, { EHexTileType::Ocean, EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Lake }, {}, false, false, 0.5f, 2.0f, 1.0f, 0.25f, 2.0f, true, 0.45f);
	AddRule(EHexTileType::Lake, 3.0f, 1, 4, { EHexTileType::Lake, EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Tundra }, { EHexTileType::Ocean, EHexTileType::Coast, EHexTileType::Mountain }, { EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Tundra }, true, true, 1.0f, -1.0f, 0.55f, 0.75f, 0.25f, false, 0.0f);
}

void FHexMapGenerator::BuildDesiredTileCounts(FRandomStream& RandomStream, TMap<EHexTileType, int32>& OutDesiredCounts, int32 AvailableTileCount) const
{
	OutDesiredCounts.Reset();
	if (AvailableTileCount <= 0 || Settings.GenerationRules.Num() <= 0)
	{
		return;
	}

	TArray<float> AdjustedWeights;
	AdjustedWeights.SetNum(Settings.GenerationRules.Num());
	float TotalAdjustedWeight = 0.0f;

	for (int32 RuleIndex = 0; RuleIndex < Settings.GenerationRules.Num(); ++RuleIndex)
	{
		const FHexTileGenerationRule& Rule = Settings.GenerationRules[RuleIndex];
		if (Model->IsWaterTileType(Rule.TileType))
		{
			AdjustedWeights[RuleIndex] = 0.0f;
			continue;
		}

		const float RandomScale = RandomStream.FRandRange(1.0f - Settings.CountRandomness, 1.0f + Settings.CountRandomness);
		AdjustedWeights[RuleIndex] = FMath::Max(0.0f, Rule.Weight * RandomScale);
		TotalAdjustedWeight += AdjustedWeights[RuleIndex];
	}

	if (TotalAdjustedWeight <= 0.0f)
	{
		OutDesiredCounts.Add(EHexTileType::Grassland, AvailableTileCount);
		return;
	}

	int32 CountSoFar = 0;
	for (int32 RuleIndex = 0; RuleIndex < Settings.GenerationRules.Num(); ++RuleIndex)
	{
		const FHexTileGenerationRule& Rule = Settings.GenerationRules[RuleIndex];
		if (Model->IsWaterTileType(Rule.TileType))
		{
			continue;
		}

		const int32 RemainingTiles = AvailableTileCount - CountSoFar;
		if (RemainingTiles <= 0)
		{
			break;
		}

		int32 Count = FMath::RoundToInt(static_cast<float>(AvailableTileCount) * (AdjustedWeights[RuleIndex] / TotalAdjustedWeight));
		Count = FMath::Clamp(Count, 0, RemainingTiles);
		OutDesiredCounts.FindOrAdd(Rule.TileType) += Count;
		CountSoFar += Count;
	}

	if (CountSoFar < AvailableTileCount)
	{
		OutDesiredCounts.FindOrAdd(EHexTileType::Grassland) += AvailableTileCount - CountSoFar;
	}
}

bool FHexMapGenerator::FindSeedTileForRule(const FHexTileGenerationRule& Rule, FRandomStream& RandomStream, const TArray<bool>& Assigned, int32& OutQ, int32& OutR) const
{
	const int32 TotalTiles = Model->GetGridWidth() * Model->GetGridHeight();
	float BestScore = -100000.0f;
	int32 BestIndex = INDEX_NONE;

	for (int32 Attempt = 0; Attempt < Settings.SeedSelectionAttempts; ++Attempt)
	{
		const int32 CandidateIndex = RandomStream.RandRange(0, TotalTiles - 1);
		if (!Assigned.IsValidIndex(CandidateIndex) || Assigned[CandidateIndex])
		{
			continue;
		}

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
		if (Assigned[Index])
		{
			continue;
		}

		const int32 CandidateQ = Index % Model->GetGridWidth();
		const int32 CandidateR = Index / Model->GetGridWidth();

		if (!DoesTileSatisfyTemperature(Rule, CandidateQ, CandidateR))
		{
			continue;
		}

		if (!DoesTileSatisfyRequiredAdjacency(Rule, CandidateQ, CandidateR, Assigned))
		{
			continue;
		}

		const float CandidateScore = ScoreSeedTileForRule(Rule, CandidateQ, CandidateR, Assigned);
		if (Rule.bRejectBadAdjacency && CandidateScore < Rule.MinPlacementScore)
		{
			continue;
		}

		OutQ = CandidateQ;
		OutR = CandidateR;
		return true;
	}

	return false;
}

float FHexMapGenerator::ScoreSeedTileForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const
{
	if (!DoesTileSatisfyTemperature(Rule, Q, R))
	{
		return -100000.0f;
	}

	float Score = ScoreTileForRuleAdjacency(Rule, Q, R, Assigned);
	Score += ScoreTileForRuleTemperature(Rule, Q, R);
	return Score;
}

float FHexMapGenerator::ScoreTileForRuleAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const
{
	if (!DoesTileSatisfyRequiredAdjacency(Rule, Q, R, Assigned))
	{
		return -100000.0f;
	}

	float Score = 1.0f;
	const TArray<FHexTileData>& Tiles = Model->GetTiles();

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;
		if (!Model->GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR))
		{
			continue;
		}

		const int32 NeighbourIndex = Model->GetTileIndex(NeighbourQ, NeighbourR);
		if (!Tiles.IsValidIndex(NeighbourIndex) || !Assigned.IsValidIndex(NeighbourIndex) || !Assigned[NeighbourIndex])
		{
			continue;
		}

		const EHexTileType NeighbourType = Tiles[NeighbourIndex].TileType;
		if (NeighbourType == Rule.TileType)
		{
			Score += Settings.SameTypeAdjacencyBonus;
		}
		if (Rule.PreferredAdjacentTypes.Contains(NeighbourType))
		{
			Score += Settings.PreferredAdjacencyBonus;
		}
		if (Rule.AvoidAdjacentTypes.Contains(NeighbourType))
		{
			Score -= Rule.TileType == EHexTileType::Ocean ? Settings.HardAvoidAdjacencyPenalty : Settings.AvoidAdjacencyPenalty;
		}
	}

	return Score;
}

float FHexMapGenerator::GetNormalizedTemperatureAtRow(int32 R) const
{
	if (!Model)
	{
		return 0.5f;
	}

	const int32 GridHeight = Model->GetGridHeight();
	if (GridHeight <= 1)
	{
		return 0.5f;
	}

	const float NormalizedY = static_cast<float>(R) / static_cast<float>(GridHeight - 1);
	const float DistanceFromNearestPole = FMath::Min(NormalizedY, 1.0f - NormalizedY);
	float Temperature = FMath::Clamp(DistanceFromNearestPole / 0.5f, 0.0f, 1.0f);

	const FHexTemperatureSettings& TemperatureSettings = Settings.TemperatureSettings;
	Temperature = FMath::Pow(Temperature, FMath::Max(0.1f, TemperatureSettings.PolarFalloffPower));

	if (TemperatureSettings.TemperatureNoiseStrength > 0.0f)
	{
		const float Noise = FMath::Frac(FMath::Sin(static_cast<float>(R) * 12.9898f + Settings.RandomSeed * 0.123f) * 43758.5453f);
		const float SignedNoise = (Noise * 2.0f) - 1.0f;
		Temperature += SignedNoise * TemperatureSettings.TemperatureNoiseStrength;
	}

	return FMath::Clamp(Temperature, 0.0f, 1.0f);
}

float FHexMapGenerator::GetTemperatureSuitabilityForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const
{
	if (!Settings.TemperatureSettings.bUseTemperatureBias)
	{
		return 1.0f;
	}

	const float Temperature = GetNormalizedTemperatureAtRow(R);
	const float Tolerance = FMath::Max(0.01f, Rule.TemperatureTolerance);
	const float Difference = FMath::Abs(Temperature - Rule.PreferredTemperature);
	return 1.0f - FMath::Clamp(Difference / Tolerance, 0.0f, 1.0f);
}

float FHexMapGenerator::ScoreTileForRuleTemperature(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const
{
	if (!Settings.TemperatureSettings.bUseTemperatureBias)
	{
		return 0.0f;
	}

	const float Suitability = GetTemperatureSuitabilityForRule(Rule, Q, R);
	return Suitability * Settings.TemperatureSettings.TemperatureBiasStrength * Rule.TemperatureWeight;
}

bool FHexMapGenerator::DoesTileSatisfyTemperature(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const
{
	if (!Settings.TemperatureSettings.bUseTemperatureBias || !Rule.bRejectBadTemperature)
	{
		return true;
	}

	const float Suitability = GetTemperatureSuitabilityForRule(Rule, Q, R);
	return Suitability >= Rule.MinTemperatureSuitability;
}

bool FHexMapGenerator::DoesTileSatisfyRequiredAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const
{
	if (Rule.RequiredAdjacentTypes.Num() <= 0)
	{
		return true;
	}

	const TArray<FHexTileData>& Tiles = Model->GetTiles();
	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;
		if (!Model->GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR))
		{
			continue;
		}

		const int32 NeighbourIndex = Model->GetTileIndex(NeighbourQ, NeighbourR);
		if (!Tiles.IsValidIndex(NeighbourIndex) || !Assigned.IsValidIndex(NeighbourIndex) || !Assigned[NeighbourIndex])
		{
			continue;
		}

		if (Rule.RequiredAdjacentTypes.Contains(Tiles[NeighbourIndex].TileType))
		{
			return true;
		}
	}

	return false;
}

int32 FHexMapGenerator::GrowClump(const FHexTileGenerationRule& Rule, int32 SeedQ, int32 SeedR, int32 TargetSize, FRandomStream& RandomStream, TArray<bool>& Assigned)
{
	if (!Model->IsValidCoord(SeedQ, SeedR) || TargetSize <= 0)
	{
		return 0;
	}

	TArray<FIntPoint> Frontier;
	Frontier.Add(FIntPoint(SeedQ, SeedR));
	int32 PlacedCount = 0;
	TArray<FHexTileData>& Tiles = Model->GetMutableTiles();

	while (Frontier.Num() > 0 && PlacedCount < TargetSize)
	{
		const int32 ChosenFrontierIndex = PickBestFrontierIndex(Rule, Frontier, RandomStream, Assigned);
		if (!Frontier.IsValidIndex(ChosenFrontierIndex))
		{
			break;
		}

		const FIntPoint Coord = Frontier[ChosenFrontierIndex];
		Frontier.RemoveAtSwap(ChosenFrontierIndex);

		if (!Model->IsValidCoord(Coord.X, Coord.Y))
		{
			continue;
		}

		const int32 TileIndex = Model->GetTileIndex(Coord.X, Coord.Y);
		if (!Assigned.IsValidIndex(TileIndex) || Assigned[TileIndex])
		{
			continue;
		}

		if (!DoesTileSatisfyTemperature(Rule, Coord.X, Coord.Y))
		{
			continue;
		}

		Tiles[TileIndex].TileType = Rule.TileType;
		Assigned[TileIndex] = true;
		++PlacedCount;

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NeighbourQ = 0;
			int32 NeighbourR = 0;
			if (!Model->GetNeighbourCoord(Coord.X, Coord.Y, Direction, NeighbourQ, NeighbourR))
			{
				continue;
			}

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
	if (Frontier.Num() <= 0)
	{
		return INDEX_NONE;
	}

	TArray<float> Scores;
	Scores.SetNum(Frontier.Num());
	float TotalScore = 0.0f;

	for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
	{
		const FIntPoint Coord = Frontier[FrontierIndex];
		if (!DoesTileSatisfyTemperature(Rule, Coord.X, Coord.Y))
		{
			Scores[FrontierIndex] = 0.0f;
			continue;
		}

		float Score = ScoreTileForRuleAdjacency(Rule, Coord.X, Coord.Y, Assigned);
		Score += ScoreTileForRuleTemperature(Rule, Coord.X, Coord.Y);
		Score = (Rule.bRejectBadAdjacency && Score < Rule.MinPlacementScore) ? 0.0f : FMath::Max(0.05f, Score);

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

EHexTileType FHexMapGenerator::PickWeightedLandTileType(FRandomStream& RandomStream, int32 Q, int32 R) const
{
	float TotalWeight = 0.0f;
	TArray<float> RuleWeights;
	RuleWeights.SetNum(Settings.GenerationRules.Num());

	for (int32 RuleIndex = 0; RuleIndex < Settings.GenerationRules.Num(); ++RuleIndex)
	{
		const FHexTileGenerationRule& Rule = Settings.GenerationRules[RuleIndex];
		if (Model->IsWaterTileType(Rule.TileType) || !DoesTileSatisfyTemperature(Rule, Q, R))
		{
			RuleWeights[RuleIndex] = 0.0f;
			continue;
		}

		float Weight = FMath::Max(0.0f, Rule.Weight);
		if (Settings.TemperatureSettings.bUseTemperatureBias)
		{
			const float Suitability = GetTemperatureSuitabilityForRule(Rule, Q, R);
			const float TemperatureMultiplier = FMath::Lerp(0.15f, 1.0f, Suitability);
			Weight *= FMath::Lerp(1.0f, TemperatureMultiplier, FMath::Clamp(Rule.TemperatureWeight, 0.0f, 1.0f));
		}

		RuleWeights[RuleIndex] = Weight;
		TotalWeight += Weight;
	}

	if (TotalWeight <= 0.0f)
	{
		return EHexTileType::Grassland;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	for (int32 RuleIndex = 0; RuleIndex < Settings.GenerationRules.Num(); ++RuleIndex)
	{
		const FHexTileGenerationRule& Rule = Settings.GenerationRules[RuleIndex];
		if (Model->IsWaterTileType(Rule.TileType))
		{
			continue;
		}

		Roll -= RuleWeights[RuleIndex];
		if (Roll <= 0.0f)
		{
			return Rule.TileType;
		}
	}

	return EHexTileType::Grassland;
}
