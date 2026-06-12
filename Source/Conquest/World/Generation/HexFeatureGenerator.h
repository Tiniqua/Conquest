#pragma once

#include "CoreMinimal.h"
#include "HexGridModel.h"
#include "HexGridSettings.h"
#include "HexTileResourceData.h"

class FHexFeatureGenerator
{
public:
	void Generate(
		FHexGridModel& InModel,
		const UHexTileResourceData* InTileData,
		const FHexFeatureGenerationSettings& InSettings,
		int32 RandomSeed
	);

private:
	FHexGridModel* Model = nullptr;
	TWeakObjectPtr<const UHexTileResourceData> TileData;
	FHexFeatureGenerationSettings Settings;

	void ClearGeneratedFeatures();
	void PlaceFeatureType(const FHexFeatureDefinition& FeatureDefinition, int32 TargetCount, FRandomStream& RandomStream);

	int32 ResolveTargetCount(const FHexFeatureDefinition& FeatureDefinition) const;

	bool CanPlaceFeatureOnTile(const FHexFeatureDefinition& FeatureDefinition, int32 TileIndex) const;
	bool PlaceFeatureOnTile(const FHexFeatureDefinition& FeatureDefinition, int32 TileIndex);

	int32 GrowFeatureClump(
		const FHexFeatureDefinition& FeatureDefinition,
		int32 SeedTileIndex,
		int32 TargetSize,
		FRandomStream& RandomStream
	);

	int32 PickBestFeatureSeedTile(
		const FHexFeatureDefinition& FeatureDefinition,
		FRandomStream& RandomStream
	) const;

	int32 PickBestFrontierTile(
		const FHexFeatureDefinition& FeatureDefinition,
		const TArray<int32>& Frontier,
		FRandomStream& RandomStream
	) const;

	float ScoreTileForFeature(
		const FHexFeatureDefinition& FeatureDefinition,
		int32 TileIndex
	) const;

	int32 CountAdjacentSameFeature(EHexFeatureType FeatureType, int32 TileIndex) const;
	void AddValidNeighboursToFrontier(const FHexFeatureDefinition& FeatureDefinition, int32 TileIndex, TArray<int32>& Frontier) const;

	int32 GetClumpSizeForPattern(const FHexFeatureDefinition& FeatureDefinition, FRandomStream& RandomStream) const;
};