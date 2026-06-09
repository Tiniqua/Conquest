#include "SimpleHexGridActor.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"

ASimpleHexGridActor::ASimpleHexGridActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GridMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GridMesh"));
	GridMesh->SetupAttachment(SceneRoot);
	GridMesh->bUseAsyncCooking = true;
	GridMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	WaterMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterMesh"));
	WaterMesh->SetupAttachment(SceneRoot);
	WaterMesh->bUseAsyncCooking = true;
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterMesh->SetVisibility(bShowWaterLayer);
	WaterMesh->SetCastShadow(false);
	WaterMesh->bCastDynamicShadow = false;
	WaterMesh->bCastStaticShadow = false;
	WaterMesh->CastShadow = false;
	WaterMesh->TranslucencySortPriority = 1;

	HexGridOverlayMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HexGridOverlayMesh"));
	HexGridOverlayMesh->SetupAttachment(SceneRoot);
	HexGridOverlayMesh->bUseAsyncCooking = true;
	HexGridOverlayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HexGridOverlayMesh->SetVisibility(bShowHexGridOverlay);
	HexGridOverlayMesh->SetCastShadow(false);
	HexGridOverlayMesh->bCastDynamicShadow = false;
	HexGridOverlayMesh->bCastStaticShadow = false;
	HexGridOverlayMesh->CastShadow = false;
	HexGridOverlayMesh->TranslucencySortPriority = 2;

	SetupDefaultGenerationRules();
}

void ASimpleHexGridActor::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		RebuildGrid();
	}
}

void ASimpleHexGridActor::RebuildGrid()
{
	GenerateTileData();
	ResolveTileHeights();
	ResolveSharedVertexHeights();

	GenerateMesh();
	GenerateWaterMesh();
	GenerateGridOverlayMesh();
}

void ASimpleHexGridActor::SetHexGridOverlayVisible(bool bVisible)
{
	bShowHexGridOverlay = bVisible;

	if (HexGridOverlayMesh)
	{
		HexGridOverlayMesh->SetVisibility(bShowHexGridOverlay);
	}
}

void ASimpleHexGridActor::SetWaterLayerVisible(bool bVisible)
{
	bShowWaterLayer = bVisible;

	if (WaterMesh)
	{
		WaterMesh->SetVisibility(bShowWaterLayer);
	}
}

void ASimpleHexGridActor::SetupDefaultGenerationRules()
{
	GenerationRules.Reset();

	FSimpleHexTileGenerationRule Ocean;
	Ocean.TileType = ESimpleHexTileType::Ocean;
	Ocean.Weight = 8.0f;
	Ocean.MinClumpSize = 10;
	Ocean.MaxClumpSize = 28;
	Ocean.PreferredAdjacentTypes = {
		ESimpleHexTileType::Ocean,
		ESimpleHexTileType::Coast
	};
	Ocean.AvoidAdjacentTypes = {
		ESimpleHexTileType::Grassland,
		ESimpleHexTileType::Plains,
		ESimpleHexTileType::Desert,
		ESimpleHexTileType::Tundra,
		ESimpleHexTileType::Snow,
		ESimpleHexTileType::Mountain,
		ESimpleHexTileType::Lake
	};
	Ocean.bSoftCount = true;
	Ocean.bRejectBadAdjacency = true;
	Ocean.MinPlacementScore = 1.0f;
	Ocean.HeightOffset = -2.0f;
	GenerationRules.Add(Ocean);

	FSimpleHexTileGenerationRule Coast;
	Coast.TileType = ESimpleHexTileType::Coast;
	Coast.Weight = 5.0f;
	Coast.MinClumpSize = 3;
	Coast.MaxClumpSize = 10;
	Coast.PreferredAdjacentTypes = {
		ESimpleHexTileType::Ocean,
		ESimpleHexTileType::Grassland,
		ESimpleHexTileType::Plains
	};
	Coast.AvoidAdjacentTypes = {
		ESimpleHexTileType::Mountain,
		ESimpleHexTileType::Snow,
		ESimpleHexTileType::Lake
	};
	Coast.bSoftCount = true;
	Coast.bRejectBadAdjacency = true;
	Coast.MinPlacementScore = 2.0f;
	Coast.HeightOffset = -1.0f;
	GenerationRules.Add(Coast);

	FSimpleHexTileGenerationRule Grassland;
	Grassland.TileType = ESimpleHexTileType::Grassland;
	Grassland.Weight = 31.0f;
	Grassland.MinClumpSize = 8;
	Grassland.MaxClumpSize = 28;
	Grassland.PreferredAdjacentTypes = {
		ESimpleHexTileType::Grassland,
		ESimpleHexTileType::Plains,
		ESimpleHexTileType::Coast,
		ESimpleHexTileType::Lake
	};
	Grassland.AvoidAdjacentTypes = {
		ESimpleHexTileType::Ocean
	};
	Grassland.HeightOffset = 0.5f;
	GenerationRules.Add(Grassland);

	FSimpleHexTileGenerationRule Plains;
	Plains.TileType = ESimpleHexTileType::Plains;
	Plains.Weight = 27.0f;
	Plains.MinClumpSize = 8;
	Plains.MaxClumpSize = 26;
	Plains.PreferredAdjacentTypes = {
		ESimpleHexTileType::Plains,
		ESimpleHexTileType::Grassland,
		ESimpleHexTileType::Desert,
		ESimpleHexTileType::Lake
	};
	Plains.AvoidAdjacentTypes = {
		ESimpleHexTileType::Ocean
	};
	Plains.HeightOffset = 1.0f;
	GenerationRules.Add(Plains);

	FSimpleHexTileGenerationRule Desert;
	Desert.TileType = ESimpleHexTileType::Desert;
	Desert.Weight = 10.0f;
	Desert.MinClumpSize = 5;
	Desert.MaxClumpSize = 18;
	Desert.PreferredAdjacentTypes = {
		ESimpleHexTileType::Desert,
		ESimpleHexTileType::Plains
	};
	Desert.AvoidAdjacentTypes = {
		ESimpleHexTileType::Ocean,
		ESimpleHexTileType::Snow,
		ESimpleHexTileType::Tundra,
		ESimpleHexTileType::Lake
	};
	Desert.HeightOffset = 2.0f;
	GenerationRules.Add(Desert);

	FSimpleHexTileGenerationRule Tundra;
	Tundra.TileType = ESimpleHexTileType::Tundra;
	Tundra.Weight = 8.0f;
	Tundra.MinClumpSize = 5;
	Tundra.MaxClumpSize = 16;
	Tundra.PreferredAdjacentTypes = {
		ESimpleHexTileType::Tundra,
		ESimpleHexTileType::Snow,
		ESimpleHexTileType::Plains,
		ESimpleHexTileType::Lake
	};
	Tundra.AvoidAdjacentTypes = {
		ESimpleHexTileType::Ocean,
		ESimpleHexTileType::Desert
	};
	Tundra.HeightOffset = 0.0f;
	GenerationRules.Add(Tundra);

	FSimpleHexTileGenerationRule Snow;
	Snow.TileType = ESimpleHexTileType::Snow;
	Snow.Weight = 5.0f;
	Snow.MinClumpSize = 3;
	Snow.MaxClumpSize = 12;
	Snow.PreferredAdjacentTypes = {
		ESimpleHexTileType::Snow,
		ESimpleHexTileType::Tundra,
		ESimpleHexTileType::Mountain
	};
	Snow.AvoidAdjacentTypes = {
		ESimpleHexTileType::Ocean,
		ESimpleHexTileType::Desert,
		ESimpleHexTileType::Coast,
		ESimpleHexTileType::Lake
	};
	Snow.HeightOffset = 1.0f;
	GenerationRules.Add(Snow);

	FSimpleHexTileGenerationRule Lake;
	Lake.TileType = ESimpleHexTileType::Lake;
	Lake.Weight = 3.0f;
	Lake.MinClumpSize = 1;
	Lake.MaxClumpSize = 4;
	Lake.PreferredAdjacentTypes = {
		ESimpleHexTileType::Lake,
		ESimpleHexTileType::Grassland,
		ESimpleHexTileType::Plains,
		ESimpleHexTileType::Tundra
	};
	Lake.AvoidAdjacentTypes = {
		ESimpleHexTileType::Ocean,
		ESimpleHexTileType::Coast
	};
	Lake.RequiredAdjacentTypes = {
		ESimpleHexTileType::Grassland,
		ESimpleHexTileType::Plains,
		ESimpleHexTileType::Tundra
	};
	Lake.bSoftCount = true;
	Lake.bRejectBadAdjacency = true;
	Lake.MinPlacementScore = 1.0f;
	Lake.HeightOffset = -1.0f;
	GenerationRules.Add(Lake);

	FSimpleHexTileGenerationRule Mountain;
	Mountain.TileType = ESimpleHexTileType::Mountain;
	Mountain.Weight = 1.5f;
	Mountain.MinClumpSize = 2;
	Mountain.MaxClumpSize = 8;
	Mountain.PreferredAdjacentTypes = {
		ESimpleHexTileType::Mountain,
		ESimpleHexTileType::Snow,
		ESimpleHexTileType::Tundra
	};
	Mountain.AvoidAdjacentTypes = {
		ESimpleHexTileType::Ocean,
		ESimpleHexTileType::Coast,
		ESimpleHexTileType::Lake
	};
	Mountain.bSoftCount = true;
	Mountain.bRejectBadAdjacency = true;
	Mountain.MinPlacementScore = 0.5f;
	Mountain.HeightOffset = 5.0f;
	
	GenerationRules.Add(Mountain);
}

void ASimpleHexGridActor::GenerateTileData()
{
	Tiles.Reset();

	const int32 TotalTiles = GridWidth * GridHeight;
	if (TotalTiles <= 0)
	{
		return;
	}

	Tiles.SetNum(TotalTiles);

	TArray<bool> Assigned;
	Assigned.Init(false, TotalTiles);

	FRandomStream RandomStream(RandomSeed);

	if (GenerationRules.Num() <= 0)
	{
		for (FSimpleHexTileData& Tile : Tiles)
		{
			Tile.TileType = ESimpleHexTileType::Grassland;
		}

		return;
	}

	TMap<ESimpleHexTileType, int32> DesiredCounts;
	BuildDesiredTileCounts(RandomStream, DesiredCounts);

	for (const FSimpleHexTileGenerationRule& Rule : GenerationRules)
	{
		int32 RemainingForType = DesiredCounts.FindRef(Rule.TileType);
		int32 FailedClumpAttempts = 0;

		while (RemainingForType > 0)
		{
			const int32 MinClump = FMath::Max(1, Rule.MinClumpSize);
			const int32 MaxClump = FMath::Max(MinClump, Rule.MaxClumpSize);

			const int32 TargetClumpSize = FMath::Min(
				RemainingForType,
				RandomStream.RandRange(MinClump, MaxClump)
			);

			int32 SeedQ = 0;
			int32 SeedR = 0;

			if (!FindSeedTileForRule(Rule, RandomStream, Assigned, SeedQ, SeedR))
			{
				break;
			}

			const int32 PlacedCount = GrowClump(
				Rule,
				SeedQ,
				SeedR,
				TargetClumpSize,
				RandomStream,
				Assigned
			);

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

			if (PlacedCount < TargetClumpSize)
			{
				++FailedClumpAttempts;
			}
			else
			{
				FailedClumpAttempts = 0;
			}

			if (FailedClumpAttempts >= 12)
			{
				break;
			}
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

void ASimpleHexGridActor::ResolveTileHeights()
{
	for (FSimpleHexTileData& Tile : Tiles)
	{
		if (!bUseHeightOffsets)
		{
			Tile.Height = 0.0f;
			continue;
		}

		Tile.Height = GetHeightOffsetForTileType(Tile.TileType) * HeightScale;
	}
}

void ASimpleHexGridActor::ResolveSharedVertexHeights()
{
	ResolvedVertexHeights.Reset();

	TMap<FSimpleHexVertexKey, float> HeightSums;
	TMap<FSimpleHexVertexKey, int32> HeightCounts;

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			if (!IsValidTile(Q, R))
			{
				continue;
			}

			const int32 TileIndex = GetTileIndex(Q, R);
			const float TileHeight = Tiles[TileIndex].Height;

			const FVector FlatCenter = GetHexCenter(Q, R);

			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const FVector FlatCorner = FlatCenter + GetHexCornerOffset(CornerIndex);
				const FSimpleHexVertexKey VertexKey = MakeVertexKey(FlatCorner);

				HeightSums.FindOrAdd(VertexKey) += TileHeight;
				HeightCounts.FindOrAdd(VertexKey) += 1;
			}
		}
	}

	for (const TPair<FSimpleHexVertexKey, float>& Pair : HeightSums)
	{
		const FSimpleHexVertexKey& VertexKey = Pair.Key;
		const float HeightSum = Pair.Value;
		const int32 HeightCount = HeightCounts.FindRef(VertexKey);

		const float ResolvedHeight = HeightCount > 0
			? HeightSum / static_cast<float>(HeightCount)
			: 0.0f;

		ResolvedVertexHeights.Add(VertexKey, ResolvedHeight);
	}
}

void ASimpleHexGridActor::BuildDesiredTileCounts(
	FRandomStream& RandomStream,
	TMap<ESimpleHexTileType, int32>& OutDesiredCounts
) const
{
	OutDesiredCounts.Reset();

	const int32 TotalTiles = GridWidth * GridHeight;
	if (TotalTiles <= 0 || GenerationRules.Num() <= 0)
	{
		return;
	}

	TArray<float> AdjustedWeights;
	AdjustedWeights.SetNum(GenerationRules.Num());

	float TotalAdjustedWeight = 0.0f;

	for (int32 RuleIndex = 0; RuleIndex < GenerationRules.Num(); ++RuleIndex)
	{
		const FSimpleHexTileGenerationRule& Rule = GenerationRules[RuleIndex];

		const float BaseWeight = FMath::Max(0.0f, Rule.Weight);
		const float RandomScale = RandomStream.FRandRange(
			1.0f - CountRandomness,
			1.0f + CountRandomness
		);

		const float AdjustedWeight = FMath::Max(0.0f, BaseWeight * RandomScale);

		AdjustedWeights[RuleIndex] = AdjustedWeight;
		TotalAdjustedWeight += AdjustedWeight;
	}

	if (TotalAdjustedWeight <= 0.0f)
	{
		OutDesiredCounts.Add(ESimpleHexTileType::Grassland, TotalTiles);
		return;
	}

	int32 CountSoFar = 0;

	for (int32 RuleIndex = 0; RuleIndex < GenerationRules.Num(); ++RuleIndex)
	{
		const FSimpleHexTileGenerationRule& Rule = GenerationRules[RuleIndex];

		int32 Count = 0;

		if (RuleIndex == GenerationRules.Num() - 1)
		{
			Count = TotalTiles - CountSoFar;
		}
		else
		{
			const float Ratio = AdjustedWeights[RuleIndex] / TotalAdjustedWeight;
			Count = FMath::RoundToInt(static_cast<float>(TotalTiles) * Ratio);
			Count = FMath::Clamp(Count, 0, TotalTiles - CountSoFar);
		}

		OutDesiredCounts.FindOrAdd(Rule.TileType) += Count;
		CountSoFar += Count;
	}
}

bool ASimpleHexGridActor::FindSeedTileForRule(
	const FSimpleHexTileGenerationRule& Rule,
	FRandomStream& RandomStream,
	const TArray<bool>& Assigned,
	int32& OutQ,
	int32& OutR
) const
{
	const int32 TotalTiles = GridWidth * GridHeight;
	if (TotalTiles <= 0)
	{
		return false;
	}

	float BestScore = -1.0f;
	int32 BestIndex = INDEX_NONE;

	for (int32 Attempt = 0; Attempt < SeedSelectionAttempts; ++Attempt)
	{
		const int32 CandidateIndex = RandomStream.RandRange(0, TotalTiles - 1);

		if (!Assigned.IsValidIndex(CandidateIndex) || Assigned[CandidateIndex])
		{
			continue;
		}

		const int32 CandidateQ = CandidateIndex % GridWidth;
		const int32 CandidateR = CandidateIndex / GridWidth;

		const float Score = ScoreSeedTileForRule(
			Rule,
			CandidateQ,
			CandidateR,
			Assigned
		);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestIndex = CandidateIndex;
		}
	}

	if (BestIndex != INDEX_NONE)
	{
		if (!Rule.bRejectBadAdjacency || BestScore >= Rule.MinPlacementScore)
		{
			OutQ = BestIndex % GridWidth;
			OutR = BestIndex / GridWidth;
			return true;
		}
	}

	for (int32 Index = 0; Index < Assigned.Num(); ++Index)
	{
		if (!Assigned[Index])
		{
			const int32 CandidateQ = Index % GridWidth;
			const int32 CandidateR = Index / GridWidth;

			if (!DoesTileSatisfyRequiredAdjacency(Rule, CandidateQ, CandidateR, Assigned))
			{
				continue;
			}

			const float CandidateScore = ScoreSeedTileForRule(
				Rule,
				CandidateQ,
				CandidateR,
				Assigned
			);

			if (Rule.bRejectBadAdjacency && CandidateScore < Rule.MinPlacementScore)
			{
				continue;
			}

			OutQ = CandidateQ;
			OutR = CandidateR;
			return true;
		}
	}

	return false;
}

float ASimpleHexGridActor::ScoreSeedTileForRule(
	const FSimpleHexTileGenerationRule& Rule,
	int32 Q,
	int32 R,
	const TArray<bool>& Assigned
) const
{
	return ScoreTileForRuleAdjacency(Rule, Q, R, Assigned);
}

float ASimpleHexGridActor::ScoreTileForRuleAdjacency(
	const FSimpleHexTileGenerationRule& Rule,
	int32 Q,
	int32 R,
	const TArray<bool>& Assigned
) const
{
	if (!DoesTileSatisfyRequiredAdjacency(Rule, Q, R, Assigned))
	{
		return -100000.0f;
	}

	float Score = 1.0f;

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;

		if (!GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR))
		{
			continue;
		}

		const int32 NeighbourIndex = GetTileIndex(NeighbourQ, NeighbourR);

		if (!Tiles.IsValidIndex(NeighbourIndex) ||
			!Assigned.IsValidIndex(NeighbourIndex) ||
			!Assigned[NeighbourIndex])
		{
			continue;
		}

		const ESimpleHexTileType NeighbourType = Tiles[NeighbourIndex].TileType;

		if (NeighbourType == Rule.TileType)
		{
			Score += SameTypeAdjacencyBonus;
		}

		if (Rule.PreferredAdjacentTypes.Contains(NeighbourType))
		{
			Score += PreferredAdjacencyBonus;
		}

		if (Rule.AvoidAdjacentTypes.Contains(NeighbourType))
		{
			if (Rule.TileType == ESimpleHexTileType::Ocean)
			{
				Score -= HardAvoidAdjacencyPenalty;
			}
			else
			{
				Score -= AvoidAdjacencyPenalty;
			}
		}
	}

	return Score;
}

bool ASimpleHexGridActor::DoesTileSatisfyRequiredAdjacency(
	const FSimpleHexTileGenerationRule& Rule,
	int32 Q,
	int32 R,
	const TArray<bool>& Assigned
) const
{
	if (Rule.RequiredAdjacentTypes.Num() <= 0)
	{
		return true;
	}

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;

		if (!GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR))
		{
			continue;
		}

		const int32 NeighbourIndex = GetTileIndex(NeighbourQ, NeighbourR);

		if (!Tiles.IsValidIndex(NeighbourIndex) ||
			!Assigned.IsValidIndex(NeighbourIndex) ||
			!Assigned[NeighbourIndex])
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

int32 ASimpleHexGridActor::GrowClump(
	const FSimpleHexTileGenerationRule& Rule,
	int32 SeedQ,
	int32 SeedR,
	int32 TargetSize,
	FRandomStream& RandomStream,
	TArray<bool>& Assigned
)
{
	if (!IsValidCoord(SeedQ, SeedR) || TargetSize <= 0)
	{
		return 0;
	}

	TArray<FIntPoint> Frontier;
	Frontier.Add(FIntPoint(SeedQ, SeedR));

	int32 PlacedCount = 0;

	while (Frontier.Num() > 0 && PlacedCount < TargetSize)
	{
		const int32 ChosenFrontierIndex = PickBestFrontierIndex(
			Rule,
			Frontier,
			RandomStream,
			Assigned
		);

		if (!Frontier.IsValidIndex(ChosenFrontierIndex))
		{
			break;
		}

		const FIntPoint Coord = Frontier[ChosenFrontierIndex];
		Frontier.RemoveAtSwap(ChosenFrontierIndex);

		if (!IsValidCoord(Coord.X, Coord.Y))
		{
			continue;
		}

		const int32 TileIndex = GetTileIndex(Coord.X, Coord.Y);

		if (!Assigned.IsValidIndex(TileIndex) || Assigned[TileIndex])
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

			if (!GetNeighbourCoord(Coord.X, Coord.Y, Direction, NeighbourQ, NeighbourR))
			{
				continue;
			}

			const int32 NeighbourIndex = GetTileIndex(NeighbourQ, NeighbourR);

			if (Assigned.IsValidIndex(NeighbourIndex) && !Assigned[NeighbourIndex])
			{
				Frontier.AddUnique(FIntPoint(NeighbourQ, NeighbourR));
			}
		}
	}

	return PlacedCount;
}

int32 ASimpleHexGridActor::PickBestFrontierIndex(
	const FSimpleHexTileGenerationRule& Rule,
	const TArray<FIntPoint>& Frontier,
	FRandomStream& RandomStream,
	const TArray<bool>& Assigned
) const
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

		float Score = ScoreTileForRuleAdjacency(
			Rule,
			Coord.X,
			Coord.Y,
			Assigned
		);

		if (Rule.bRejectBadAdjacency && Score < Rule.MinPlacementScore)
		{
			Score = 0.0f;
		}
		else
		{
			Score = FMath::Max(0.05f, Score);
		}

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

ESimpleHexTileType ASimpleHexGridActor::PickWeightedLandTileType(FRandomStream& RandomStream) const
{
	float TotalWeight = 0.0f;

	for (const FSimpleHexTileGenerationRule& Rule : GenerationRules)
	{
		if (IsWaterTileType(Rule.TileType))
		{
			continue;
		}

		TotalWeight += FMath::Max(0.0f, Rule.Weight);
	}

	if (TotalWeight <= 0.0f)
	{
		return ESimpleHexTileType::Grassland;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);

	for (const FSimpleHexTileGenerationRule& Rule : GenerationRules)
	{
		if (IsWaterTileType(Rule.TileType))
		{
			continue;
		}

		Roll -= FMath::Max(0.0f, Rule.Weight);

		if (Roll <= 0.0f)
		{
			return Rule.TileType;
		}
	}

	return ESimpleHexTileType::Grassland;
}

float ASimpleHexGridActor::GetHeightOffsetForTileType(ESimpleHexTileType TileType) const
{
	for (const FSimpleHexTileGenerationRule& Rule : GenerationRules)
	{
		if (Rule.TileType == TileType)
		{
			return Rule.HeightOffset;
		}
	}

	return 0.0f;
}

float ASimpleHexGridActor::GetResolvedCornerHeight(const FVector& FlatCornerPosition) const
{
	const FSimpleHexVertexKey VertexKey = MakeVertexKey(FlatCornerPosition);

	if (const float* ResolvedHeight = ResolvedVertexHeights.Find(VertexKey))
	{
		return *ResolvedHeight;
	}

	return 0.0f;
}

float ASimpleHexGridActor::GetWaterSurfaceRenderZ() const
{
	return WaterSurfaceZ + WaterSurfaceRenderOffset;
}

bool ASimpleHexGridActor::IsWaterLayerTargetTile(int32 Q, int32 R) const
{
	if (!IsValidTile(Q, R))
	{
		return false;
	}

	const FSimpleHexTileData& Tile = Tiles[GetTileIndex(Q, R)];

	if (IsWaterTileType(Tile.TileType))
	{
		return true;
	}

	return DoesAdjacentLandTileNeedWaterGapFill(Q, R);
}

bool ASimpleHexGridActor::IsLandTileAdjacentToWater(int32 Q, int32 R) const
{
	if (!IsValidTile(Q, R))
	{
		return false;
	}

	const FSimpleHexTileData& Tile = Tiles[GetTileIndex(Q, R)];
	if (IsWaterTileType(Tile.TileType))
	{
		return false;
	}

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;

		if (!GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR))
		{
			continue;
		}

		if (!IsValidTile(NeighbourQ, NeighbourR))
		{
			continue;
		}

		const FSimpleHexTileData& NeighbourTile = Tiles[GetTileIndex(NeighbourQ, NeighbourR)];
		if (IsWaterTileType(NeighbourTile.TileType))
		{
			return true;
		}
	}

	return false;
}

bool ASimpleHexGridActor::DoesAdjacentLandTileNeedWaterGapFill(int32 Q, int32 R) const
{
	if (!bFillAdjacentSlopedLandWaterGaps || !IsLandTileAdjacentToWater(Q, R))
	{
		return false;
	}

	const int32 TileIndex = GetTileIndex(Q, R);
	if (!Tiles.IsValidIndex(TileIndex))
	{
		return false;
	}

	const FSimpleHexTileData& Tile = Tiles[TileIndex];
	const FVector FlatCenter = GetHexCenter(Q, R);

	float MinSurfaceZ = Tile.Height;
	float MaxSurfaceZ = Tile.Height;

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector FlatCorner = FlatCenter + GetHexCornerOffset(CornerIndex);
		const float CornerZ = bUseHeightOffsets
			? GetResolvedCornerHeight(FlatCorner)
			: 0.0f;

		MinSurfaceZ = FMath::Min(MinSurfaceZ, CornerZ);
		MaxSurfaceZ = FMath::Max(MaxSurfaceZ, CornerZ);
	}

	const bool bIsActuallySloped = (MaxSurfaceZ - MinSurfaceZ) >= WaterGapFillMinSlopeDelta;
	if (!bIsActuallySloped)
	{
		return false;
	}

	// If any part of the adjacent land fan has been averaged/tapered below the water plane,
	// the flat water overlay needs to extend into this tile to cover the visual hole.
	const float GapThresholdZ = WaterSurfaceZ - WaterGapFillBelowSurfaceTolerance;
	return MinSurfaceZ <= GapThresholdZ;
}

void ASimpleHexGridActor::GenerateMesh()
{
	if (!GridMesh)
	{
		return;
	}

	GridMesh->ClearAllMeshSections();

	TArray<FSimpleHexMeshSection> MeshSections;
	MeshSections.SetNum(GetSectionCount());

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			AddHexTile(Q, R, MeshSections);
		}
	}

	for (int32 SectionIndex = 0; SectionIndex < MeshSections.Num(); ++SectionIndex)
	{
		FSimpleHexMeshSection& Section = MeshSections[SectionIndex];

		GridMesh->CreateMeshSection(
			SectionIndex,
			Section.Vertices,
			Section.Triangles,
			Section.Normals,
			Section.UVs,
			Section.VertexColors,
			Section.Tangents,
			true
		);

		if (TileMaterials.IsValidIndex(SectionIndex) && TileMaterials[SectionIndex])
		{
			GridMesh->SetMaterial(SectionIndex, TileMaterials[SectionIndex]);
		}
	}
}

void ASimpleHexGridActor::GenerateWaterMesh()
{
	if (!WaterMesh)
	{
		return;
	}

	WaterMesh->ClearAllMeshSections();
	WaterMesh->SetVisibility(bShowWaterLayer);
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterMesh->SetCastShadow(false);
	WaterMesh->bCastDynamicShadow = false;
	WaterMesh->bCastStaticShadow = false;
	WaterMesh->CastShadow = false;
	WaterMesh->TranslucencySortPriority = 1;

	FSimpleHexMeshSection WaterSection;

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			if (!IsWaterLayerTargetTile(Q, R))
			{
				continue;
			}

			AddFlatWaterHex(Q, R, WaterSection);
		}
	}

	WaterMesh->CreateMeshSection(
		0,
		WaterSection.Vertices,
		WaterSection.Triangles,
		WaterSection.Normals,
		WaterSection.UVs,
		WaterSection.VertexColors,
		WaterSection.Tangents,
		false
	);

	if (WaterMaterial)
	{
		WaterMesh->SetMaterial(0, WaterMaterial);
	}

	WaterMesh->SetCastShadow(false);
	WaterMesh->bCastDynamicShadow = false;
	WaterMesh->bCastStaticShadow = false;
	WaterMesh->CastShadow = false;
}

void ASimpleHexGridActor::GenerateGridOverlayMesh()
{
	if (!HexGridOverlayMesh)
	{
		return;
	}

	HexGridOverlayMesh->ClearAllMeshSections();
	HexGridOverlayMesh->SetVisibility(bShowHexGridOverlay);
	HexGridOverlayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HexGridOverlayMesh->SetCastShadow(false);
	HexGridOverlayMesh->bCastDynamicShadow = false;
	HexGridOverlayMesh->bCastStaticShadow = false;
	HexGridOverlayMesh->CastShadow = false;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			const FVector FlatCenter = GetHexCenter(Q, R);

			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				if (!ShouldDrawGridEdge(Q, R, EdgeIndex))
				{
					continue;
				}

				const int32 AIndex = EdgeIndex;
				const int32 BIndex = (EdgeIndex + 1) % 6;

				const FVector FlatA = FlatCenter + GetHexCornerOffset(AIndex);
				const FVector FlatB = FlatCenter + GetHexCornerOffset(BIndex);

				const float HeightA = bUseHeightOffsets
					? GetResolvedCornerHeight(FlatA)
					: 0.0f;

				const float HeightB = bUseHeightOffsets
					? GetResolvedCornerHeight(FlatB)
					: 0.0f;

				const FVector A(FlatA.X, FlatA.Y, HeightA + GridOverlaySurfaceOffset);
				const FVector B(FlatB.X, FlatB.Y, HeightB + GridOverlaySurfaceOffset);

				AddGridEdgeQuad(
					Vertices,
					Triangles,
					Normals,
					UVs,
					VertexColors,
					Tangents,
					A,
					B,
					GridLineWidth
				);
			}
		}
	}

	HexGridOverlayMesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false
	);

	if (GridOverlayMaterial)
	{
		HexGridOverlayMesh->SetMaterial(0, GridOverlayMaterial);
	}

	HexGridOverlayMesh->SetCastShadow(false);
	HexGridOverlayMesh->bCastDynamicShadow = false;
	HexGridOverlayMesh->bCastStaticShadow = false;
	HexGridOverlayMesh->CastShadow = false;
}

void ASimpleHexGridActor::AddHexTile(
	int32 Q,
	int32 R,
	TArray<FSimpleHexMeshSection>& MeshSections
)
{
	if (!IsValidTile(Q, R))
	{
		return;
	}

	const int32 TileIndex = GetTileIndex(Q, R);
	const FSimpleHexTileData& Tile = Tiles[TileIndex];

	const int32 SectionIndex = GetSectionIndexForTileType(Tile.TileType);

	if (!MeshSections.IsValidIndex(SectionIndex))
	{
		return;
	}

	FSimpleHexMeshSection& Section = MeshSections[SectionIndex];

	const FVector FlatCenter = GetHexCenter(Q, R);
	const FVector Center = FVector(FlatCenter.X, FlatCenter.Y, Tile.Height);
	const FVector2D CenterUV(0.5f, 0.5f);

	TArray<FVector> Corners;
	TArray<FVector2D> CornerUVs;

	Corners.SetNum(6);
	CornerUVs.SetNum(6);

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector FlatCorner = FlatCenter + GetHexCornerOffset(CornerIndex);
		const float CornerHeight = bUseHeightOffsets
			? GetResolvedCornerHeight(FlatCorner)
			: 0.0f;

		Corners[CornerIndex] = FVector(
			FlatCorner.X,
			FlatCorner.Y,
			CornerHeight
		);

		CornerUVs[CornerIndex] = GetHexCornerUV(CornerIndex);
	}

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 NextCornerIndex = (CornerIndex + 1) % 6;

		AddTriangle(
			Section,
			Center,
			Corners[NextCornerIndex],
			Corners[CornerIndex],
			CenterUV,
			CornerUVs[NextCornerIndex],
			CornerUVs[CornerIndex]
		);
	}
}

void ASimpleHexGridActor::AddFlatWaterHex(
	int32 Q,
	int32 R,
	FSimpleHexMeshSection& Section
)
{
	const float WaterZ = GetWaterSurfaceRenderZ();
	const FVector FlatCenter = GetHexCenter(Q, R);
	const FVector Center(FlatCenter.X, FlatCenter.Y, WaterZ);
	const FVector2D CenterUV(0.5f, 0.5f);

	TArray<FVector> Corners;
	TArray<FVector2D> CornerUVs;

	Corners.SetNum(6);
	CornerUVs.SetNum(6);

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector FlatCorner = FlatCenter + GetHexCornerOffset(CornerIndex);

		Corners[CornerIndex] = FVector(
			FlatCorner.X,
			FlatCorner.Y,
			WaterZ
		);

		CornerUVs[CornerIndex] = GetHexCornerUV(CornerIndex);
	}

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 NextCornerIndex = (CornerIndex + 1) % 6;

		AddTriangle(
			Section,
			Center,
			Corners[NextCornerIndex],
			Corners[CornerIndex],
			CenterUV,
			CornerUVs[NextCornerIndex],
			CornerUVs[CornerIndex]
		);
	}
}

void ASimpleHexGridActor::AddTriangle(
	FSimpleHexMeshSection& Section,
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector2D& UVA,
	const FVector2D& UVB,
	const FVector2D& UVC
)
{
	FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();

	if (Normal.Z < 0.0f)
	{
		Normal *= -1.0f;
	}

	if (Normal.IsNearlyZero())
	{
		Normal = FVector::UpVector;
	}

	AddVertex(Section, A, UVA, Normal);
	AddVertex(Section, B, UVB, Normal);
	AddVertex(Section, C, UVC, Normal);
}

void ASimpleHexGridActor::AddVertex(
	FSimpleHexMeshSection& Section,
	const FVector& Position,
	const FVector2D& UV,
	const FVector& Normal
)
{
	const int32 NewIndex = Section.Vertices.Num();

	Section.Vertices.Add(Position);
	Section.Triangles.Add(NewIndex);
	Section.Normals.Add(Normal);
	Section.UVs.Add(UV);
	Section.VertexColors.Add(FColor::White);
	Section.Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
}

void ASimpleHexGridActor::AddGridEdgeQuad(
	TArray<FVector>& Vertices,
	TArray<int32>& Triangles,
	TArray<FVector>& Normals,
	TArray<FVector2D>& UVs,
	TArray<FColor>& VertexColors,
	TArray<FProcMeshTangent>& Tangents,
	const FVector& A,
	const FVector& B,
	float Width
)
{
	const FVector FlatDirection = FVector(
		B.X - A.X,
		B.Y - A.Y,
		0.0f
	).GetSafeNormal();

	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const FVector Perpendicular = FVector(
		-FlatDirection.Y,
		FlatDirection.X,
		0.0f
	) * (Width * 0.5f);

	const FVector V0 = A + Perpendicular;
	const FVector V1 = B + Perpendicular;
	const FVector V2 = B - Perpendicular;
	const FVector V3 = A - Perpendicular;

	const int32 StartIndex = Vertices.Num();

	Vertices.Add(V0);
	Vertices.Add(V1);
	Vertices.Add(V2);
	Vertices.Add(V3);

	Triangles.Add(StartIndex + 0);
	Triangles.Add(StartIndex + 1);
	Triangles.Add(StartIndex + 2);

	Triangles.Add(StartIndex + 0);
	Triangles.Add(StartIndex + 2);
	Triangles.Add(StartIndex + 3);

	for (int32 i = 0; i < 4; ++i)
	{
		Normals.Add(FVector::UpVector);
		VertexColors.Add(FColor::Black);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}

	UVs.Add(FVector2D(0.0f, 0.0f));
	UVs.Add(FVector2D(1.0f, 0.0f));
	UVs.Add(FVector2D(1.0f, 1.0f));
	UVs.Add(FVector2D(0.0f, 1.0f));
}

bool ASimpleHexGridActor::ShouldDrawGridEdge(int32 Q, int32 R, int32 EdgeIndex) const
{
	if (EdgeIndex == 3 || EdgeIndex == 4 || EdgeIndex == 5)
	{
		return true;
	}

	int32 NeighbourQ = 0;
	int32 NeighbourR = 0;

	return !GetNeighbourCoord(Q, R, EdgeIndex, NeighbourQ, NeighbourR);
}

FVector ASimpleHexGridActor::GetHexCenter(int32 Q, int32 R) const
{
	const float HexWidth = FMath::Sqrt(3.0f) * HexRadius;
	const float VerticalSpacing = 1.5f * HexRadius;

	const float X = HexWidth * (static_cast<float>(Q) + 0.5f * static_cast<float>(R & 1));
	const float Y = VerticalSpacing * static_cast<float>(R);

	return FVector(X, Y, 0.0f);
}

FVector ASimpleHexGridActor::GetHexCornerOffset(int32 CornerIndex) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	return FVector(
		HexRadius * FMath::Cos(AngleRad),
		HexRadius * FMath::Sin(AngleRad),
		0.0f
	);
}

FVector2D ASimpleHexGridActor::GetHexCornerUV(int32 CornerIndex) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	return FVector2D(
		0.5f + 0.5f * FMath::Cos(AngleRad),
		0.5f + 0.5f * FMath::Sin(AngleRad)
	);
}

FSimpleHexVertexKey ASimpleHexGridActor::MakeVertexKey(const FVector& FlatPosition) const
{
	const float SafePrecision = FMath::Max(1.0f, VertexKeyPrecision);

	FSimpleHexVertexKey Key;
	Key.X = FMath::RoundToInt(FlatPosition.X * SafePrecision);
	Key.Y = FMath::RoundToInt(FlatPosition.Y * SafePrecision);

	return Key;
}

bool ASimpleHexGridActor::GetNeighbourCoord(
	int32 Q,
	int32 R,
	int32 Direction,
	int32& OutQ,
	int32& OutR
) const
{
	const bool bOddRow = (R & 1) != 0;

	switch (Direction)
	{
	case 0:
		OutQ = bOddRow ? Q + 1 : Q;
		OutR = R - 1;
		break;

	case 1:
		OutQ = bOddRow ? Q : Q - 1;
		OutR = R - 1;
		break;

	case 2:
		OutQ = Q - 1;
		OutR = R;
		break;

	case 3:
		OutQ = bOddRow ? Q : Q - 1;
		OutR = R + 1;
		break;

	case 4:
		OutQ = bOddRow ? Q + 1 : Q;
		OutR = R + 1;
		break;

	case 5:
		OutQ = Q + 1;
		OutR = R;
		break;

	default:
		return false;
	}

	return IsValidCoord(OutQ, OutR);
}

int32 ASimpleHexGridActor::GetTileIndex(int32 Q, int32 R) const
{
	return R * GridWidth + Q;
}

bool ASimpleHexGridActor::IsValidCoord(int32 Q, int32 R) const
{
	return Q >= 0 && Q < GridWidth && R >= 0 && R < GridHeight;
}

bool ASimpleHexGridActor::IsValidTile(int32 Q, int32 R) const
{
	if (!IsValidCoord(Q, R))
	{
		return false;
	}

	return Tiles.IsValidIndex(GetTileIndex(Q, R));
}

bool ASimpleHexGridActor::IsWaterTileType(ESimpleHexTileType TileType) const
{
	return TileType == ESimpleHexTileType::Ocean ||
		TileType == ESimpleHexTileType::Coast ||
		TileType == ESimpleHexTileType::Lake;
}

int32 ASimpleHexGridActor::GetSectionIndexForTileType(ESimpleHexTileType TileType) const
{
	switch (TileType)
	{
	case ESimpleHexTileType::Grassland:
		return 0;

	case ESimpleHexTileType::Plains:
		return 1;

	case ESimpleHexTileType::Desert:
		return 2;

	case ESimpleHexTileType::Tundra:
		return 3;

	case ESimpleHexTileType::Snow:
		return 4;

	case ESimpleHexTileType::Coast:
		return 5;

	case ESimpleHexTileType::Ocean:
	case ESimpleHexTileType::Lake:
		return 6;

	case ESimpleHexTileType::Mountain:
		return 7;

	default:
		return 0;
	}
}

int32 ASimpleHexGridActor::GetSectionCount() const
{
	return 8;
}