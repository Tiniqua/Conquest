#include "HexSimpleRiverGenerator.h"

void FHexSimpleRiverGenerator::Generate(
	FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	int32 Seed,
	TArray<FHexSimpleRiverPath>& OutRivers
) const
{
	OutRivers.Reset();

	for (FHexTileData& Tile : Model.GetMutableTiles())
	{
		Tile.bHasRiver = false;
	}

	if (!Settings.bGenerateRivers || Settings.RiverDensityPer100Tiles <= 0.0f)
	{
		return;
	}

	const int32 TileCount = Model.GetGridWidth() * Model.GetGridHeight();
	const int32 TargetRiverCount = FMath::Clamp(
		FMath::RoundToInt((static_cast<float>(TileCount) / 100.0f) * Settings.RiverDensityPer100Tiles),
		0,
		FMath::Max(0, Settings.MaxRiverCount)
	);

	if (TargetRiverCount <= 0)
	{
		return;
	}

	FRandomStream RandomStream(Seed ^ 0x51A7);
	TSet<int64> GloballyUsedEdges;
	TSet<int32> AvoidanceTiles;
	TMap<FIntPoint, int32> RiverEdgeCountsByTile;

	int32 RiverId = 1;
	int32 FailedAttempts = 0;
	const int32 MaxFailedAttempts = FMath::Max(TargetRiverCount * Settings.MaxStartAttemptsPerRiver, TargetRiverCount);

	while (OutRivers.Num() < TargetRiverCount && FailedAttempts < MaxFailedAttempts)
	{
		FHexSimpleRiverPath River;
		if (!TryGenerateRiver(
			Model,
			Settings,
			RandomStream,
			RiverId,
			GloballyUsedEdges,
			AvoidanceTiles,
			RiverEdgeCountsByTile,
			River
		))
		{
			++FailedAttempts;
			continue;
		}

		for (const FHexSimpleRiverEdge& Edge : River.Edges)
		{
			FWalkEdge WalkEdge;
			if (BuildWalkEdge(Model, Edge.Tile.X, Edge.Tile.Y, Edge.EdgeIndex, nullptr, WalkEdge))
			{
				GloballyUsedEdges.Add(MakeCanonicalEdgeKey(Model, WalkEdge));
			}
		}

		MarkRiverTiles(Model, River);
		AddAvoidanceTilesForRiver(Model, Settings, River, AvoidanceTiles);
		OutRivers.Add(River);
		++RiverId;
	}
}

bool FHexSimpleRiverGenerator::TryGenerateRiver(
	FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	FRandomStream& RandomStream,
	int32 RiverId,
	const TSet<int64>& GloballyUsedEdges,
	const TSet<int32>& AvoidanceTiles,
	TMap<FIntPoint, int32>& RiverEdgeCountsByTile,
	FHexSimpleRiverPath& OutRiver
) const
{
	const int32 SafeMinLength = FMath::Max(1, Settings.MinRiverLength);
	const int32 SafeMaxLength = FMath::Max(SafeMinLength, Settings.MaxRiverLength);
	const int32 TargetLength = RandomStream.RandRange(SafeMinLength, SafeMaxLength);

	FWalkEdge CurrentEdge;
	if (!PickStartEdge(
		Model,
		Settings,
		RandomStream,
		GloballyUsedEdges,
		AvoidanceTiles,
		RiverEdgeCountsByTile,
		CurrentEdge
	))
	{
		return false;
	}

	TArray<FWalkEdge> Edges;
	Edges.Reserve(TargetLength);
	Edges.Add(CurrentEdge);

	TMap<FIntPoint, int32> WorkingRiverEdgeCountsByTile = RiverEdgeCountsByTile;
	AddEdgeTileCounts(Model, CurrentEdge, WorkingRiverEdgeCountsByTile);

	TSet<int64> LocalUsedEdges;
	TSet<FHexVertexKey> LocalUsedVertices;
	LocalUsedEdges.Add(MakeCanonicalEdgeKey(Model, CurrentEdge));
	LocalUsedVertices.Add(CurrentEdge.StartVertex);
	LocalUsedVertices.Add(CurrentEdge.EndVertex);

	for (int32 Step = 1; Step < TargetLength; ++Step)
	{
		FWalkEdge NextEdge;
		if (!PickNextEdge(
			Model,
			Settings,
			RandomStream,
			CurrentEdge,
			GloballyUsedEdges,
			LocalUsedEdges,
			LocalUsedVertices,
			AvoidanceTiles,
			WorkingRiverEdgeCountsByTile,
			NextEdge
		))
		{
			break;
		}

		Edges.Add(NextEdge);
		LocalUsedEdges.Add(MakeCanonicalEdgeKey(Model, NextEdge));
		LocalUsedVertices.Add(NextEdge.EndVertex);
		AddEdgeTileCounts(Model, NextEdge, WorkingRiverEdgeCountsByTile);
		CurrentEdge = NextEdge;
	}

	if (Edges.Num() < SafeMinLength)
	{
		return false;
	}

	OutRiver.RiverId = RiverId;
	OutRiver.Edges.Reset(Edges.Num());

	for (const FWalkEdge& Edge : Edges)
	{
		FHexSimpleRiverEdge RiverEdge;
		RiverEdge.Tile = Edge.Tile;
		RiverEdge.EdgeIndex = Edge.EdgeIndex;
		OutRiver.Edges.Add(RiverEdge);
	}

	RiverEdgeCountsByTile = WorkingRiverEdgeCountsByTile;
	return true;
}

bool FHexSimpleRiverGenerator::PickStartEdge(
	const FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	FRandomStream& RandomStream,
	const TSet<int64>& GloballyUsedEdges,
	const TSet<int32>& AvoidanceTiles,
	const TMap<FIntPoint, int32>& RiverEdgeCountsByTile,
	FWalkEdge& OutEdge
) const
{
	const int32 Width = Model.GetGridWidth();
	const int32 Height = Model.GetGridHeight();
	const int32 MaxAttempts = FMath::Max(1, Settings.MaxStartAttemptsPerRiver);

	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const int32 Q = RandomStream.RandRange(0, Width - 1);
		const int32 R = RandomStream.RandRange(0, Height - 1);
		const int32 EdgeIndex = RandomStream.RandRange(0, 5);

		FWalkEdge Candidate;
		if (!BuildWalkEdge(Model, Q, R, EdgeIndex, nullptr, Candidate))
		{
			continue;
		}

		if (RandomStream.RandRange(0, 1) == 1)
		{
			Swap(Candidate.StartVertex, Candidate.EndVertex);
		}

		TSet<int64> EmptyLocalEdges;
		TSet<FHexVertexKey> EmptyLocalVertices;
		if (IsCandidateAllowed(
			Model,
			Settings,
			RandomStream,
			Candidate,
			GloballyUsedEdges,
			EmptyLocalEdges,
			EmptyLocalVertices,
			AvoidanceTiles,
			RiverEdgeCountsByTile
		))
		{
			OutEdge = Candidate;
			return true;
		}
	}

	return false;
}

bool FHexSimpleRiverGenerator::PickNextEdge(
	const FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	FRandomStream& RandomStream,
	const FWalkEdge& CurrentEdge,
	const TSet<int64>& GlobalUsedEdges,
	const TSet<int64>& LocalUsedEdges,
	const TSet<FHexVertexKey>& LocalUsedVertices,
	const TSet<int32>& AvoidanceTiles,
	const TMap<FIntPoint, int32>& RiverEdgeCountsByTile,
	FWalkEdge& OutEdge
) const
{
	TArray<FWalkEdge> Candidates;
	GetCandidateEdgesTouchingVertex(Model, CurrentEdge.EndVertex, CurrentEdge.Tile, Candidates);

	float BestScore = -FLT_MAX;
	bool bFound = false;

	for (FWalkEdge& Candidate : Candidates)
	{
		if (Candidate.StartVertex != CurrentEdge.EndVertex)
		{
			Swap(Candidate.StartVertex, Candidate.EndVertex);
		}

		if (Candidate.EndVertex == CurrentEdge.StartVertex)
		{
			continue;
		}

		if (!IsCandidateAllowed(
			Model,
			Settings,
			RandomStream,
			Candidate,
			GlobalUsedEdges,
			LocalUsedEdges,
			LocalUsedVertices,
			AvoidanceTiles,
			RiverEdgeCountsByTile
		))
		{
			continue;
		}

		const float Score = ScoreCandidate(Model, Settings, RandomStream, CurrentEdge, Candidate, AvoidanceTiles);
		if (Score > BestScore)
		{
			BestScore = Score;
			OutEdge = Candidate;
			bFound = true;
		}
	}

	return bFound;
}

void FHexSimpleRiverGenerator::GetCandidateEdgesTouchingVertex(
	const FHexGridModel& Model,
	const FHexVertexKey& Vertex,
	const FIntPoint& SearchCenter,
	TArray<FWalkEdge>& OutEdges
) const
{
	OutEdges.Reset();
	TSet<int64> SeenEdges;

	for (int32 R = SearchCenter.Y - 2; R <= SearchCenter.Y + 2; ++R)
	{
		for (int32 Q = SearchCenter.X - 2; Q <= SearchCenter.X + 2; ++Q)
		{
			if (!Model.IsValidTile(Q, R))
			{
				continue;
			}

			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				FWalkEdge Edge;
				if (!BuildWalkEdge(Model, Q, R, EdgeIndex, &Vertex, Edge))
				{
					continue;
				}

				const int64 EdgeKey = MakeCanonicalEdgeKey(Model, Edge);
				if (SeenEdges.Contains(EdgeKey))
				{
					continue;
				}

				SeenEdges.Add(EdgeKey);
				OutEdges.Add(Edge);
			}
		}
	}
}

bool FHexSimpleRiverGenerator::IsCandidateAllowed(
	const FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	FRandomStream& RandomStream,
	const FWalkEdge& Candidate,
	const TSet<int64>& GlobalUsedEdges,
	const TSet<int64>& LocalUsedEdges,
	const TSet<FHexVertexKey>& LocalUsedVertices,
	const TSet<int32>& AvoidanceTiles,
	const TMap<FIntPoint, int32>& RiverEdgeCountsByTile
) const
{
	const int64 EdgeKey = MakeCanonicalEdgeKey(Model, Candidate);
	if (GlobalUsedEdges.Contains(EdgeKey) || LocalUsedEdges.Contains(EdgeKey))
	{
		return false;
	}

	if (LocalUsedVertices.Contains(Candidate.EndVertex))
	{
		return false;
	}

	if (!IsEdgeTerrainAllowed(Model, Settings, Candidate))
	{
		return false;
	}

	if (WouldExceedMaxRiverEdgesPerTile(Model, Settings, Candidate, RiverEdgeCountsByTile))
	{
		return false;
	}

	const int32 MainTileKey = MakeTileKey(Candidate.Tile.X, Candidate.Tile.Y);
	if (AvoidanceTiles.Contains(MainTileKey) && RandomStream.FRand() > Settings.ExistingRiverAvoidanceChance)
	{
		return false;
	}

	int32 NQ = 0;
	int32 NR = 0;
	if (Model.GetNeighbourCoord(Candidate.Tile.X, Candidate.Tile.Y, Candidate.EdgeIndex, NQ, NR))
	{
		const int32 NeighbourTileKey = MakeTileKey(NQ, NR);
		if (AvoidanceTiles.Contains(NeighbourTileKey) && RandomStream.FRand() > Settings.ExistingRiverAvoidanceChance)
		{
			return false;
		}
	}

	return true;
}

float FHexSimpleRiverGenerator::ScoreCandidate(
	const FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	FRandomStream& RandomStream,
	const FWalkEdge& CurrentEdge,
	const FWalkEdge& Candidate,
	const TSet<int32>& AvoidanceTiles
) const
{
	const FVector CurrentStart = FVector(CurrentEdge.StartVertex.X, CurrentEdge.StartVertex.Y, 0.0f);
	const FVector CurrentEnd = FVector(CurrentEdge.EndVertex.X, CurrentEdge.EndVertex.Y, 0.0f);
	const FVector CandidateEnd = FVector(Candidate.EndVertex.X, Candidate.EndVertex.Y, 0.0f);

	const FVector CurrentDirection = (CurrentEnd - CurrentStart).GetSafeNormal();
	const FVector CandidateDirection = (CandidateEnd - CurrentEnd).GetSafeNormal();
	const float ForwardDot = FVector::DotProduct(CurrentDirection, CandidateDirection);

	float Score = RandomStream.FRandRange(0.0f, 1.0f + Settings.TurnBias);
	Score += FMath::Max(0.0f, ForwardDot) * Settings.ForwardBias;

	if (AvoidanceTiles.Contains(MakeTileKey(Candidate.Tile.X, Candidate.Tile.Y)))
	{
		Score *= FMath::Clamp(Settings.ExistingRiverAvoidanceChance, 0.05f, 1.0f);
	}

	int32 NQ = 0;
	int32 NR = 0;
	if (Model.GetNeighbourCoord(Candidate.Tile.X, Candidate.Tile.Y, Candidate.EdgeIndex, NQ, NR) &&
		AvoidanceTiles.Contains(MakeTileKey(NQ, NR)))
	{
		Score *= FMath::Clamp(Settings.ExistingRiverAvoidanceChance, 0.05f, 1.0f);
	}

	return Score;
}

bool FHexSimpleRiverGenerator::BuildWalkEdge(
	const FHexGridModel& Model,
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	const FHexVertexKey* RequiredStartVertex,
	FWalkEdge& OutEdge
) const
{
	if (!Model.IsValidTile(Q, R) || EdgeIndex < 0 || EdgeIndex >= 6)
	{
		return false;
	}

	FHexVertexKey A;
	FHexVertexKey B;
	if (!GetEdgeVertexKeys(Model, Q, R, EdgeIndex, A, B))
	{
		return false;
	}

	OutEdge.Tile = FIntPoint(Q, R);
	OutEdge.EdgeIndex = EdgeIndex;
	OutEdge.StartVertex = A;
	OutEdge.EndVertex = B;

	if (RequiredStartVertex)
	{
		if (B == *RequiredStartVertex)
		{
			Swap(OutEdge.StartVertex, OutEdge.EndVertex);
		}
		else if (A != *RequiredStartVertex)
		{
			return false;
		}
	}

	return true;
}

bool FHexSimpleRiverGenerator::IsEdgeTerrainAllowed(
	const FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	const FWalkEdge& Edge
) const
{
	const FHexTileData* Tile = Model.GetTile(Edge.Tile);
	if (!Tile || Settings.AvoidTileTypes.Contains(Tile->TileType))
	{
		return false;
	}

	int32 NQ = 0;
	int32 NR = 0;
	if (Model.GetNeighbourCoord(Edge.Tile.X, Edge.Tile.Y, Edge.EdgeIndex, NQ, NR))
	{
		const FHexTileData* NeighbourTile = Model.GetTile(NQ, NR);
		if (!NeighbourTile || Settings.AvoidTileTypes.Contains(NeighbourTile->TileType))
		{
			return false;
		}
	}

	return true;
}

bool FHexSimpleRiverGenerator::WouldExceedMaxRiverEdgesPerTile(
	const FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	const FWalkEdge& Edge,
	const TMap<FIntPoint, int32>& RiverEdgeCountsByTile
) const
{
	const int32 MaxEdges = FMath::Clamp(Settings.MaxRiverEdgesPerTile, 1, 6);
	if (RiverEdgeCountsByTile.FindRef(Edge.Tile) + 1 > MaxEdges)
	{
		return true;
	}

	int32 NQ = 0;
	int32 NR = 0;
	if (Model.GetNeighbourCoord(Edge.Tile.X, Edge.Tile.Y, Edge.EdgeIndex, NQ, NR))
	{
		if (RiverEdgeCountsByTile.FindRef(FIntPoint(NQ, NR)) + 1 > MaxEdges)
		{
			return true;
		}
	}

	return false;
}

void FHexSimpleRiverGenerator::AddEdgeTileCounts(
	const FHexGridModel& Model,
	const FWalkEdge& Edge,
	TMap<FIntPoint, int32>& RiverEdgeCountsByTile
) const
{
	RiverEdgeCountsByTile.FindOrAdd(Edge.Tile)++;

	int32 NQ = 0;
	int32 NR = 0;
	if (Model.GetNeighbourCoord(Edge.Tile.X, Edge.Tile.Y, Edge.EdgeIndex, NQ, NR))
	{
		RiverEdgeCountsByTile.FindOrAdd(FIntPoint(NQ, NR))++;
	}
}

void FHexSimpleRiverGenerator::AddAvoidanceTilesForRiver(
	const FHexGridModel& Model,
	const FHexSimpleRiverSettings& Settings,
	const FHexSimpleRiverPath& River,
	TSet<int32>& InOutAvoidanceTiles
) const
{
	const int32 Radius = FMath::Max(0, Settings.RiverAvoidanceRadius);
	if (Radius <= 0)
	{
		return;
	}

	for (const FHexSimpleRiverEdge& Edge : River.Edges)
	{
		const TArray<FIntPoint> NearbyTiles = Model.GetCoordsInRange(Edge.Tile, Radius);
		for (const FIntPoint& Coord : NearbyTiles)
		{
			InOutAvoidanceTiles.Add(MakeTileKey(Coord.X, Coord.Y));
		}
	}
}

void FHexSimpleRiverGenerator::MarkRiverTiles(FHexGridModel& Model, const FHexSimpleRiverPath& River) const
{
	for (const FHexSimpleRiverEdge& Edge : River.Edges)
	{
		if (FHexTileData* Tile = Model.GetTileMutable(Edge.Tile))
		{
			Tile->bHasRiver = true;
			Tile->bHasFreshWater = true;
		}

		int32 NQ = 0;
		int32 NR = 0;
		if (Model.GetNeighbourCoord(Edge.Tile.X, Edge.Tile.Y, Edge.EdgeIndex, NQ, NR))
		{
			if (FHexTileData* NeighbourTile = Model.GetTileMutable(NQ, NR))
			{
				NeighbourTile->bHasRiver = true;
				NeighbourTile->bHasFreshWater = true;
			}
		}
	}
}

bool FHexSimpleRiverGenerator::GetEdgeVertexKeys(
	const FHexGridModel& Model,
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	FHexVertexKey& OutA,
	FHexVertexKey& OutB
) const
{
	if (!Model.IsValidTile(Q, R) || EdgeIndex < 0 || EdgeIndex >= 6)
	{
		return false;
	}

	const FVector Center = Model.GetHexCenter(Q, R);
	const FVector A = Center + Model.GetHexCornerOffset(EdgeIndex);
	const FVector B = Center + Model.GetHexCornerOffset((EdgeIndex + 1) % 6);
	OutA = Model.MakeVertexKey(A);
	OutB = Model.MakeVertexKey(B);
	return true;
}

int64 FHexSimpleRiverGenerator::MakeCanonicalEdgeKey(const FHexGridModel& Model, const FWalkEdge& Edge) const
{
	const int64 ThisTileKey = MakeTileKey64(Edge.Tile.X, Edge.Tile.Y);

	int32 NQ = 0;
	int32 NR = 0;
	if (!Model.GetNeighbourCoord(Edge.Tile.X, Edge.Tile.Y, Edge.EdgeIndex, NQ, NR))
	{
		return (ThisTileKey << 3) | static_cast<int64>(Edge.EdgeIndex);
	}

	const int64 NeighbourTileKey = MakeTileKey64(NQ, NR);
	const int32 OppositeEdgeIndex = GetOppositeEdgeIndex(Edge.EdgeIndex);

	if (ThisTileKey < NeighbourTileKey)
	{
		return (ThisTileKey << 3) | static_cast<int64>(Edge.EdgeIndex);
	}

	return (NeighbourTileKey << 3) | static_cast<int64>(OppositeEdgeIndex);
}

int32 FHexSimpleRiverGenerator::MakeTileKey(int32 Q, int32 R)
{
	return ((R & 0xFFFF) << 16) | (Q & 0xFFFF);
}

int64 FHexSimpleRiverGenerator::MakeTileKey64(int32 Q, int32 R)
{
	return (static_cast<int64>(R) << 32) | static_cast<uint32>(Q);
}

int32 FHexSimpleRiverGenerator::GetOppositeEdgeIndex(int32 EdgeIndex)
{
	return (EdgeIndex + 3) % 6;
}
