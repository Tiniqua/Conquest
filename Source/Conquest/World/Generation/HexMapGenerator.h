#pragma once

#include "CoreMinimal.h"
#include "HexGridModel.h"

class FHexMapGenerator
{
public:
	void Generate(FHexGridModel& InModel, const FHexGenerationSettings& InGenerationSettings);

	static void SetupDefaultGenerationRules(TArray<FHexTileGenerationRule>& OutRules);

private:
	FHexGridModel* Model = nullptr;
	FHexGenerationSettings Settings;

	void BuildLandWaterMask(FRandomStream& RandomStream, TArray<bool>& OutIsLand) const;
	void SeedMajorLandmasses(FRandomStream& RandomStream, TArray<bool>& InOutIsLand, int32 TargetLandTiles) const;
	void AddIslandChains(FRandomStream& RandomStream, TArray<bool>& InOutIsLand, int32 TargetLandTiles) const;
	void RefineLandWaterMask(FRandomStream& RandomStream, TArray<bool>& InOutIsLand) const;
	void ApplySoftOceanBorder(FRandomStream& RandomStream, TArray<bool>& InOutIsLand) const;
	void RestoreLandBudget(FRandomStream& RandomStream, TArray<bool>& InOutIsLand, int32 TargetLandTiles) const;
	void FillEnclosedWaterRegions(TArray<bool>& InOutIsLand) const;
	void ApplyInlandSea(TArray<bool>& InOutIsLand) const;
	void ApplyCoastsFromLandMask(TArray<FHexTileData>& Tiles, const TArray<bool>& IsLand) const;
	void ApplyLakes(FRandomStream& RandomStream, TArray<FHexTileData>& Tiles, const TArray<bool>& IsLand) const;

	void GenerateLandTerrain(FRandomStream& RandomStream, const TArray<bool>& IsLand);
	void BuildDesiredTileCounts(FRandomStream& RandomStream, TMap<EHexTileType, int32>& OutDesiredCounts, int32 AvailableTileCount) const;

	bool FindSeedTileForRule(const FHexTileGenerationRule& Rule, FRandomStream& RandomStream, const TArray<bool>& Assigned, int32& OutQ, int32& OutR) const;
	float ScoreSeedTileForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;
	float ScoreTileForRuleAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;

	float GetNormalizedTemperatureAtTile(int32 Q, int32 R) const;
	float GetDeterministicValueNoise2D(int32 Q, int32 R, int32 Salt) const;	float GetTemperatureSuitabilityForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const;
	float GetRawTemperatureAtTile(int32 Q, int32 R) const;
	float ScoreTileForRuleTemperature(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const;
	bool DoesTileSatisfyTemperature(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const;

	bool DoesTileSatisfyRequiredAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;
	int32 GrowClump(const FHexTileGenerationRule& Rule, int32 SeedQ, int32 SeedR, int32 TargetSize, FRandomStream& RandomStream, TArray<bool>& Assigned);
	int32 PickBestFrontierIndex(const FHexTileGenerationRule& Rule, const TArray<FIntPoint>& Frontier, FRandomStream& RandomStream, const TArray<bool>& Assigned) const;

	EHexTileType PickWeightedLandTileType(FRandomStream& RandomStream, int32 Q, int32 R) const;
	bool IsMapEdge(int32 Q, int32 R) const;
	bool IsReservedWaterTile(int32 Q, int32 R) const;
	float GetCenterBiasScore(int32 Q, int32 R) const;
	int32 CountLandNeighbours(int32 Q, int32 R, const TArray<bool>& IsLand) const;
	int32 CountWaterNeighbours(int32 Q, int32 R, const TArray<bool>& IsLand) const;
	bool IsNearLandOutsideSet(int32 Q, int32 R, const TArray<bool>& IsLand, const TSet<int32>& AllowedLandIndices, int32 Radius) const;
	bool IsNearTileType(int32 Q, int32 R, const TArray<FHexTileData>& Tiles, EHexTileType TileType, int32 Radius) const;
	int32 CountLandTiles(const TArray<bool>& IsLand) const;
};
