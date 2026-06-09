#pragma once

#include "CoreMinimal.h"
#include "HexGridModel.h"
#include "HexResourceSetData.h"

class FHexResourceGenerator
{
public:
	void Generate(FHexGridModel& InModel, const UHexResourceSetData* InResourceSet, const FHexResourceGenerationSettings& InSettings, int32 RandomSeed);

private:
	FHexGridModel* Model = nullptr;
	TWeakObjectPtr<const UHexResourceSetData> ResourceSet;
	FHexResourceGenerationSettings Settings;

	void PlaceCategory(EHexResourceCategory Category, int32 TargetCount, FRandomStream& RandomStream);
	int32 ResolveTargetCount(EHexResourceCategory Category) const;
	const FHexResourceDefinition* PickWeightedResourceForTile(const TArray<const FHexResourceDefinition*>& Resources, const FHexTileData& Tile, FRandomStream& RandomStream) const;
	int32 RollQuantity(const FHexResourceDefinition& Resource, FRandomStream& RandomStream) const;
};
