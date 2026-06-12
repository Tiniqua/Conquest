#include "HexRiverGenerator.h"
#include "Containers/Queue.h"

DEFINE_LOG_CATEGORY_STATIC(LogHexRivers2, Log, All);

void FHexRiverGenerator::Generate(
	FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	int32 Seed
)
{
	Model.ClearRivers();

	if (!Settings.bGenerateRivers || Settings.RiverCount <= 0 || Settings.MaxRiverLength <= 0)
	{
		return;
	}

	FRandomStream RandomStream(Seed + 91273);

	TSet<int32> ExistingRiverAvoidanceTiles;
	TSet<int64> GloballyUsedEdges;

	const int32 TargetRiverCount = FMath::Max(0, Settings.RiverCount);
	const int32 MaxAttempts = FMath::Max(TargetRiverCount * 30, TargetRiverCount);
	const int32 RetryCountPerRiver = FMath::Max(1, Settings.MaxRiverBuildRetriesPerRiver);

	int32 AcceptedRiverCount = 0;

	for (int32 Attempt = 0; Attempt < MaxAttempts && AcceptedRiverCount < TargetRiverCount; ++Attempt)
	{
		if (RandomStream.FRand() > Settings.RiverStartChance)
		{
			continue;
		}

		bool bAcceptedThisRiver = false;

		for (int32 RetryIndex = 0; RetryIndex < RetryCountPerRiver; ++RetryIndex)
		{
			const bool bGenerated = TryGenerateSingleRiver(
				Model,
				Settings,
				RandomStream,
				AcceptedRiverCount,
				ExistingRiverAvoidanceTiles,
				GloballyUsedEdges
			);

			if (bGenerated)
			{
				bAcceptedThisRiver = true;

				UE_LOG(
					LogHexRivers2,
					Log,
					TEXT("RiverId=%d accepted after Retry=%d Attempt=%d"),
					AcceptedRiverCount,
					RetryIndex,
					Attempt
				);

				break;
			}
		}

		if (bAcceptedThisRiver)
		{
			++AcceptedRiverCount;
		}
	}

	UE_LOG(
		LogHexRivers2,
		Log,
		TEXT("River generation complete. Requested=%d Accepted=%d MaxAttempts=%d"),
		TargetRiverCount,
		AcceptedRiverCount,
		MaxAttempts
	);
}

bool FHexRiverGenerator::TryGenerateSingleRiver(
	FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	FRandomStream& RandomStream,
	int32 RiverId,
	TSet<int32>& ExistingRiverAvoidanceTiles,
	TSet<int64>& GloballyUsedEdges
) const
{
	FHexRiverCandidateEdge StartEdge;

	if (!PickStartEdge(
		Model,
		Settings,
		RandomStream,
		ExistingRiverAvoidanceTiles,
		GloballyUsedEdges,
		StartEdge
	))
	{
		UE_LOG(
			LogHexRivers2,
			Verbose,
			TEXT("RiverId=%d rejected: no valid start edge"),
			RiverId
		);

		return false;
	}

	const int32 SafeMinLength = FMath::Max(1, Settings.MinRiverLength);
	const int32 SafeMaxLength = FMath::Max(SafeMinLength, Settings.MaxRiverLength);
	const int32 TargetLength = RandomStream.RandRange(SafeMinLength, SafeMaxLength);

	TArray<FHexRiverCandidateEdge> Segments;
	TSet<int64> LocalUsedEdges;
	TSet<FHexVertexKey> LocalUsedVertices;
	TMap<int32, int32> LocalPlannedRiverEdgeCounts;

	Segments.Reserve(TargetLength);

	FHexRiverCandidateEdge CurrentSegment = StartEdge;
	EHexRiverStopReason StopReason = EHexRiverStopReason::Unknown;

	for (int32 Step = 0; Step < TargetLength; ++Step)
	{
		const int64 CanonicalEdgeKey = MakeCanonicalEdgeKey(
			Model,
			CurrentSegment.Q,
			CurrentSegment.R,
			CurrentSegment.EdgeIndex
		);

		if (!IsCandidateEdgeValid(
			Model,
			Settings,
			CurrentSegment.Q,
			CurrentSegment.R,
			CurrentSegment.EdgeIndex,
			LocalUsedEdges,
			GloballyUsedEdges,
			LocalPlannedRiverEdgeCounts
		))
		{
			StopReason = EHexRiverStopReason::Blocked;
			break;
		}

		Segments.Add(CurrentSegment);
		LocalUsedEdges.Add(CanonicalEdgeKey);
		LocalUsedVertices.Add(CurrentSegment.StartVertex);
		LocalUsedVertices.Add(CurrentSegment.EndVertex);

		AddSegmentToLocalPlannedCounts(
			Model,
			CurrentSegment,
			LocalPlannedRiverEdgeCounts
		);

		if (Step == TargetLength - 1)
		{
			StopReason = EHexRiverStopReason::ReachedTargetLength;
			break;
		}

		int32 NQ = 0;
		int32 NR = 0;

		const bool bHasNeighbour =
			Model.GetNeighbourCoord(
				CurrentSegment.Q,
				CurrentSegment.R,
				CurrentSegment.EdgeIndex,
				NQ,
				NR
			) &&
			Model.IsValidTile(NQ, NR);

		if (!bHasNeighbour)
		{
			StopReason = EHexRiverStopReason::ReachedMapEdge;
			break;
		}

		const int32 NeighbourIndex = Model.GetTileIndex(NQ, NR);

		if (Model.GetTiles().IsValidIndex(NeighbourIndex))
		{
			const FHexTileData& NeighbourTile = Model.GetTiles()[NeighbourIndex];

			if (Model.IsRiverEndTileType(NeighbourTile.TileType, Settings))
			{
				StopReason = EHexRiverStopReason::ReachedWater;
				break;
			}
		}

		FHexRiverCandidateEdge NextSegment;

		if (!PickNextEdgeFromVertex(
			Model,
			Settings,
			RandomStream,
			CurrentSegment.EndVertex,
			LocalUsedEdges,
			LocalUsedVertices,
			GloballyUsedEdges,
			ExistingRiverAvoidanceTiles,
			LocalPlannedRiverEdgeCounts,
			NextSegment
		))
		{
			StopReason = EHexRiverStopReason::Blocked;
			break;
		}

		CurrentSegment = NextSegment;
	}

	if (StopReason == EHexRiverStopReason::Unknown)
	{
		StopReason = EHexRiverStopReason::Blocked;
	}

	if (Segments.Num() < SafeMinLength)
	{
		UE_LOG(
			LogHexRivers2,
			Verbose,
			TEXT("RiverId=%d rejected: too short. Segments=%d Min=%d Target=%d StopReason=%d"),
			RiverId,
			Segments.Num(),
			SafeMinLength,
			TargetLength,
			static_cast<int32>(StopReason)
		);

		return false;
	}

	const float CompletionRatio =
		static_cast<float>(Segments.Num()) /
		static_cast<float>(FMath::Max(1, TargetLength));

	if (StopReason == EHexRiverStopReason::Blocked &&
		CompletionRatio < Settings.MinBlockedRiverCompletionRatio)
	{
		UE_LOG(
			LogHexRivers2,
			Verbose,
			TEXT("RiverId=%d rejected: blocked too early. Segments=%d Target=%d Completion=%.2f Required=%.2f"),
			RiverId,
			Segments.Num(),
			TargetLength,
			CompletionRatio,
			Settings.MinBlockedRiverCompletionRatio
		);

		return false;
	}

	if (Settings.bRequireRiverToEndAtWaterOrMapEdge)
	{
		const bool bEndedAtValidEndpoint =
			StopReason == EHexRiverStopReason::ReachedWater ||
			StopReason == EHexRiverStopReason::ReachedMapEdge;

		if (!bEndedAtValidEndpoint)
		{
			const bool bAcceptEarlyStoppedRiver =
				RandomStream.FRand() <= Settings.EarlyStoppedRiverAcceptanceChance;

			if (!bAcceptEarlyStoppedRiver)
			{
				UE_LOG(
					LogHexRivers2,
					Verbose,
					TEXT("RiverId=%d rejected: did not end at water/map edge. Segments=%d Target=%d StopReason=%d AcceptanceChance=%.2f"),
					RiverId,
					Segments.Num(),
					TargetLength,
					static_cast<int32>(StopReason),
					Settings.EarlyStoppedRiverAcceptanceChance
				);

				return false;
			}
		}
	}

	UE_LOG(
		LogHexRivers2,
		Verbose,
		TEXT("RiverId=%d ACCEPTED: Segments=%d Target=%d Completion=%.2f StopReason=%d"),
		RiverId,
		Segments.Num(),
		TargetLength,
		CompletionRatio,
		static_cast<int32>(StopReason)
	);

	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		const FHexRiverCandidateEdge& Segment = Segments[SegmentIndex];

		int32 NQ = 0;
		int32 NR = 0;

		const bool bHasNeighbour =
			Model.GetNeighbourCoord(
				Segment.Q,
				Segment.R,
				Segment.EdgeIndex,
				NQ,
				NR
			) &&
			Model.IsValidTile(NQ, NR);

		const int64 SegmentCanonicalKey = MakeCanonicalEdgeKey(
			Model,
			Segment.Q,
			Segment.R,
			Segment.EdgeIndex
		);

		UE_LOG(
			LogHexRivers2,
			Verbose,
			TEXT("  RiverId=%d Segment=%d Tile=(%d,%d) Edge=%d Neighbour=(%d,%d) HasNeighbour=%s StartV=(%d,%d) EndV=(%d,%d) CanonicalKey=%lld"),
			RiverId,
			SegmentIndex,
			Segment.Q,
			Segment.R,
			Segment.EdgeIndex,
			NQ,
			NR,
			bHasNeighbour ? TEXT("true") : TEXT("false"),
			Segment.StartVertex.X,
			Segment.StartVertex.Y,
			Segment.EndVertex.X,
			Segment.EndVertex.Y,
			static_cast<long long>(SegmentCanonicalKey)
		);
	}

	for (const FHexRiverCandidateEdge& Segment : Segments)
	{
		Model.SetRiverOnEdge(
			Segment.Q,
			Segment.R,
			Segment.EdgeIndex,
			RiverId,
			Settings.RiverWidth,
			Settings.RiverDepth
		);

		const int64 SegmentCanonicalKey = MakeCanonicalEdgeKey(
			Model,
			Segment.Q,
			Segment.R,
			Segment.EdgeIndex
		);

		GloballyUsedEdges.Add(SegmentCanonicalKey);
	}

	MarkRiverTilesAndFreshWater(Model, Segments);

	AddAvoidanceTilesAroundRiver(
		Model,
		Settings,
		Segments,
		ExistingRiverAvoidanceTiles
	);

	return true;
}

bool FHexRiverGenerator::IsRiverEdgeAllowedByVertexTerrain(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	int32 Q,
	int32 R,
	int32 EdgeIndex
) const
{
	if (!Model.IsValidTile(Q, R) || EdgeIndex < 0 || EdgeIndex >= 6)
	{
		return false;
	}

	const int32 CornerA = EdgeIndex;
	const int32 CornerB = (EdgeIndex + 1) % 6;

	const bool bCornerAAllowed = IsRiverCornerAllowed(
		Model,
		Settings,
		Q,
		R,
		CornerA
	);

	const bool bCornerBAllowed = IsRiverCornerAllowed(
		Model,
		Settings,
		Q,
		R,
		CornerB
	);

	if (!bCornerAAllowed || !bCornerBAllowed)
	{
		FHexVertexKey A;
		FHexVertexKey B;
		GetEdgeVertexKeys(Model, Q, R, EdgeIndex, A, B);

		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("River edge rejected by corner terrain Q=%d R=%d Edge=%d CornerA=%d A=(%d,%d) AAllowed=%s CornerB=%d B=(%d,%d) BAllowed=%s"),
			Q,
			R,
			EdgeIndex,
			CornerA,
			A.X,
			A.Y,
			bCornerAAllowed ? TEXT("true") : TEXT("false"),
			CornerB,
			B.X,
			B.Y,
			bCornerBAllowed ? TEXT("true") : TEXT("false")
		);
	}

	return bCornerAAllowed && bCornerBAllowed;
}

bool FHexRiverGenerator::PickStartEdge(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	FRandomStream& RandomStream,
	const TSet<int32>& ExistingRiverAvoidanceTiles,
	const TSet<int64>& GloballyUsedEdges,
	FHexRiverCandidateEdge& OutStartEdge
) const
{
	TArray<FHexRiverCandidateEdge> Candidates;

	TSet<int64> EmptyLocalUsedEdges;
	TMap<int32, int32> EmptyLocalPlannedRiverEdgeCounts;

	for (int32 R = 0; R < Model.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < Model.GetGridWidth(); ++Q)
		{
			if (!Model.IsValidTile(Q, R))
			{
				continue;
			}

			const int32 TileIndex = Model.GetTileIndex(Q, R);
			if (!Model.GetTiles().IsValidIndex(TileIndex))
			{
				continue;
			}

			const FHexTileData& Tile = Model.GetTiles()[TileIndex];

			if (!Model.IsValidRiverPassThroughTileType(Tile.TileType, Settings))
			{
				continue;
			}

			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				if (!IsCandidateEdgeValid(
					Model,
					Settings,
					Q,
					R,
					EdgeIndex,
					EmptyLocalUsedEdges,
					GloballyUsedEdges,
					EmptyLocalPlannedRiverEdgeCounts
				))
				{
					continue;
				}

				FHexVertexKey A;
				FHexVertexKey B;

				if (!GetEdgeVertexKeys(Model, Q, R, EdgeIndex, A, B))
				{
					continue;
				}

				FHexRiverCandidateEdge Candidate;
				Candidate.Q = Q;
				Candidate.R = R;
				Candidate.EdgeIndex = EdgeIndex;

				if (RandomStream.RandRange(0, 1) == 0)
				{
					Candidate.StartVertex = A;
					Candidate.EndVertex = B;
				}
				else
				{
					Candidate.StartVertex = B;
					Candidate.EndVertex = A;
				}

				Candidate.Score = ScoreCandidateEdge(
					Model,
					Settings,
					Q,
					R,
					EdgeIndex,
					ExistingRiverAvoidanceTiles,
					EmptyLocalPlannedRiverEdgeCounts
				);

				if (Candidate.Score > 0.0f)
				{
					Candidates.Add(Candidate);
				}
			}
		}
	}

	if (Candidates.Num() <= 0)
	{
		return false;
	}

	float TotalScore = 0.0f;
	for (const FHexRiverCandidateEdge& Candidate : Candidates)
	{
		TotalScore += FMath::Max(0.0f, Candidate.Score);
	}

	if (TotalScore <= 0.0f)
	{
		return false;
	}

	float Pick = RandomStream.FRandRange(0.0f, TotalScore);

	for (const FHexRiverCandidateEdge& Candidate : Candidates)
	{
		Pick -= FMath::Max(0.0f, Candidate.Score);

		if (Pick <= 0.0f)
		{
			OutStartEdge = Candidate;
			return true;
		}
	}

	OutStartEdge = Candidates.Last();
	return true;
}

bool FHexRiverGenerator::PickNextEdgeFromVertex(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	FRandomStream& RandomStream,
	const FHexVertexKey& RequiredStartVertex,
	const TSet<int64>& LocalUsedEdges,
	const TSet<FHexVertexKey>& LocalUsedVertices,
	const TSet<int64>& GloballyUsedEdges,
	const TSet<int32>& ExistingRiverAvoidanceTiles,
	const TMap<int32, int32>& LocalPlannedRiverEdgeCounts,
	FHexRiverCandidateEdge& OutNextEdge
) const
{
	TArray<FHexRiverCandidateEdge> Candidates;

	// Brute force is acceptable for generation-time debugging.
	// Later, this can be optimized by prebuilding vertex -> edge adjacency.
	for (int32 R = 0; R < Model.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < Model.GetGridWidth(); ++Q)
		{
			if (!Model.IsValidTile(Q, R))
			{
				continue;
			}

			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				if (!EdgeTouchesVertex(Model, Q, R, EdgeIndex, RequiredStartVertex))
				{
					continue;
				}

				if (!IsCandidateEdgeValid(
					Model,
					Settings,
					Q,
					R,
					EdgeIndex,
					LocalUsedEdges,
					GloballyUsedEdges,
					LocalPlannedRiverEdgeCounts
				))
				{
					continue;
				}

				FHexRiverCandidateEdge Candidate;
				BuildCandidateEdge(
					Model,
					Q,
					R,
					EdgeIndex,
					RequiredStartVertex,
					Candidate
				);

				// Prevent local folding/clumping around the same vertex cluster.
				if (LocalUsedVertices.Contains(Candidate.EndVertex))
				{
					continue;
				}

				Candidate.Score = ScoreCandidateEdge(
					Model,
					Settings,
					Q,
					R,
					EdgeIndex,
					ExistingRiverAvoidanceTiles,
					LocalPlannedRiverEdgeCounts
				);

				if (Candidate.Score > 0.0f)
				{
					Candidates.Add(Candidate);
				}
			}
		}
	}

	if (Candidates.Num() <= 0)
	{
		return false;
	}

	float TotalScore = 0.0f;
	for (const FHexRiverCandidateEdge& Candidate : Candidates)
	{
		TotalScore += FMath::Max(0.0f, Candidate.Score);
	}

	if (TotalScore <= 0.0f)
	{
		return false;
	}

	float Pick = RandomStream.FRandRange(0.0f, TotalScore);

	for (const FHexRiverCandidateEdge& Candidate : Candidates)
	{
		Pick -= FMath::Max(0.0f, Candidate.Score);

		if (Pick <= 0.0f)
		{
			OutNextEdge = Candidate;
			return true;
		}
	}

	OutNextEdge = Candidates.Last();
	return true;
}

bool FHexRiverGenerator::IsCandidateEdgeValid(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	const TSet<int64>& LocalUsedEdges,
	const TSet<int64>& GloballyUsedEdges,
	const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
) const
{
	if (!Model.IsValidTile(Q, R) || EdgeIndex < 0 || EdgeIndex >= 6)
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: invalid tile/edge Q=%d R=%d Edge=%d"),
			Q,
			R,
			EdgeIndex
		);

		return false;
	}

	const int64 CanonicalEdgeKey = MakeCanonicalEdgeKey(Model, Q, R, EdgeIndex);

	if (LocalUsedEdges.Contains(CanonicalEdgeKey))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: local used edge Q=%d R=%d Edge=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	if (GloballyUsedEdges.Contains(CanonicalEdgeKey))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: global used edge Q=%d R=%d Edge=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	if (Model.HasRiverOnEdge(Q, R, EdgeIndex))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: model already has river Q=%d R=%d Edge=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	const int32 TileIndex = Model.GetTileIndex(Q, R);

	if (!Model.GetTiles().IsValidIndex(TileIndex))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: invalid tile index Q=%d R=%d Edge=%d TileIndex=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			TileIndex,
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	if (!IsRiverEdgeAllowedByVertexTerrain(Model, Settings, Q, R, EdgeIndex))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: invalid river vertex terrain Q=%d R=%d Edge=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	const FHexTileData& Tile = Model.GetTiles()[TileIndex];

	if (!Model.IsValidRiverPassThroughTileType(Tile.TileType, Settings))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: tile type cannot pass river Q=%d R=%d Edge=%d TileType=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			static_cast<int32>(Tile.TileType),
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	if (WouldExceedMaxRiverEdgesPerTile(
		Model,
		Settings,
		Q,
		R,
		EdgeIndex,
		LocalPlannedRiverEdgeCounts
	))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: max edges per tile Q=%d R=%d Edge=%d MainExisting=%d MainLocal=%d Max=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			Model.GetRiverEdgeCount(Q, R),
			GetLocalPlannedRiverEdgeCount(Q, R, LocalPlannedRiverEdgeCounts),
			FMath::Clamp(Settings.MaxRiverEdgesPerTile, 1, 6),
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	int32 NQ = 0;
	int32 NR = 0;

	const bool bHasNeighbour =
		Model.GetNeighbourCoord(Q, R, EdgeIndex, NQ, NR) &&
		Model.IsValidTile(NQ, NR);

	if (!bHasNeighbour)
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate accepted as map-edge river endpoint Q=%d R=%d Edge=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			static_cast<long long>(CanonicalEdgeKey)
		);

		return true;
	}

	const int32 NeighbourIndex = Model.GetTileIndex(NQ, NR);

	if (!Model.GetTiles().IsValidIndex(NeighbourIndex))
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: invalid neighbour index Q=%d R=%d Edge=%d NQ=%d NR=%d NeighbourIndex=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			NQ,
			NR,
			NeighbourIndex,
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	const FHexTileData& NeighbourTile = Model.GetTiles()[NeighbourIndex];

	const bool bNeighbourIsRiverEnd =
		Model.IsRiverEndTileType(NeighbourTile.TileType, Settings);

	const bool bNeighbourIsPassThrough =
		Model.IsValidRiverPassThroughTileType(NeighbourTile.TileType, Settings);

	if (!bNeighbourIsRiverEnd && !bNeighbourIsPassThrough)
	{
		UE_LOG(
			LogHexRivers2,
			VeryVerbose,
			TEXT("Candidate rejected: neighbour cannot receive river Q=%d R=%d Edge=%d NQ=%d NR=%d NeighbourType=%d CanonicalKey=%lld"),
			Q,
			R,
			EdgeIndex,
			NQ,
			NR,
			static_cast<int32>(NeighbourTile.TileType),
			static_cast<long long>(CanonicalEdgeKey)
		);

		return false;
	}

	return true;
}

float FHexRiverGenerator::ScoreCandidateEdge(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	const TSet<int32>& ExistingRiverAvoidanceTiles,
	const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
) const
{
	if (!Model.IsValidTile(Q, R))
	{
		return 0.0f;
	}

	if (WouldExceedMaxRiverEdgesPerTile(
		Model,
		Settings,
		Q,
		R,
		EdgeIndex,
		LocalPlannedRiverEdgeCounts
	))
	{
		return 0.0f;
	}

	const int32 TileIndex = Model.GetTileIndex(Q, R);
	if (!Model.GetTiles().IsValidIndex(TileIndex))
	{
		return 0.0f;
	}

	const FHexTileData& Tile = Model.GetTiles()[TileIndex];

	float Score = 1.0f;

	// Prefer higher terrain slightly.
	Score += FMath::Max(0.0f, Tile.Height) * 0.01f;

	const int32 ThisTileKey = MakeTileKey(Q, R);

	if (ExistingRiverAvoidanceTiles.Contains(ThisTileKey))
	{
		// If the user sets very strong avoidance, treat it as near-hard avoidance.
		if (Settings.ExistingRiverAvoidanceWeight <= 0.15f)
		{
			return 0.0f;
		}

		Score *= Settings.ExistingRiverAvoidanceWeight;
	}

	int32 NQ = 0;
	int32 NR = 0;

	if (Model.GetNeighbourCoord(Q, R, EdgeIndex, NQ, NR) && Model.IsValidTile(NQ, NR))
	{
		const int32 NeighbourTileKey = MakeTileKey(NQ, NR);

		if (ExistingRiverAvoidanceTiles.Contains(NeighbourTileKey))
		{
			if (Settings.ExistingRiverAvoidanceWeight <= 0.15f)
			{
				return 0.0f;
			}

			Score *= Settings.ExistingRiverAvoidanceWeight;
		}

		const int32 NeighbourIndex = Model.GetTileIndex(NQ, NR);
		if (Model.GetTiles().IsValidIndex(NeighbourIndex))
		{
			const FHexTileData& NeighbourTile = Model.GetTiles()[NeighbourIndex];

			if (Model.IsRiverEndTileType(NeighbourTile.TileType, Settings))
			{
				Score += 3.0f;
			}

			if (NeighbourTile.Height < Tile.Height)
			{
				Score += 1.5f;
			}
		}
	}

	return Score;
}

void FHexRiverGenerator::BuildCandidateEdge(
	const FHexGridModel& Model,
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	const FHexVertexKey& StartVertex,
	FHexRiverCandidateEdge& OutCandidate
) const
{
	FHexVertexKey A;
	FHexVertexKey B;

	GetEdgeVertexKeys(Model, Q, R, EdgeIndex, A, B);

	OutCandidate.Q = Q;
	OutCandidate.R = R;
	OutCandidate.EdgeIndex = EdgeIndex;
	OutCandidate.StartVertex = StartVertex;

	if (A == StartVertex)
	{
		OutCandidate.EndVertex = B;
	}
	else
	{
		OutCandidate.EndVertex = A;
	}
}

void FHexRiverGenerator::MarkRiverTilesAndFreshWater(
	FHexGridModel& Model,
	const TArray<FHexRiverCandidateEdge>& RiverSegments
) const
{
	TSet<FHexVertexKey> RiverVertices;

	for (const FHexRiverCandidateEdge& Segment : RiverSegments)
	{
		RiverVertices.Add(Segment.StartVertex);
		RiverVertices.Add(Segment.EndVertex);
	}

	TArray<FHexTileData>& Tiles = Model.GetMutableTiles();

	for (int32 R = 0; R < Model.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < Model.GetGridWidth(); ++Q)
		{
			if (!Model.IsValidTile(Q, R))
			{
				continue;
			}

			const int32 TileIndex = Model.GetTileIndex(Q, R);
			if (!Tiles.IsValidIndex(TileIndex))
			{
				continue;
			}

			const FVector Center = Model.GetHexCenter(Q, R);

			bool bUsesRiverVertex = false;

			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const FVector Corner = Center + Model.GetHexCornerOffset(CornerIndex);
				const FHexVertexKey CornerKey = Model.MakeVertexKey(Corner);

				if (RiverVertices.Contains(CornerKey))
				{
					bUsesRiverVertex = true;
					break;
				}
			}

			if (bUsesRiverVertex)
			{
				Tiles[TileIndex].bHasRiver = true;
				Tiles[TileIndex].bHasFreshWater = true;
			}
		}
	}
}

void FHexRiverGenerator::AddAvoidanceTilesAroundRiver(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	const TArray<FHexRiverCandidateEdge>& RiverSegments,
	TSet<int32>& InOutAvoidanceTiles
) const
{
	for (const FHexRiverCandidateEdge& Segment : RiverSegments)
	{
		AddTilesWithinRadius(
			Model,
			Segment.Q,
			Segment.R,
			Settings.RiverAvoidanceRadius,
			InOutAvoidanceTiles
		);

		int32 NQ = 0;
		int32 NR = 0;

		if (Model.GetNeighbourCoord(Segment.Q, Segment.R, Segment.EdgeIndex, NQ, NR) && Model.IsValidTile(NQ, NR))
		{
			AddTilesWithinRadius(
				Model,
				NQ,
				NR,
				Settings.RiverAvoidanceRadius,
				InOutAvoidanceTiles
			);
		}
	}
}

void FHexRiverGenerator::AddTilesWithinRadius(
	const FHexGridModel& Model,
	int32 StartQ,
	int32 StartR,
	int32 Radius,
	TSet<int32>& InOutTiles
) const
{
	if (!Model.IsValidTile(StartQ, StartR))
	{
		return;
	}

	TQueue<TPair<FIntPoint, int32>> Queue;
	TSet<int32> Visited;

	Queue.Enqueue(TPair<FIntPoint, int32>(FIntPoint(StartQ, StartR), 0));
	Visited.Add(MakeTileKey(StartQ, StartR));

	while (!Queue.IsEmpty())
	{
		TPair<FIntPoint, int32> Current;
		Queue.Dequeue(Current);

		const int32 Q = Current.Key.X;
		const int32 R = Current.Key.Y;
		const int32 Distance = Current.Value;

		InOutTiles.Add(MakeTileKey(Q, R));

		if (Distance >= Radius)
		{
			continue;
		}

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NQ = 0;
			int32 NR = 0;

			if (!Model.GetNeighbourCoord(Q, R, Direction, NQ, NR) || !Model.IsValidTile(NQ, NR))
			{
				continue;
			}

			const int32 NeighbourKey = MakeTileKey(NQ, NR);
			if (Visited.Contains(NeighbourKey))
			{
				continue;
			}

			Visited.Add(NeighbourKey);
			Queue.Enqueue(TPair<FIntPoint, int32>(FIntPoint(NQ, NR), Distance + 1));
		}
	}
}

int32 FHexRiverGenerator::GetLocalPlannedRiverEdgeCount(
	int32 Q,
	int32 R,
	const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
) const
{
	const int32 TileKey = MakeTileKey(Q, R);

	if (const int32* Count = LocalPlannedRiverEdgeCounts.Find(TileKey))
	{
		return *Count;
	}

	return 0;
}

bool FHexRiverGenerator::WouldExceedMaxRiverEdgesPerTile(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
) const
{
	if (!Model.IsValidTile(Q, R))
	{
		return true;
	}

	const int32 SafeMaxEdges = FMath::Clamp(Settings.MaxRiverEdgesPerTile, 1, 6);

	const int32 MainExistingCount = Model.GetRiverEdgeCount(Q, R);
	const int32 MainLocalCount = GetLocalPlannedRiverEdgeCount(Q, R, LocalPlannedRiverEdgeCounts);

	if ((MainExistingCount + MainLocalCount + 1) > SafeMaxEdges)
	{
		return true;
	}

	int32 NQ = 0;
	int32 NR = 0;

	if (Model.GetNeighbourCoord(Q, R, EdgeIndex, NQ, NR) && Model.IsValidTile(NQ, NR))
	{
		const int32 NeighbourExistingCount = Model.GetRiverEdgeCount(NQ, NR);
		const int32 NeighbourLocalCount = GetLocalPlannedRiverEdgeCount(NQ, NR, LocalPlannedRiverEdgeCounts);

		if ((NeighbourExistingCount + NeighbourLocalCount + 1) > SafeMaxEdges)
		{
			return true;
		}
	}

	return false;
}

void FHexRiverGenerator::AddSegmentToLocalPlannedCounts(
	const FHexGridModel& Model,
	const FHexRiverCandidateEdge& Segment,
	TMap<int32, int32>& LocalPlannedRiverEdgeCounts
) const
{
	const int32 MainTileKey = MakeTileKey(Segment.Q, Segment.R);
	LocalPlannedRiverEdgeCounts.FindOrAdd(MainTileKey)++;

	int32 NQ = 0;
	int32 NR = 0;

	if (Model.GetNeighbourCoord(Segment.Q, Segment.R, Segment.EdgeIndex, NQ, NR) && Model.IsValidTile(NQ, NR))
	{
		const int32 NeighbourTileKey = MakeTileKey(NQ, NR);
		LocalPlannedRiverEdgeCounts.FindOrAdd(NeighbourTileKey)++;
	}
}

FHexVertexKey FHexRiverGenerator::MakeVertexKey(
	const FHexGridModel& Model,
	const FVector& Position
) const
{
	return Model.MakeVertexKey(Position);
}

bool FHexRiverGenerator::GetEdgeVertexKeys(
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

bool FHexRiverGenerator::EdgeTouchesVertex(
	const FHexGridModel& Model,
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	const FHexVertexKey& VertexKey
) const
{
	FHexVertexKey A;
	FHexVertexKey B;

	if (!GetEdgeVertexKeys(Model, Q, R, EdgeIndex, A, B))
	{
		return false;
	}

	return A == VertexKey || B == VertexKey;
}

int32 FHexRiverGenerator::MakeTileKey(int32 Q, int32 R)
{
	return ((R & 0xFFFF) << 16) | (Q & 0xFFFF);
}

int64 FHexRiverGenerator::MakeTileKey64(int32 Q, int32 R)
{
	const uint64 UQ = static_cast<uint32>(Q);
	const uint64 UR = static_cast<uint32>(R);

	return static_cast<int64>((UR << 32) | UQ);
}

int64 FHexRiverGenerator::MakeCanonicalEdgeKeyFromParts(
	int32 Q,
	int32 R,
	int32 EdgeIndex
)
{
	const uint64 UQ = static_cast<uint32>(Q);
	const uint64 UR = static_cast<uint32>(R);
	const uint64 UE = static_cast<uint32>(EdgeIndex & 0xF);

	// Layout:
	// bits 63..36 = R
	// bits 35..4  = Q
	// bits 3..0   = EdgeIndex
	return static_cast<int64>((UR << 36) | (UQ << 4) | UE);
}

void FHexRiverGenerator::AddTouchingTileIfValid(
	const FHexGridModel& Model,
	int32 Q,
	int32 R,
	TSet<int32>& InOutSeenTiles,
	TArray<FIntPoint>& OutTiles
) const
{
	if (!Model.IsValidTile(Q, R))
	{
		return;
	}

	const int32 TileKey = MakeTileKey(Q, R);

	if (InOutSeenTiles.Contains(TileKey))
	{
		return;
	}

	InOutSeenTiles.Add(TileKey);
	OutTiles.Add(FIntPoint(Q, R));
}

void FHexRiverGenerator::GetTilesTouchingCorner(
	const FHexGridModel& Model,
	int32 Q,
	int32 R,
	int32 CornerIndex,
	TArray<FIntPoint>& OutTiles
) const
{
	OutTiles.Reset();

	if (!Model.IsValidTile(Q, R))
	{
		return;
	}

	const int32 SafeCornerIndex = ((CornerIndex % 6) + 6) % 6;

	// In your geometry, edge N runs from corner N to corner N + 1.
	// Therefore corner N touches edge N and edge N - 1.
	const int32 EdgeAfterCorner = SafeCornerIndex;
	const int32 EdgeBeforeCorner = (SafeCornerIndex + 5) % 6;

	TSet<int32> SeenTiles;

	AddTouchingTileIfValid(
		Model,
		Q,
		R,
		SeenTiles,
		OutTiles
	);

	int32 NQ = 0;
	int32 NR = 0;

	if (Model.GetNeighbourCoord(Q, R, EdgeBeforeCorner, NQ, NR))
	{
		AddTouchingTileIfValid(
			Model,
			NQ,
			NR,
			SeenTiles,
			OutTiles
		);
	}

	if (Model.GetNeighbourCoord(Q, R, EdgeAfterCorner, NQ, NR))
	{
		AddTouchingTileIfValid(
			Model,
			NQ,
			NR,
			SeenTiles,
			OutTiles
		);
	}
}

bool FHexRiverGenerator::IsRiverCornerAllowed(
	const FHexGridModel& Model,
	const FHexRiverGenerationSettings& Settings,
	int32 Q,
	int32 R,
	int32 CornerIndex
) const
{
	TArray<FIntPoint> TouchingTiles;
	GetTilesTouchingCorner(
		Model,
		Q,
		R,
		CornerIndex,
		TouchingTiles
	);

	if (TouchingTiles.Num() <= 0)
	{
		return false;
	}

	bool bTouchesLand = false;
	bool bTouchesCoast = false;
	bool bTouchesOcean = false;
	bool bTouchesLake = false;

	for (const FIntPoint& Coord : TouchingTiles)
	{
		const int32 TileIndex = Model.GetTileIndex(Coord.X, Coord.Y);

		if (!Model.GetTiles().IsValidIndex(TileIndex))
		{
			continue;
		}

		const FHexTileData& Tile = Model.GetTiles()[TileIndex];

		if (Tile.TileType == EHexTileType::Ocean)
		{
			bTouchesOcean = true;
		}
		else if (Tile.TileType == EHexTileType::Coast)
		{
			bTouchesCoast = true;
		}
		else if (Tile.TileType == EHexTileType::Lake)
		{
			bTouchesLake = true;
		}
		else if (!Model.IsWaterTileType(Tile.TileType))
		{
			bTouchesLand = true;
		}
	}

	// Strong rule from your requirement:
	// do not allow river geometry on any vertex touching ocean.
	if (Settings.bDisallowOceanVertexRivers && bTouchesOcean)
	{
		return false;
	}

	// Coast vertices are only valid when also touching land.
	// This allows rivers to reach the land/coast edge, but prevents underwater coast-only rivers.
	if (Settings.bAllowCoastVertexRiversOnlyWhenTouchingLand && bTouchesCoast && !bTouchesLand)
	{
		return false;
	}

	// Lake-only vertices should not create underwater lake rivers.
	// Land/lake vertices are still valid endpoints.
	if (bTouchesLake && !bTouchesLand && !bTouchesCoast)
	{
		return false;
	}

	return true;
}

int64 FHexRiverGenerator::MakeCanonicalEdgeKey(
	const FHexGridModel& Model,
	int32 Q,
	int32 R,
	int32 EdgeIndex
) const
{
	int32 NQ = 0;
	int32 NR = 0;

	if (!Model.GetNeighbourCoord(Q, R, EdgeIndex, NQ, NR) || !Model.IsValidTile(NQ, NR))
	{
		return MakeCanonicalEdgeKeyFromParts(Q, R, EdgeIndex);
	}

	const int64 ThisTileKey = MakeTileKey64(Q, R);
	const int64 NeighbourTileKey = MakeTileKey64(NQ, NR);

	if (ThisTileKey < NeighbourTileKey)
	{
		return MakeCanonicalEdgeKeyFromParts(Q, R, EdgeIndex);
	}

	const int32 OppositeEdgeIndex = FHexGridModel::GetOppositeEdgeIndex(EdgeIndex);

	return MakeCanonicalEdgeKeyFromParts(NQ, NR, OppositeEdgeIndex);
}