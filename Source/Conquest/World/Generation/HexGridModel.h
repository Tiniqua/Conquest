#pragma once

#include "CoreMinimal.h"
#include "HexGridSettings.h"
#include "HexTileResourceData.h"
#include "HexResourceSetData.h"
#include "HexImprovementSetData.h"

class FHexGridModel
{
public:
	void Initialize(
		const FHexGridSizeSettings& InSize,
		const FHexHeightSettings& InHeight,
		const UHexTileResourceData* InTileData,
		const UHexResourceSetData* InResourceSetData,
		const UHexImprovementSetData* InImprovementSetData
	);

	TArray<FHexTileData>& GetMutableTiles() { return Tiles; }
	const TArray<FHexTileData>& GetTiles() const { return Tiles; }
	const TMap<FHexVertexKey, float>& GetResolvedVertexHeights() const { return ResolvedVertexHeights; }

	void ResolveTileHeights(const TArray<FHexTileGenerationRule>& GenerationRules);
	void ResolveSharedVertexHeights();
	void ResolveTileYields();

	bool SetTileImprovement(int32 Q, int32 R, FName ImprovementId);
	void GetPossibleImprovementsForTile(int32 Q, int32 R, TArray<const FHexImprovementDefinition*>& OutImprovements) const;
	void GetPossibleImprovementIdsForTile(int32 Q, int32 R, TArray<FName>& OutImprovementIds) const;

	FVector GetHexCenter(int32 Q, int32 R) const;
	FVector GetHexCornerOffset(int32 CornerIndex) const;
	FVector2D GetHexCornerUV(int32 CornerIndex) const;
	FHexVertexKey MakeVertexKey(const FVector& FlatPosition) const;

	float GetResolvedCornerHeight(const FVector& FlatCornerPosition) const;
	float GetHeightOffsetForTileType(EHexTileType TileType, const TArray<FHexTileGenerationRule>& GenerationRules) const;

	bool GetNeighbourCoord(int32 Q, int32 R, int32 Direction, int32& OutQ, int32& OutR) const;
	int32 GetTileIndex(int32 Q, int32 R) const;
	bool IsValidCoord(int32 Q, int32 R) const;
	bool IsValidTile(int32 Q, int32 R) const;
	bool IsWaterTileType(EHexTileType TileType) const;

	int32 GetGridWidth() const { return Size.GridWidth; }
	int32 GetGridHeight() const { return Size.GridHeight; }
	float GetHexRadius() const { return Size.HexRadius; }
	bool UsesHeightOffsets() const { return Height.bUseHeightOffsets; }
	
	bool GetCoordFromIndex(int32 TileIndex, int32& OutQ, int32& OutR) const;
	

private:
	FHexGridSizeSettings Size;
	FHexHeightSettings Height;
	TWeakObjectPtr<const UHexTileResourceData> TileData;
	TWeakObjectPtr<const UHexResourceSetData> ResourceSetData;
	TWeakObjectPtr<const UHexImprovementSetData> ImprovementSetData;
	TArray<FHexTileData> Tiles;
	TMap<FHexVertexKey, float> ResolvedVertexHeights;
};
