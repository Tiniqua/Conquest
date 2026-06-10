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
	void ApplyInlandSea(TArray<bool>& InOutIsLand) const;
	void ApplyCoastsFromLandMask(TArray<FHexTileData>& Tiles, const TArray<bool>& IsLand) const;

	void GenerateLandTerrain(FRandomStream& RandomStream, const TArray<bool>& IsLand);
	void BuildDesiredTileCounts(FRandomStream& RandomStream, TMap<EHexTileType, int32>& OutDesiredCounts, int32 AvailableTileCount) const;

	bool FindSeedTileForRule(const FHexTileGenerationRule& Rule, FRandomStream& RandomStream, const TArray<bool>& Assigned, int32& OutQ, int32& OutR) const;
	float ScoreSeedTileForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;
	float ScoreTileForRuleAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;

	float GetNormalizedTemperatureAtRow(int32 R) const;
	float GetTemperatureSuitabilityForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const;
	float ScoreTileForRuleTemperature(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const;
	bool DoesTileSatisfyTemperature(const FHexTileGenerationRule& Rule, int32 Q, int32 R) const;

	bool DoesTileSatisfyRequiredAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;
	int32 GrowClump(const FHexTileGenerationRule& Rule, int32 SeedQ, int32 SeedR, int32 TargetSize, FRandomStream& RandomStream, TArray<bool>& Assigned);
	int32 PickBestFrontierIndex(const FHexTileGenerationRule& Rule, const TArray<FIntPoint>& Frontier, FRandomStream& RandomStream, const TArray<bool>& Assigned) const;

	EHexTileType PickWeightedLandTileType(FRandomStream& RandomStream, int32 Q, int32 R) const;
};
