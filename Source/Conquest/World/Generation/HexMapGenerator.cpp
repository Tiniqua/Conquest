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
	ApplyLakes(RandomStream, Tiles, IsLand);
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
	const bool bUseOnlyDistributedIslands = Settings.MapTypePreset == EHexMapTypePreset::SmallIslands;
	const int32 MajorLandTarget = bUseOnlyDistributedIslands
		? 0
		: FMath::Clamp(FMath::RoundToInt(static_cast<float>(TargetLandTiles) * (1.0f - IslandBudgetRatio)), 0, TargetLandTiles);

	if (MajorLandTarget > 0)
	{
		SeedMajorLandmasses(RandomStream, OutIsLand, MajorLandTarget);
	}

	if (Shape.IslandChainChance > 0.0f || bUseOnlyDistributedIslands)
	{
		AddIslandChains(RandomStream, OutIsLand, TargetLandTiles);
	}

	RefineLandWaterMask(RandomStream, OutIsLand);

	if (Shape.bUseInlandSea)
	{
		ApplyInlandSea(OutIsLand);
	}

	ApplySoftOceanBorder(RandomStream, OutIsLand);
	RestoreLandBudget(RandomStream, OutIsLand, TargetLandTiles);
	RefineLandWaterMask(RandomStream, OutIsLand);
	ApplySoftOceanBorder(RandomStream, OutIsLand);
	RestoreLandBudget(RandomStream, OutIsLand, TargetLandTiles);
	ApplySoftOceanBorder(RandomStream, OutIsLand);

	if (Settings.MapTypePreset == EHexMapTypePreset::Pangaea)
	{
		FillEnclosedWaterRegions(OutIsLand);
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
	const int32 SafeSeparation = LandmassCount > 1 ? FMath::Max(0, Shape.LandmassSeparation) : 0;
	const int32 MinQ = FMath::Clamp(Shape.OceanBorderWidth, 0, FMath::Max(0, Width - 1));
	const int32 MaxQ = FMath::Clamp(Width - Shape.OceanBorderWidth - 1, MinQ, FMath::Max(0, Width - 1));
	const int32 MinR = FMath::Clamp(Shape.OceanBorderWidth, 0, FMath::Max(0, Height - 1));
	const int32 MaxR = FMath::Clamp(Height - Shape.OceanBorderWidth - 1, MinR, FMath::Max(0, Height - 1));

	TArray<float> LandmassWeights;
	LandmassWeights.SetNum(LandmassCount);
	float WeightTotal = 0.0f;
	for (int32 LandmassIndex = 0; LandmassIndex < LandmassCount; ++LandmassIndex)
	{
		const float Variance = Settings.MapTypePreset == EHexMapTypePreset::Fractal ? 0.85f : 0.35f;
		float Weight = RandomStream.FRandRange(1.0f - Variance, 1.0f + Variance);
		if (Settings.MapTypePreset == EHexMapTypePreset::Continents && LandmassIndex >= 3)
		{
			Weight *= 0.38f;
		}

		LandmassWeights[LandmassIndex] = FMath::Max(0.1f, Weight);
		WeightTotal += LandmassWeights[LandmassIndex];
	}

	int32 AssignedTarget = 0;
	for (int32 LandmassIndex = 0; LandmassIndex < LandmassCount; ++LandmassIndex)
	{
		const int32 RemainingLand = TargetLandTiles - AssignedTarget;
		if (RemainingLand <= 0)
		{
			break;
		}

		const bool bLastLandmass = LandmassIndex == LandmassCount - 1;
		int32 LandmassTarget = bLastLandmass
			? RemainingLand
			: FMath::RoundToInt(static_cast<float>(TargetLandTiles) * (LandmassWeights[LandmassIndex] / FMath::Max(KINDA_SMALL_NUMBER, WeightTotal)));
		LandmassTarget = FMath::Clamp(LandmassTarget, 1, RemainingLand);
		AssignedTarget += LandmassTarget;

		FIntPoint Seed(INDEX_NONE, INDEX_NONE);
		float BestSeedScore = -FLT_MAX;
		for (int32 Attempt = 0; Attempt < 128; ++Attempt)
		{
			const int32 SeedQ = RandomStream.RandRange(MinQ, MaxQ);
			const int32 SeedR = RandomStream.RandRange(MinR, MaxR);
			const int32 SeedIndex = Model->GetTileIndex(SeedQ, SeedR);

			if (InOutIsLand.IsValidIndex(SeedIndex) &&
				!InOutIsLand[SeedIndex] &&
				!IsReservedWaterTile(SeedQ, SeedR) &&
				!IsNearLandOutsideSet(SeedQ, SeedR, InOutIsLand, TSet<int32>(), SafeSeparation))
			{
				const float SeedScore = GetCenterBiasScore(SeedQ, SeedR) + RandomStream.FRandRange(0.0f, 0.35f);
				if (SeedScore > BestSeedScore)
				{
					BestSeedScore = SeedScore;
					Seed = FIntPoint(SeedQ, SeedR);
				}
			}
		}

		if (Seed.X == INDEX_NONE)
		{
			continue;
		}

		TArray<FIntPoint> Frontier;
		TSet<int32> CurrentLandmassIndices;
		const int32 SeedIndex = Model->GetTileIndex(Seed.X, Seed.Y);
		InOutIsLand[SeedIndex] = true;
		CurrentLandmassIndices.Add(SeedIndex);
		Frontier.Add(Seed);

		int32 PlacedForLandmass = 1;
		int32 Safety = 0;
		const int32 MaxSafety = FMath::Max(1000, LandmassTarget * 80);

		while (PlacedForLandmass < LandmassTarget && Frontier.Num() > 0 && Safety < MaxSafety)
		{
			++Safety;
			const int32 FrontierIndex = RandomStream.RandRange(0, Frontier.Num() - 1);
			const FIntPoint Coord = Frontier[FrontierIndex];

			if (RandomStream.FRand() < Shape.LandmassFragmentation * 0.35f)
			{
				Frontier.RemoveAtSwap(FrontierIndex);
				continue;
			}

			TArray<FIntPoint> Candidates;
			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				int32 NQ = 0;
				int32 NR = 0;

				if (!Model->GetNeighbourCoord(Coord.X, Coord.Y, Direction, NQ, NR) || IsReservedWaterTile(NQ, NR))
				{
					continue;
				}

				const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
				if (!InOutIsLand.IsValidIndex(NeighbourIndex) ||
					InOutIsLand[NeighbourIndex] ||
					IsNearLandOutsideSet(NQ, NR, InOutIsLand, CurrentLandmassIndices, SafeSeparation))
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

			InOutIsLand[ChosenIndex] = true;
			CurrentLandmassIndices.Add(ChosenIndex);
			Frontier.Add(Chosen);
			++PlacedForLandmass;

			if (RandomStream.FRand() > Shape.CoastlineNoise)
			{
				Frontier.RemoveAtSwap(FrontierIndex);
			}
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
		const int32 SeedQ = RandomStream.RandRange(0, Width - 1);
		const int32 SeedR = RandomStream.RandRange(0, Height - 1);
		if (!Model->IsValidCoord(SeedQ, SeedR) || IsReservedWaterTile(SeedQ, SeedR))
		{
			continue;
		}

		const int32 SeedIndex = Model->GetTileIndex(SeedQ, SeedR);
		if (!InOutIsLand.IsValidIndex(SeedIndex) || InOutIsLand[SeedIndex])
		{
			continue;
		}

		const int32 MaxIslandSize = Settings.MapTypePreset == EHexMapTypePreset::SmallIslands ? 72 : 18;
		const int32 MinIslandSize = Settings.MapTypePreset == EHexMapTypePreset::SmallIslands ? 24 : 4;
		const int32 IslandTargetSize = FMath::Min(IslandBudget - Placed, RandomStream.RandRange(MinIslandSize, MaxIslandSize));
		TArray<FIntPoint> Frontier;
		TSet<int32> IslandIndices;

		InOutIsLand[SeedIndex] = true;
		IslandIndices.Add(SeedIndex);
		++Placed;

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NQ = 0;
			int32 NR = 0;
			if (!Model->GetNeighbourCoord(SeedQ, SeedR, Direction, NQ, NR) || IsReservedWaterTile(NQ, NR))
			{
				continue;
			}

			const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
			if (InOutIsLand.IsValidIndex(NeighbourIndex) && !InOutIsLand[NeighbourIndex])
			{
				Frontier.AddUnique(FIntPoint(NQ, NR));
			}
		}

		while (Frontier.Num() > 0 && IslandIndices.Num() < IslandTargetSize && Placed < IslandBudget)
		{
			float BestScore = -1.0f;
			int32 BestFrontierIndex = INDEX_NONE;
			FIntPoint BestCoord(INDEX_NONE, INDEX_NONE);

			for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
			{
				const FIntPoint Coord = Frontier[FrontierIndex];
				const int32 LandNeighbours = CountLandNeighbours(Coord.X, Coord.Y, InOutIsLand);
				const int32 WaterNeighbours = CountWaterNeighbours(Coord.X, Coord.Y, InOutIsLand);
				const float Score =
					static_cast<float>(LandNeighbours) * 2.2f -
					static_cast<float>(FMath::Max(0, WaterNeighbours - 3)) * 1.4f +
					RandomStream.FRandRange(0.0f, 1.0f);

				if (Score > BestScore)
				{
					BestScore = Score;
					BestFrontierIndex = FrontierIndex;
					BestCoord = Coord;
				}
			}

			if (BestFrontierIndex == INDEX_NONE)
			{
				break;
			}

			Frontier.RemoveAtSwap(BestFrontierIndex);

			if (!Model->IsValidCoord(BestCoord.X, BestCoord.Y) || IsReservedWaterTile(BestCoord.X, BestCoord.Y))
			{
				continue;
			}

			const int32 BestIndex = Model->GetTileIndex(BestCoord.X, BestCoord.Y);
			if (!InOutIsLand.IsValidIndex(BestIndex) || InOutIsLand[BestIndex])
			{
				continue;
			}

			InOutIsLand[BestIndex] = true;
			IslandIndices.Add(BestIndex);
			++Placed;

			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				int32 NQ = 0;
				int32 NR = 0;
				if (!Model->GetNeighbourCoord(BestCoord.X, BestCoord.Y, Direction, NQ, NR) || IsReservedWaterTile(NQ, NR))
				{
					continue;
				}

				const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
				if (InOutIsLand.IsValidIndex(NeighbourIndex) && !InOutIsLand[NeighbourIndex])
				{
					Frontier.AddUnique(FIntPoint(NQ, NR));
				}
			}
		}
	}
}

void FHexMapGenerator::RefineLandWaterMask(FRandomStream& RandomStream, TArray<bool>& InOutIsLand) const
{
	constexpr int32 RefinementPasses = 3;

	for (int32 Pass = 0; Pass < RefinementPasses; ++Pass)
	{
		TArray<bool> NextMask = InOutIsLand;

		for (int32 R = 0; R < Model->GetGridHeight(); ++R)
		{
			for (int32 Q = 0; Q < Model->GetGridWidth(); ++Q)
			{
				const int32 Index = Model->GetTileIndex(Q, R);
				if (!InOutIsLand.IsValidIndex(Index))
				{
					continue;
				}

				if (IsMapEdge(Q, R))
				{
					NextMask[Index] = false;
					continue;
				}

				const int32 LandNeighbours = CountLandNeighbours(Q, R, InOutIsLand);
				const int32 WaterNeighbours = CountWaterNeighbours(Q, R, InOutIsLand);

				if (InOutIsLand[Index])
				{
					if (WaterNeighbours >= 5 || (WaterNeighbours == 4 && RandomStream.FRand() < 0.82f))
					{
						NextMask[Index] = false;
					}
					else if (WaterNeighbours == 3 && LandNeighbours <= 2 && RandomStream.FRand() < 0.25f)
					{
						NextMask[Index] = false;
					}
				}
				else
				{
					if (LandNeighbours >= 5 || (LandNeighbours == 4 && RandomStream.FRand() < 0.72f))
					{
						NextMask[Index] = true;
					}
				}
			}
		}

		InOutIsLand = NextMask;
	}
}

void FHexMapGenerator::ApplySoftOceanBorder(FRandomStream& RandomStream, TArray<bool>& InOutIsLand) const
{
	const int32 BorderWidth = FMath::Max(0, Settings.MapShapeSettings.OceanBorderWidth);
	if (BorderWidth <= 0)
	{
		return;
	}

	const int32 Width = Model->GetGridWidth();
	const int32 Height = Model->GetGridHeight();

	for (int32 R = 0; R < Height; ++R)
	{
		for (int32 Q = 0; Q < Width; ++Q)
		{
			const int32 Index = Model->GetTileIndex(Q, R);
			if (!InOutIsLand.IsValidIndex(Index))
			{
				continue;
			}

			const int32 DistanceToEdge = FMath::Min(FMath::Min(Q, Width - 1 - Q), FMath::Min(R, Height - 1 - R));
			if (DistanceToEdge <= 0)
			{
				InOutIsLand[Index] = false;
				continue;
			}

			if (!InOutIsLand[Index] || DistanceToEdge >= BorderWidth)
			{
				continue;
			}

			const float BorderAlpha = 1.0f - (static_cast<float>(DistanceToEdge) / static_cast<float>(FMath::Max(1, BorderWidth)));
			float ErodeChance = FMath::Lerp(0.18f, 0.72f, BorderAlpha);
			if (CountWaterNeighbours(Q, R, InOutIsLand) >= 3)
			{
				ErodeChance += 0.18f;
			}

			if (RandomStream.FRand() < FMath::Clamp(ErodeChance, 0.0f, 0.95f))
			{
				InOutIsLand[Index] = false;
			}
		}
	}
}

void FHexMapGenerator::RestoreLandBudget(FRandomStream& RandomStream, TArray<bool>& InOutIsLand, int32 TargetLandTiles) const
{
	int32 CurrentLandTiles = CountLandTiles(InOutIsLand);
	if (CurrentLandTiles <= 0)
	{
		float BestSeedScore = -FLT_MAX;
		int32 BestSeedIndex = INDEX_NONE;
		for (int32 R = 0; R < Model->GetGridHeight(); ++R)
		{
			for (int32 Q = 0; Q < Model->GetGridWidth(); ++Q)
			{
				if (IsReservedWaterTile(Q, R))
				{
					continue;
				}

				const int32 Index = Model->GetTileIndex(Q, R);
				if (!InOutIsLand.IsValidIndex(Index))
				{
					continue;
				}

				const float Score = GetCenterBiasScore(Q, R) + RandomStream.FRandRange(0.0f, 0.5f);
				if (Score > BestSeedScore)
				{
					BestSeedScore = Score;
					BestSeedIndex = Index;
				}
			}
		}

		if (BestSeedIndex != INDEX_NONE)
		{
			InOutIsLand[BestSeedIndex] = true;
			CurrentLandTiles = 1;
		}
	}

	if (CurrentLandTiles >= TargetLandTiles)
	{
		return;
	}

	int32 Safety = 0;
	const int32 MaxSafety = FMath::Max(500, (TargetLandTiles - CurrentLandTiles) * 60);
	while (CurrentLandTiles < TargetLandTiles && Safety < MaxSafety)
	{
		++Safety;

		float BestScore = -FLT_MAX;
		int32 BestIndex = INDEX_NONE;

		for (int32 R = 0; R < Model->GetGridHeight(); ++R)
		{
			for (int32 Q = 0; Q < Model->GetGridWidth(); ++Q)
			{
				if (IsReservedWaterTile(Q, R))
				{
					continue;
				}

				const int32 Index = Model->GetTileIndex(Q, R);
				if (!InOutIsLand.IsValidIndex(Index) || InOutIsLand[Index])
				{
					continue;
				}

				const int32 LandNeighbours = CountLandNeighbours(Q, R, InOutIsLand);
				if (LandNeighbours <= 0)
				{
					continue;
				}

				const int32 WaterNeighbours = CountWaterNeighbours(Q, R, InOutIsLand);
				const float BiasScore = Settings.MapTypePreset == EHexMapTypePreset::SmallIslands
					? 0.0f
					: GetCenterBiasScore(Q, R);
				const float Score =
					static_cast<float>(LandNeighbours) * 2.5f -
					static_cast<float>(FMath::Max(0, WaterNeighbours - 3)) +
					BiasScore +
					RandomStream.FRandRange(0.0f, 0.75f);

				if (Score > BestScore)
				{
					BestScore = Score;
					BestIndex = Index;
				}
			}
		}

		if (BestIndex == INDEX_NONE)
		{
			break;
		}

		InOutIsLand[BestIndex] = true;
		++CurrentLandTiles;
	}
}

void FHexMapGenerator::FillEnclosedWaterRegions(TArray<bool>& InOutIsLand) const
{
	TArray<bool> Visited;
	Visited.Init(false, InOutIsLand.Num());

	for (int32 StartIndex = 0; StartIndex < InOutIsLand.Num(); ++StartIndex)
	{
		if (Visited[StartIndex] || InOutIsLand[StartIndex])
		{
			continue;
		}

		int32 StartQ = 0;
		int32 StartR = 0;
		if (!Model->GetCoordFromIndex(StartIndex, StartQ, StartR))
		{
			continue;
		}

		bool bTouchesEdgeOcean = false;
		TArray<int32> Region;
		TArray<int32> Stack;
		Stack.Add(StartIndex);
		Visited[StartIndex] = true;

		while (Stack.Num() > 0)
		{
			const int32 CurrentIndex = Stack.Pop(EAllowShrinking::No);
			Region.Add(CurrentIndex);

			int32 Q = 0;
			int32 R = 0;
			if (!Model->GetCoordFromIndex(CurrentIndex, Q, R))
			{
				continue;
			}

			if (IsMapEdge(Q, R))
			{
				bTouchesEdgeOcean = true;
			}

			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				int32 NQ = 0;
				int32 NR = 0;
				if (!Model->GetNeighbourCoord(Q, R, Direction, NQ, NR))
				{
					bTouchesEdgeOcean = true;
					continue;
				}

				const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
				if (!InOutIsLand.IsValidIndex(NeighbourIndex) ||
					Visited[NeighbourIndex] ||
					InOutIsLand[NeighbourIndex])
				{
					continue;
				}

				Visited[NeighbourIndex] = true;
				Stack.Add(NeighbourIndex);
			}
		}

		if (bTouchesEdgeOcean)
		{
			continue;
		}

		for (const int32 RegionIndex : Region)
		{
			if (InOutIsLand.IsValidIndex(RegionIndex))
			{
				InOutIsLand[RegionIndex] = true;
			}
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

void FHexMapGenerator::ApplyLakes(FRandomStream& RandomStream, TArray<FHexTileData>& Tiles, const TArray<bool>& IsLand) const
{
	const FHexMapShapeSettings& Shape = Settings.MapShapeSettings;
	const int32 LandTileCount = CountLandTiles(IsLand);
	const int32 TargetLakeTiles = Shape.LakeCount > 0
		? Shape.LakeCount
		: FMath::RoundToInt(static_cast<float>(LandTileCount) * FMath::Clamp(Shape.LakeFrequency, 0.0f, 1.0f));

	if (TargetLakeTiles <= 0)
	{
		return;
	}

	const int32 SafeMinSize = FMath::Max(1, Shape.LakeMinSize);
	const int32 SafeMaxSize = FMath::Max(SafeMinSize, Shape.LakeMaxSize);
	const int32 SafeSpacing = FMath::Max(0, Shape.LakeSpacing);

	int32 PlacedLakeTiles = 0;
	int32 Attempts = 0;
	const int32 MaxAttempts = FMath::Max(200, TargetLakeTiles * 40);

	while (PlacedLakeTiles < TargetLakeTiles && Attempts < MaxAttempts)
	{
		++Attempts;

		const int32 SeedIndex = RandomStream.RandRange(0, Tiles.Num() - 1);
		if (!Tiles.IsValidIndex(SeedIndex) || !IsLand.IsValidIndex(SeedIndex) || !IsLand[SeedIndex])
		{
			continue;
		}

		const int32 SeedQ = SeedIndex % Model->GetGridWidth();
		const int32 SeedR = SeedIndex / Model->GetGridWidth();
		if (IsMapEdge(SeedQ, SeedR) ||
			Tiles[SeedIndex].TileType == EHexTileType::Mountain ||
			Model->IsWaterTileType(Tiles[SeedIndex].TileType) ||
			IsNearTileType(SeedQ, SeedR, Tiles, EHexTileType::Ocean, 1) ||
			IsNearTileType(SeedQ, SeedR, Tiles, EHexTileType::Coast, 1) ||
			IsNearTileType(SeedQ, SeedR, Tiles, EHexTileType::Lake, SafeSpacing))
		{
			continue;
		}

		const int32 LakeTargetSize = FMath::Min(TargetLakeTiles - PlacedLakeTiles, RandomStream.RandRange(SafeMinSize, SafeMaxSize));
		TArray<FIntPoint> Frontier;
		TSet<int32> LakeIndices;
		LakeIndices.Add(SeedIndex);

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NQ = 0;
			int32 NR = 0;
			if (Model->GetNeighbourCoord(SeedQ, SeedR, Direction, NQ, NR))
			{
				Frontier.AddUnique(FIntPoint(NQ, NR));
			}
		}

		while (Frontier.Num() > 0 && LakeIndices.Num() < LakeTargetSize)
		{
			float BestScore = -FLT_MAX;
			int32 BestFrontierIndex = INDEX_NONE;
			FIntPoint Coord(INDEX_NONE, INDEX_NONE);

			for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
			{
				const FIntPoint Candidate = Frontier[FrontierIndex];
				if (!Model->IsValidTile(Candidate.X, Candidate.Y) || IsMapEdge(Candidate.X, Candidate.Y))
				{
					continue;
				}

				const int32 CandidateIndex = Model->GetTileIndex(Candidate.X, Candidate.Y);
				if (!Tiles.IsValidIndex(CandidateIndex) ||
					!IsLand.IsValidIndex(CandidateIndex) ||
					!IsLand[CandidateIndex] ||
					LakeIndices.Contains(CandidateIndex) ||
					Tiles[CandidateIndex].TileType == EHexTileType::Mountain ||
					Model->IsWaterTileType(Tiles[CandidateIndex].TileType) ||
					IsNearTileType(Candidate.X, Candidate.Y, Tiles, EHexTileType::Ocean, 1) ||
					IsNearTileType(Candidate.X, Candidate.Y, Tiles, EHexTileType::Coast, 1))
				{
					continue;
				}

				int32 LakeNeighbours = 0;
				for (int32 Direction = 0; Direction < 6; ++Direction)
				{
					int32 NQ = 0;
					int32 NR = 0;
					if (!Model->GetNeighbourCoord(Candidate.X, Candidate.Y, Direction, NQ, NR))
					{
						continue;
					}

					const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
					if (LakeIndices.Contains(NeighbourIndex))
					{
						++LakeNeighbours;
					}
				}

				const float Score =
					static_cast<float>(LakeNeighbours) * 3.0f +
					RandomStream.FRandRange(0.0f, 1.0f);

				if (Score > BestScore)
				{
					BestScore = Score;
					BestFrontierIndex = FrontierIndex;
					Coord = Candidate;
				}
			}

			if (BestFrontierIndex == INDEX_NONE)
			{
				break;
			}

			Frontier.RemoveAtSwap(BestFrontierIndex);

			if (!Model->IsValidTile(Coord.X, Coord.Y) || IsMapEdge(Coord.X, Coord.Y))
			{
				continue;
			}

			const int32 TileIndex = Model->GetTileIndex(Coord.X, Coord.Y);
			if (!Tiles.IsValidIndex(TileIndex) ||
				!IsLand.IsValidIndex(TileIndex) ||
				!IsLand[TileIndex] ||
				LakeIndices.Contains(TileIndex) ||
				Tiles[TileIndex].TileType == EHexTileType::Mountain ||
				Model->IsWaterTileType(Tiles[TileIndex].TileType) ||
				IsNearTileType(Coord.X, Coord.Y, Tiles, EHexTileType::Ocean, 1) ||
				IsNearTileType(Coord.X, Coord.Y, Tiles, EHexTileType::Coast, 1))
			{
				continue;
			}

			LakeIndices.Add(TileIndex);

			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				int32 NQ = 0;
				int32 NR = 0;
				if (Model->GetNeighbourCoord(Coord.X, Coord.Y, Direction, NQ, NR))
				{
					Frontier.AddUnique(FIntPoint(NQ, NR));
				}
			}
		}

		if (LakeIndices.Num() <= 0)
		{
			continue;
		}

		for (const int32 LakeIndex : LakeIndices)
		{
			if (!Tiles.IsValidIndex(LakeIndex))
			{
				continue;
			}

			Tiles[LakeIndex].TileType = EHexTileType::Lake;
			Tiles[LakeIndex].bHasFreshWater = true;
			++PlacedLakeTiles;
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

	auto AddRule = [&OutRules](
		EHexTileType Type,
		float Weight,
		int32 Min,
		int32 Max,
		TArray<EHexTileType> Preferred,
		TArray<EHexTileType> Avoid,
		TArray<EHexTileType> Required,
		bool bSoft,
		bool bReject,
		float MinScore,
		float Height,
		float PreferredTemperature,
		float TemperatureTolerance,
		float TemperatureWeight,
		bool bRejectBadTemperature,
		float MinTemperatureSuitability,
		float RingVariancePercent,
		bool bUseTemperatureRangeGate,
		float MinAllowedTemperature,
		float MaxAllowedTemperature
	)
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
		Rule.RingVertexHeightVariancePercent = RingVariancePercent;
		Rule.bUseTemperatureRangeGate = bUseTemperatureRangeGate;
		Rule.MinAllowedTemperature = MinAllowedTemperature;
		Rule.MaxAllowedTemperature = MaxAllowedTemperature;

		OutRules.Add(Rule);
	};

	AddRule(
		EHexTileType::Ocean,
		8.0f,
		10,
		28,
		{ EHexTileType::Ocean, EHexTileType::Coast },
        { EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Desert, EHexTileType::Jungle, EHexTileType::Tundra, EHexTileType::Snow, EHexTileType::Mountain, EHexTileType::Lake },
		{},
		true,
		true,
		1.0f,
		-2.0f,
		0.5f,
		1.0f,
		0.0f,
		false,
		0.0f,
		0.0f,
		false,
		0.0f,
		1.0f
	);

	AddRule(
		EHexTileType::Coast,
		5.0f,
		3,
		10,
		{ EHexTileType::Ocean, EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Jungle },
		{ EHexTileType::Mountain, EHexTileType::Snow, EHexTileType::Lake },
		{},
		true,
		true,
		2.0f,
		-1.0f,
		0.55f,
		0.75f,
		0.25f,
		false,
		0.0f,
		0.0f,
		false,
		0.0f,
		1.0f
	);

	AddRule(
		EHexTileType::Jungle,
		7.0f,
		4,
		14,
		{ EHexTileType::Jungle, EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Lake },
		{ EHexTileType::Ocean, EHexTileType::Coast, EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Desert },
		{},
		false,
		false,
		0.5f,
		0.75f,
		0.88f,
		0.24f,
		2.25f,
		false,
		0.0f,
		0.035f,
		true,
		0.62f,
		1.0f
);

	AddRule(
		EHexTileType::Mountain,
		3.5f,
		1,
		6,
		{ EHexTileType::Mountain, EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Grassland, EHexTileType::Plains },
		{ EHexTileType::Ocean, EHexTileType::Coast, EHexTileType::Lake },
		{},
		true,
		true,
		0.5f,
		5.0f,
		0.45f,
		0.90f,
		0.35f,
		false,
		0.0f,
		0.30f,
		false,
		0.0f,
		1.0f
	);

	AddRule(
		EHexTileType::Snow,
		8.0f,
		4,
		16,
		{ EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Mountain },
		{ EHexTileType::Ocean, EHexTileType::Desert, EHexTileType::Coast, EHexTileType::Lake, EHexTileType::Jungle },
		{},
		false,
		false,
		0.5f,
		1.0f,
		0.0f,
		0.32f,
		2.25f,
		false,
		0.0f,
		0.04f,
		true,
		0.0f,
		0.42f
	);

	AddRule(
		EHexTileType::Tundra,
		13.0f,
		5,
		20,
		{ EHexTileType::Tundra, EHexTileType::Snow, EHexTileType::Plains, EHexTileType::Mountain, EHexTileType::Lake },
		{ EHexTileType::Ocean, EHexTileType::Desert, EHexTileType::Jungle },
		{},
		false,
		false,
		0.5f,
		0.0f,
		0.25f,
		0.42f,
		1.50f,
		false,
		0.0f,
		0.02f,
		true,
		0.0f,
		0.62f
	);

	AddRule(
		EHexTileType::Grassland,
		28.0f,
		8,
		28,
		{ EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Coast, EHexTileType::Lake, EHexTileType::Mountain, EHexTileType::Jungle},
		{ EHexTileType::Ocean, EHexTileType::Snow },
		{},
		false,
		false,
		0.5f,
		0.5f,
		0.65f,
		0.45f,
		0.75f,
		false,
		0.0f,
		0.02f,
		false,
		0.0f,
		1.0f
	);

	AddRule(
		EHexTileType::Plains,
		22.0f,
		7,
		24,
		{ EHexTileType::Plains, EHexTileType::Grassland, EHexTileType::Desert, EHexTileType::Tundra, EHexTileType::Mountain, EHexTileType::Lake },
		{ EHexTileType::Ocean, EHexTileType::Snow },
		{},
		false,
		false,
		0.5f,
		1.0f,
		0.72f,
		0.42f,
		1.0f,
		false,
		0.0f,
		0.03f,
		false,
		0.0f,
		1.0f
	);

	AddRule(
		EHexTileType::Desert,
		9.0f,
		5,
		18,
		{ EHexTileType::Desert, EHexTileType::Plains, EHexTileType::Mountain },
		{ EHexTileType::Ocean, EHexTileType::Snow, EHexTileType::Tundra, EHexTileType::Lake , EHexTileType::Jungle},
		{},
		false,
		false,
		0.5f,
		2.0f,
		1.0f,
		0.25f,
		2.0f,
		false,
		0.0f,
		0.025f,
		true,
		0.58f,
		1.0f
	);

	AddRule(
		EHexTileType::Lake,
		3.0f,
		1,
		4,
		{ EHexTileType::Lake, EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Tundra },
		{ EHexTileType::Ocean, EHexTileType::Coast, EHexTileType::Mountain },
		{ EHexTileType::Grassland, EHexTileType::Plains, EHexTileType::Tundra },
		true,
		true,
		1.0f,
		-1.0f,
		0.55f,
		0.75f,
		0.25f,
		false,
		0.0f,
		0.0f,
		false,
		0.0f,
		1.0f
	);
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

float FHexMapGenerator::GetDeterministicValueNoise2D(
	int32 Q,
	int32 R,
	int32 Salt
) const
{
	const int32 HashInput =
		(Q * 73856093) ^
		(R * 19349663) ^
		(Salt * 83492791) ^
		(Settings.RandomSeed * 2654435761);

	const float Noise = FMath::Frac(
		FMath::Sin(static_cast<float>(HashInput) * 0.0001f) * 43758.5453f
	);

	return (Noise * 2.0f) - 1.0f;
}

float FHexMapGenerator::GetRawTemperatureAtTile(int32 Q, int32 R) const
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

	const float NormalizedY =
		static_cast<float>(R) / static_cast<float>(GridHeight - 1);

	const float DistanceFromNearestPole =
		FMath::Min(NormalizedY, 1.0f - NormalizedY);

	float Temperature =
		FMath::Clamp(DistanceFromNearestPole / 0.5f, 0.0f, 1.0f);

	const FHexTemperatureSettings& TemperatureSettings =
		Settings.TemperatureSettings;

	Temperature = FMath::Pow(
		Temperature,
		FMath::Max(0.1f, TemperatureSettings.PolarFalloffPower)
	);

	if (TemperatureSettings.TemperatureNoiseStrength > 0.0f)
	{
		const float NoiseA = GetDeterministicValueNoise2D(Q / 3, R / 3, 11);
		const float NoiseB = GetDeterministicValueNoise2D(Q / 7, R / 7, 23);
		const float NoiseC = GetDeterministicValueNoise2D(Q, R, 37);

		const float CombinedNoise =
			(NoiseA * 0.55f) +
			(NoiseB * 0.35f) +
			(NoiseC * 0.10f);

		Temperature += CombinedNoise * TemperatureSettings.TemperatureNoiseStrength;
	}

	return FMath::Clamp(Temperature, 0.0f, 1.0f);
}

// redundant but cba fixed references
float FHexMapGenerator::GetNormalizedTemperatureAtTile(int32 Q, int32 R) const
{
	return GetRawTemperatureAtTile(Q, R);
}

float FHexMapGenerator::GetTemperatureSuitabilityForRule(
	const FHexTileGenerationRule& Rule,
	int32 Q,
	int32 R) const
{
	if (!Settings.TemperatureSettings.bUseTemperatureBias)
	{
		return 1.0f;
	}

	const float Temperature = GetNormalizedTemperatureAtTile(Q, R);
	const float Tolerance = FMath::Max(0.01f, Rule.TemperatureTolerance);
	const float Difference = FMath::Abs(Temperature - Rule.PreferredTemperature);

	return 1.0f - FMath::Clamp(Difference / Tolerance, 0.0f, 1.0f);
}

float FHexMapGenerator::ScoreTileForRuleTemperature(
	const FHexTileGenerationRule& Rule,
	int32 Q,
	int32 R
) const
{
	if (!Settings.TemperatureSettings.bUseTemperatureBias)
	{
		return 0.0f;
	}

	const float Suitability = GetTemperatureSuitabilityForRule(Rule, Q, R);

	const float ShapedSuitability =
		FMath::SmoothStep(0.0f, 1.0f, Suitability);

	return ShapedSuitability
		* Settings.TemperatureSettings.TemperatureBiasStrength
		* Rule.TemperatureWeight;
}

bool FHexMapGenerator::DoesTileSatisfyTemperature(
	const FHexTileGenerationRule& Rule,
	int32 Q,
	int32 R
) const
{
	if (!Settings.TemperatureSettings.bUseTemperatureBias)
	{
		return true;
	}

	if (Rule.bUseTemperatureRangeGate)
	{
		const float Temperature = GetRawTemperatureAtTile(Q, R);

		if (Temperature < Rule.MinAllowedTemperature ||
			Temperature > Rule.MaxAllowedTemperature)
		{
			return false;
		}
	}

	if (!Rule.bRejectBadTemperature)
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
			const float TemperatureStrength = FMath::Clamp(Rule.TemperatureWeight, 0.0f, 4.0f);
			const float TemperatureMultiplier =	FMath::Lerp(0.05f, 1.0f, Suitability);
			Weight *= FMath::Pow(TemperatureMultiplier,TemperatureStrength);
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

bool FHexMapGenerator::IsMapEdge(int32 Q, int32 R) const
{
	return Q <= 0 ||
		R <= 0 ||
		Q >= Model->GetGridWidth() - 1 ||
		R >= Model->GetGridHeight() - 1;
}

bool FHexMapGenerator::IsReservedWaterTile(int32 Q, int32 R) const
{
	if (IsMapEdge(Q, R))
	{
		return true;
	}

	const FHexMapShapeSettings& Shape = Settings.MapShapeSettings;
	if (!Shape.bUseInlandSea)
	{
		return false;
	}

	const FVector2D Center(
		static_cast<float>(Model->GetGridWidth() - 1) * 0.5f,
		static_cast<float>(Model->GetGridHeight() - 1) * 0.5f
	);
	const FVector2D Coord(static_cast<float>(Q), static_cast<float>(R));
	const float InlandSeaRadius = FMath::Min(
		static_cast<float>(Model->GetGridWidth()),
		static_cast<float>(Model->GetGridHeight())
	) * Shape.InlandSeaRadiusRatio;

	return FVector2D::Distance(Coord, Center) <= InlandSeaRadius;
}

float FHexMapGenerator::GetCenterBiasScore(int32 Q, int32 R) const
{
	const float Width = static_cast<float>(FMath::Max(1, Model->GetGridWidth() - 1));
	const float Height = static_cast<float>(FMath::Max(1, Model->GetGridHeight() - 1));
	const float NormalizedQ = static_cast<float>(Q) / Width;
	const float NormalizedR = static_cast<float>(R) / Height;

	const float EdgeDistance = FMath::Min(
		FMath::Min(NormalizedQ, 1.0f - NormalizedQ),
		FMath::Min(NormalizedR, 1.0f - NormalizedR)
	) * 2.0f;

	const FVector2D Center(0.5f, 0.5f);
	const float CenterDistance = FVector2D::Distance(FVector2D(NormalizedQ, NormalizedR), Center);
	const float CenterScore = 1.0f - FMath::Clamp(CenterDistance / 0.7071f, 0.0f, 1.0f);

	return EdgeDistance * 1.4f + CenterScore;
}

int32 FHexMapGenerator::CountLandNeighbours(int32 Q, int32 R, const TArray<bool>& IsLand) const
{
	int32 LandNeighbours = 0;
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
			++LandNeighbours;
		}
	}

	return LandNeighbours;
}

int32 FHexMapGenerator::CountWaterNeighbours(int32 Q, int32 R, const TArray<bool>& IsLand) const
{
	int32 WaterNeighbours = 0;
	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NQ = 0;
		int32 NR = 0;
		if (!Model->GetNeighbourCoord(Q, R, Direction, NQ, NR))
		{
			++WaterNeighbours;
			continue;
		}

		const int32 NeighbourIndex = Model->GetTileIndex(NQ, NR);
		if (!IsLand.IsValidIndex(NeighbourIndex) || !IsLand[NeighbourIndex])
		{
			++WaterNeighbours;
		}
	}

	return WaterNeighbours;
}

bool FHexMapGenerator::IsNearLandOutsideSet(
	int32 Q,
	int32 R,
	const TArray<bool>& IsLand,
	const TSet<int32>& AllowedLandIndices,
	int32 Radius
) const
{
	if (Radius <= 0)
	{
		return false;
	}

	const TArray<FIntPoint> NearbyCoords = Model->GetCoordsInRange(FIntPoint(Q, R), Radius);
	for (const FIntPoint& Coord : NearbyCoords)
	{
		const int32 TileIndex = Model->GetTileIndex(Coord.X, Coord.Y);
		if (IsLand.IsValidIndex(TileIndex) && IsLand[TileIndex] && !AllowedLandIndices.Contains(TileIndex))
		{
			return true;
		}
	}

	return false;
}

bool FHexMapGenerator::IsNearTileType(
	int32 Q,
	int32 R,
	const TArray<FHexTileData>& Tiles,
	EHexTileType TileType,
	int32 Radius
) const
{
	if (Radius <= 0)
	{
		return false;
	}

	const TArray<FIntPoint> NearbyCoords = Model->GetCoordsInRange(FIntPoint(Q, R), Radius);
	for (const FIntPoint& Coord : NearbyCoords)
	{
		const int32 TileIndex = Model->GetTileIndex(Coord.X, Coord.Y);
		if (Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].TileType == TileType)
		{
			return true;
		}
	}

	return false;
}

int32 FHexMapGenerator::CountLandTiles(const TArray<bool>& IsLand) const
{
	int32 LandTileCount = 0;
	for (const bool bIsLand : IsLand)
	{
		if (bIsLand)
		{
			++LandTileCount;
		}
	}

	return LandTileCount;
}
