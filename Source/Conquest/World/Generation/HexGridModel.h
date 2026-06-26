#pragma once

#include "CoreMinimal.h"
#include "HexGridSettings.h"
#include "HexTileResourceData.h"
#include "HexResourceSetData.h"
#include "HexImprovementTypes.h"

class UDataTable;

class FHexGridModel
{
public:
	void Initialize(
		const FHexGridSizeSettings& InSize,
		const FHexHeightSettings& InHeight,
		const UHexTileResourceData* InTileData,
		const UHexResourceSetData* InResourceSetData,
		const UDataTable* InImprovementTable
	);

	void ResolveSharedVertexHeightVariance(
	const TArray<FHexTileGenerationRule>& GenerationRules,
	int32 RandomSeed
);

	float GetResolvedCornerHeightVarianceOffset(const FVector& FlatCornerPosition) const;

	float GetHeightVariancePercentForTileType(
		EHexTileType TileType,
		const TArray<FHexTileGenerationRule>& GenerationRules
	) const;

	TArray<FHexTileData>& GetMutableTiles() { return Tiles; }
	const TArray<FHexTileData>& GetTiles() const { return Tiles; }
	const TMap<FHexVertexKey, float>& GetResolvedVertexHeights() const { return ResolvedVertexHeights; }

	void ResolveTileHeights(const TArray<FHexTileGenerationRule>& GenerationRules);
	void ResolveSharedVertexHeights();
	void ResolveTileYields();

	FHexTileData* GetTileMutable(int32 Q, int32 R);
	FHexTileData* GetTileMutable(const FIntPoint& Coord);

	const FHexTileData* GetTile(int32 Q, int32 R) const;
	const FHexTileData* GetTile(const FIntPoint& Coord) const;

	bool SetTileImprovement(int32 Q, int32 R, FName ImprovementId);
	bool SetTileImprovementUnchecked(int32 Q, int32 R, FName ImprovementId);
	const FHexResourceDefinition* FindResourceDefinition(FName ResourceId) const;
	void GetPossibleImprovementsForTile(int32 Q, int32 R, TArray<const FHexImprovementDefinition*>& OutImprovements) const;
	void GetPossibleImprovementIdsForTile(int32 Q, int32 R, TArray<FName>& OutImprovementIds) const;
	void GetAllImprovementDefinitions(TArray<const FHexImprovementDefinition*>& OutImprovements) const;
	const FHexImprovementDefinition* FindImprovementDefinition(FName ImprovementId) const;

	FVector GetHexCenter(int32 Q, int32 R) const;
	FVector GetHexCornerOffset(int32 CornerIndex) const;
	FVector2D GetHexCornerUV(int32 CornerIndex) const;
	FHexVertexKey MakeVertexKey(const FVector& FlatPosition) const;

	float GetResolvedCornerHeight(const FVector& FlatCornerPosition) const;
	float GetHeightOffsetForTileType(EHexTileType TileType, const TArray<FHexTileGenerationRule>& GenerationRules) const;

	bool GetNeighbourCoord(int32 Q, int32 R, int32 Direction, int32& OutQ, int32& OutR) const;
	TArray<FIntPoint> GetCoordsInRange(const FIntPoint& Center, int32 Range) const;
	int32 GetHexDistance(const FIntPoint& A, const FIntPoint& B) const;
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
	TWeakObjectPtr<const UDataTable> ImprovementTable;
	TArray<FHexTileData> Tiles;
	TMap<FHexVertexKey, float> ResolvedVertexHeights;
	TMap<FHexVertexKey, float> ResolvedVertexHeightVarianceOffsets;
};
