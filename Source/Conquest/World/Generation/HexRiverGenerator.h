#pragma once

#include "CoreMinimal.h"
#include "HexGridModel.h"


struct FHexRiverCandidateEdge
{
	int32 Q = 0;
	int32 R = 0;
	int32 EdgeIndex = 0;

	float Score = 1.0f;

	FHexVertexKey StartVertex;
	FHexVertexKey EndVertex;
};

enum class EHexRiverStopReason : uint8
{
	Unknown,
	ReachedTargetLength,
	ReachedWater,
	ReachedMapEdge,
	Blocked
};

class FHexRiverGenerator
{
public:
	void Generate(
		FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		int32 Seed
	);

private:
	bool TryGenerateSingleRiver(
		FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		FRandomStream& RandomStream,
		int32 RiverId,
		TSet<int32>& ExistingRiverAvoidanceTiles,
		TSet<int64>& GloballyUsedEdges
	) const;
	
	bool IsRiverEdgeAllowedByVertexTerrain(
		const FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		int32 Q,
		int32 R,
		int32 EdgeIndex
	) const;

	bool PickStartEdge(
		const FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		FRandomStream& RandomStream,
		const TSet<int32>& ExistingRiverAvoidanceTiles,
		const TSet<int64>& GloballyUsedEdges,
		FHexRiverCandidateEdge& OutStartEdge
	) const;

	bool PickNextEdgeFromVertex(
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
	) const;

	bool IsCandidateEdgeValid(
		const FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		const TSet<int64>& LocalUsedEdges,
		const TSet<int64>& GloballyUsedEdges,
		const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
	) const;

	float ScoreCandidateEdge(
		const FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		const TSet<int32>& ExistingRiverAvoidanceTiles,
		const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
	) const;

	void BuildCandidateEdge(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		const FHexVertexKey& StartVertex,
		FHexRiverCandidateEdge& OutCandidate
	) const;

	void MarkRiverTilesAndFreshWater(
		FHexGridModel& Model,
		const TArray<FHexRiverCandidateEdge>& RiverSegments
	) const;

	void AddAvoidanceTilesAroundRiver(
		const FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		const TArray<FHexRiverCandidateEdge>& RiverSegments,
		TSet<int32>& InOutAvoidanceTiles
	) const;

	void AddTilesWithinRadius(
		const FHexGridModel& Model,
		int32 StartQ,
		int32 StartR,
		int32 Radius,
		TSet<int32>& InOutTiles
	) const;

	int32 GetLocalPlannedRiverEdgeCount(
		int32 Q,
		int32 R,
		const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
	) const;

	bool WouldExceedMaxRiverEdgesPerTile(
		const FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		const TMap<int32, int32>& LocalPlannedRiverEdgeCounts
	) const;

	void AddSegmentToLocalPlannedCounts(
		const FHexGridModel& Model,
		const FHexRiverCandidateEdge& Segment,
		TMap<int32, int32>& LocalPlannedRiverEdgeCounts
	) const;

	FHexVertexKey MakeVertexKey(
		const FHexGridModel& Model,
		const FVector& Position
	) const;

	bool GetEdgeVertexKeys(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		FHexVertexKey& OutA,
		FHexVertexKey& OutB
	) const;

	bool EdgeTouchesVertex(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		const FHexVertexKey& VertexKey
	) const;

	static int32 MakeTileKey(int32 Q, int32 R);
	static int64 MakeTileKey64(int32 Q, int32 R);
	static int64 MakeCanonicalEdgeKeyFromParts(int32 Q, int32 R, int32 EdgeIndex);

	int64 MakeCanonicalEdgeKey(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		int32 EdgeIndex
	) const;

	void AddTouchingTileIfValid(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		TSet<int32>& InOutSeenTiles,
		TArray<FIntPoint>& OutTiles
	) const;

	void GetTilesTouchingCorner(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		int32 CornerIndex,
		TArray<FIntPoint>& OutTiles
	) const;

	bool IsRiverCornerAllowed(
		const FHexGridModel& Model,
		const FHexRiverGenerationSettings& Settings,
		int32 Q,
		int32 R,
		int32 CornerIndex
	) const;
};