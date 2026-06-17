#pragma once

#include "CoreMinimal.h"
#include "HexGridModel.h"

class CONQUEST_API FHexSimpleRiverGenerator
{
public:
	void Generate(
		FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		int32 Seed,
		TArray<FHexSimpleRiverPath>& OutRivers
	) const;

private:
	struct FWalkEdge
	{
		FIntPoint Tile = FIntPoint::ZeroValue;
		int32 EdgeIndex = 0;
		FHexVertexKey StartVertex;
		FHexVertexKey EndVertex;
		float Score = 0.0f;
	};

	bool TryGenerateRiver(
		FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		FRandomStream& RandomStream,
		int32 RiverId,
		const TSet<int64>& GloballyUsedEdges,
		const TSet<int32>& AvoidanceTiles,
		TMap<FIntPoint, int32>& RiverEdgeCountsByTile,
		FHexSimpleRiverPath& OutRiver
	) const;

	bool PickStartEdge(
		const FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		FRandomStream& RandomStream,
		const TSet<int64>& GloballyUsedEdges,
		const TSet<int32>& AvoidanceTiles,
		const TMap<FIntPoint, int32>& RiverEdgeCountsByTile,
		FWalkEdge& OutEdge
	) const;

	bool PickNextEdge(
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
	) const;

	void GetCandidateEdgesTouchingVertex(
		const FHexGridModel& Model,
		const FHexVertexKey& Vertex,
		const FIntPoint& SearchCenter,
		TArray<FWalkEdge>& OutEdges
	) const;

	bool IsCandidateAllowed(
		const FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		FRandomStream& RandomStream,
		const FWalkEdge& Candidate,
		const TSet<int64>& GlobalUsedEdges,
		const TSet<int64>& LocalUsedEdges,
		const TSet<FHexVertexKey>& LocalUsedVertices,
		const TSet<int32>& AvoidanceTiles,
		const TMap<FIntPoint, int32>& RiverEdgeCountsByTile
	) const;

	float ScoreCandidate(
		const FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		FRandomStream& RandomStream,
		const FWalkEdge& CurrentEdge,
		const FWalkEdge& Candidate,
		const TSet<int32>& AvoidanceTiles
	) const;

	bool BuildWalkEdge(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		const FHexVertexKey* RequiredStartVertex,
		FWalkEdge& OutEdge
	) const;

	bool IsEdgeTerrainAllowed(
		const FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		const FWalkEdge& Edge
	) const;

	bool WouldExceedMaxRiverEdgesPerTile(
		const FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		const FWalkEdge& Edge,
		const TMap<FIntPoint, int32>& RiverEdgeCountsByTile
	) const;

	void AddEdgeTileCounts(
		const FHexGridModel& Model,
		const FWalkEdge& Edge,
		TMap<FIntPoint, int32>& RiverEdgeCountsByTile
	) const;

	void AddAvoidanceTilesForRiver(
		const FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		const FHexSimpleRiverPath& River,
		TSet<int32>& InOutAvoidanceTiles
	) const;

	void MarkRiverTiles(
		FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		const FHexSimpleRiverPath& River
	) const;

	void MarkTilesTouchingRiverEdge(
		FHexGridModel& Model,
		const FHexSimpleRiverSettings& Settings,
		const FHexSimpleRiverEdge& Edge
	) const;

	bool GetEdgeVertexKeys(
		const FHexGridModel& Model,
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		FHexVertexKey& OutA,
		FHexVertexKey& OutB
	) const;

	int64 MakeCanonicalEdgeKey(const FHexGridModel& Model, const FWalkEdge& Edge) const;
	static int32 MakeTileKey(int32 Q, int32 R);
	static int64 MakeTileKey64(int32 Q, int32 R);
	static int32 GetOppositeEdgeIndex(int32 EdgeIndex);
};
