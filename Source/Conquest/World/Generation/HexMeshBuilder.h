#pragma once

#include "CoreMinimal.h"
#include "HexGridModel.h"
#include "HexMeshTypes.h"

class UProceduralMeshComponent;
class UHexTileResourceData;

class FHexMeshBuilder
{
public:
	void BuildTerrainMesh(UProceduralMeshComponent* GridMesh, const FHexGridModel& Model, const UHexTileResourceData* TileData) const;
	void BuildWaterMesh(UProceduralMeshComponent* WaterMesh, const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, const UHexTileResourceData* TileData) const;
	void BuildGridOverlayMesh(UProceduralMeshComponent* OverlayMesh, const FHexGridModel& Model, const FHexOverlaySettings& OverlaySettings, const UHexTileResourceData* TileData) const;
	void BuildFogOfWarMesh(UProceduralMeshComponent* FogOfWarMesh, const FHexGridModel& GridModel, const FHexFogOfWarSettings& FogOfWarSettings);
	
	static int32 GetSectionIndexForTileType(EHexTileType TileType);
	static int32 GetSectionCount();

private:
	bool IsWaterLayerTargetTile(const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, int32 Q, int32 R) const;
	bool IsLandTileAdjacentToWater(const FHexGridModel& Model, int32 Q, int32 R) const;
	bool DoesAdjacentLandTileNeedWaterGapFill(const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, int32 Q, int32 R) const;
	bool ShouldDrawGridEdge(const FHexGridModel& Model, int32 Q, int32 R, int32 EdgeIndex) const;

	void AddHexTile(const FHexGridModel& Model, int32 Q, int32 R, TArray<FHexMeshSection>& MeshSections) const;
	void AddFlatWaterHex(const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, int32 Q, int32 R, FHexMeshSection& Section) const;
	void AddTriangle(FHexMeshSection& Section, const FVector& A, const FVector& B, const FVector& C, const FVector2D& UVA, const FVector2D& UVB, const FVector2D& UVC) const;
	void AddVertex(FHexMeshSection& Section, const FVector& Position, const FVector2D& UV, const FVector& Normal) const;
	void AddGridEdgeQuad(FHexMeshSection& Section, const FVector& A, const FVector& B, float Width) const;
};
