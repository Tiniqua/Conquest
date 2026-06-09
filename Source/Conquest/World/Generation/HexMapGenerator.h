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

	void BuildDesiredTileCounts(FRandomStream& RandomStream, TMap<EHexTileType, int32>& OutDesiredCounts) const;
	bool FindSeedTileForRule(const FHexTileGenerationRule& Rule, FRandomStream& RandomStream, const TArray<bool>& Assigned, int32& OutQ, int32& OutR) const;
	float ScoreSeedTileForRule(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;
	float ScoreTileForRuleAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;
	bool DoesTileSatisfyRequiredAdjacency(const FHexTileGenerationRule& Rule, int32 Q, int32 R, const TArray<bool>& Assigned) const;
	int32 GrowClump(const FHexTileGenerationRule& Rule, int32 SeedQ, int32 SeedR, int32 TargetSize, FRandomStream& RandomStream, TArray<bool>& Assigned);
	int32 PickBestFrontierIndex(const FHexTileGenerationRule& Rule, const TArray<FIntPoint>& Frontier, FRandomStream& RandomStream, const TArray<bool>& Assigned) const;
	EHexTileType PickWeightedLandTileType(FRandomStream& RandomStream) const;
};
