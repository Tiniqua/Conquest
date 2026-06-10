#include "HexMeshBuilder.h"
#include "HexTileResourceData.h"
#include "ProceduralMeshComponent.h"

void FHexMeshBuilder::BuildTerrainMesh(UProceduralMeshComponent* GridMesh, const FHexGridModel& Model, const UHexTileResourceData* TileData) const
{
	if (!GridMesh) { return; }
	GridMesh->ClearAllMeshSections();

	TArray<FHexMeshSection> MeshSections;
	MeshSections.SetNum(GetSectionCount());

	for (int32 R = 0; R < Model.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < Model.GetGridWidth(); ++Q)
		{
			AddHexTile(Model, Q, R, MeshSections);
		}
	}

	for (int32 SectionIndex = 0; SectionIndex < MeshSections.Num(); ++SectionIndex)
	{
		FHexMeshSection& Section = MeshSections[SectionIndex];
		GridMesh->CreateMeshSection(SectionIndex, Section.Vertices, Section.Triangles, Section.Normals, Section.UVs, Section.VertexColors, Section.Tangents, true);

		if (TileData)
		{
			for (const FHexTileDefinition& Definition : TileData->TileDefinitions)
			{
				if (GetSectionIndexForTileType(Definition.TileType) == SectionIndex && Definition.Material)
				{
					GridMesh->SetMaterial(SectionIndex, Definition.Material);
					break;
				}
			}
		}
	}
}

void FHexMeshBuilder::BuildWaterMesh(UProceduralMeshComponent* WaterMesh, const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, const UHexTileResourceData* TileData) const
{
	if (!WaterMesh) { return; }
	WaterMesh->ClearAllMeshSections();
	WaterMesh->SetVisibility(WaterSettings.bShowWaterLayer);
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterMesh->SetCastShadow(false);
	WaterMesh->bCastDynamicShadow = false;
	WaterMesh->bCastStaticShadow = false;
	WaterMesh->CastShadow = false;
	WaterMesh->TranslucencySortPriority = 1;

	FHexMeshSection WaterSection;
	for (int32 R = 0; R < Model.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < Model.GetGridWidth(); ++Q)
		{
			if (IsWaterLayerTargetTile(Model, WaterSettings, Q, R))
			{
				AddFlatWaterHex(Model, WaterSettings, Q, R, WaterSection);
			}
		}
	}

	WaterMesh->CreateMeshSection(0, WaterSection.Vertices, WaterSection.Triangles, WaterSection.Normals, WaterSection.UVs, WaterSection.VertexColors, WaterSection.Tangents, false);
	if (TileData && TileData->WaterMaterial)
	{
		WaterMesh->SetMaterial(0, TileData->WaterMaterial);
	}
}

void FHexMeshBuilder::BuildGridOverlayMesh(UProceduralMeshComponent* OverlayMesh, const FHexGridModel& Model, const FHexOverlaySettings& OverlaySettings, const UHexTileResourceData* TileData) const
{
	if (!OverlayMesh) { return; }
	OverlayMesh->ClearAllMeshSections();
	OverlayMesh->SetVisibility(OverlaySettings.bShowHexGridOverlay);
	OverlayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlayMesh->SetCastShadow(false);
	OverlayMesh->bCastDynamicShadow = false;
	OverlayMesh->bCastStaticShadow = false;
	OverlayMesh->CastShadow = false;
	OverlayMesh->TranslucencySortPriority = 2;

	FHexMeshSection Section;
	for (int32 R = 0; R < Model.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < Model.GetGridWidth(); ++Q)
		{
			const FVector FlatCenter = Model.GetHexCenter(Q, R);
			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				if (!ShouldDrawGridEdge(Model, Q, R, EdgeIndex)) { continue; }
				const FVector FlatA = FlatCenter + Model.GetHexCornerOffset(EdgeIndex);
				const FVector FlatB = FlatCenter + Model.GetHexCornerOffset((EdgeIndex + 1) % 6);
				const float HeightA = Model.UsesHeightOffsets() ? Model.GetResolvedCornerHeight(FlatA) : 0.0f;
				const float HeightB = Model.UsesHeightOffsets() ? Model.GetResolvedCornerHeight(FlatB) : 0.0f;
				AddGridEdgeQuad(Section, FVector(FlatA.X, FlatA.Y, HeightA + OverlaySettings.GridOverlaySurfaceOffset), FVector(FlatB.X, FlatB.Y, HeightB + OverlaySettings.GridOverlaySurfaceOffset), OverlaySettings.GridLineWidth);
			}
		}
	}

	OverlayMesh->CreateMeshSection(0, Section.Vertices, Section.Triangles, Section.Normals, Section.UVs, Section.VertexColors, Section.Tangents, false);
	if (TileData && TileData->GridOverlayMaterial)
	{
		OverlayMesh->SetMaterial(0, TileData->GridOverlayMaterial);
	}
}

void FHexMeshBuilder::BuildFogOfWarMesh(
	UProceduralMeshComponent* FogOfWarMesh,
	const FHexGridModel& GridModel,
	const FHexFogOfWarSettings& FogOfWarSettings
)
{
	if (!FogOfWarMesh)
	{
		return;
	}

	FogOfWarMesh->ClearAllMeshSections();

	const int32 GridWidth = GridModel.GetGridWidth();
	const int32 GridHeight = GridModel.GetGridHeight();

	if (GridWidth <= 0 || GridHeight <= 0)
	{
		return;
	}

	float HighestTileHeight = 0.0f;

	const TArray<FHexTileData>& Tiles = GridModel.GetTiles();
	for (const FHexTileData& Tile : Tiles)
	{
		HighestTileHeight = FMath::Max(HighestTileHeight, Tile.Height);
	}

	const float FogZ = HighestTileHeight + FogOfWarSettings.HeightAboveHighestTile;
	const float SafeHexScale = FMath::Max(0.01f, FogOfWarSettings.HexScale);
	const float SafeUVScale = FMath::Max(0.01f, FogOfWarSettings.UVScale);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	const int32 TotalTiles = GridWidth * GridHeight;

	// 7 vertices per hex: center + 6 corners.
	// 6 triangles per hex.
	Vertices.Reserve(TotalTiles * 7);
	Triangles.Reserve(TotalTiles * 18);
	Normals.Reserve(TotalTiles * 7);
	UVs.Reserve(TotalTiles * 7);
	VertexColors.Reserve(TotalTiles * 7);
	Tangents.Reserve(TotalTiles * 7);

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			if (!GridModel.IsValidTile(Q, R))
			{
				continue;
			}

			const FVector FlatCenter = GridModel.GetHexCenter(Q, R);
			const FVector Center(FlatCenter.X, FlatCenter.Y, FogZ);

			const int32 CenterIndex = Vertices.Num();

			Vertices.Add(Center);
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(0.5f, 0.5f) * SafeUVScale);
			VertexColors.Add(FColor::White);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

			int32 CornerIndices[6];

			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const FVector CornerOffset = GridModel.GetHexCornerOffset(CornerIndex) * SafeHexScale;
				const FVector FlatCorner = FlatCenter + CornerOffset;
				const FVector Corner(FlatCorner.X, FlatCorner.Y, FogZ);

				const FVector2D BaseCornerUV = GridModel.GetHexCornerUV(CornerIndex);

				CornerIndices[CornerIndex] = Vertices.Num();

				Vertices.Add(Corner);
				Normals.Add(FVector::UpVector);
				UVs.Add(BaseCornerUV * SafeUVScale);
				VertexColors.Add(FColor::White);
				Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
			}

			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const int32 NextCornerIndex = (CornerIndex + 1) % 6;

				// Same winding style as your existing terrain/water fans:
				// center -> next corner -> current corner.
				Triangles.Add(CenterIndex);
				Triangles.Add(CornerIndices[NextCornerIndex]);
				Triangles.Add(CornerIndices[CornerIndex]);
			}
		}
	}

	FogOfWarMesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false
	);

	if (FogOfWarSettings.FogMaterial)
	{
		FogOfWarMesh->SetMaterial(0, FogOfWarSettings.FogMaterial);
	}

	FogOfWarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FogOfWarMesh->SetCastShadow(false);
	FogOfWarMesh->bCastDynamicShadow = false;
	FogOfWarMesh->bCastStaticShadow = false;
	FogOfWarMesh->CastShadow = false;
	FogOfWarMesh->TranslucencySortPriority = FogOfWarSettings.TranslucencySortPriority;
}

bool FHexMeshBuilder::IsWaterLayerTargetTile(const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, int32 Q, int32 R) const
{
	if (!Model.IsValidTile(Q, R)) { return false; }
	const FHexTileData& Tile = Model.GetTiles()[Model.GetTileIndex(Q, R)];
	return Model.IsWaterTileType(Tile.TileType) || DoesAdjacentLandTileNeedWaterGapFill(Model, WaterSettings, Q, R);
}

bool FHexMeshBuilder::IsLandTileAdjacentToWater(const FHexGridModel& Model, int32 Q, int32 R) const
{
	if (!Model.IsValidTile(Q, R)) { return false; }
	const FHexTileData& Tile = Model.GetTiles()[Model.GetTileIndex(Q, R)];
	if (Model.IsWaterTileType(Tile.TileType)) { return false; }

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NQ = 0;
		int32 NR = 0;
		if (!Model.GetNeighbourCoord(Q, R, Direction, NQ, NR) || !Model.IsValidTile(NQ, NR)) { continue; }
		if (Model.IsWaterTileType(Model.GetTiles()[Model.GetTileIndex(NQ, NR)].TileType)) { return true; }
	}
	return false;
}

bool FHexMeshBuilder::DoesAdjacentLandTileNeedWaterGapFill(const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, int32 Q, int32 R) const
{
	if (!WaterSettings.bFillAdjacentSlopedLandWaterGaps || !IsLandTileAdjacentToWater(Model, Q, R)) { return false; }
	const FHexTileData& Tile = Model.GetTiles()[Model.GetTileIndex(Q, R)];
	const FVector FlatCenter = Model.GetHexCenter(Q, R);

	float MinSurfaceZ = Tile.Height;
	float MaxSurfaceZ = Tile.Height;

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector FlatCorner = FlatCenter + Model.GetHexCornerOffset(CornerIndex);
		const float CornerZ = Model.UsesHeightOffsets() ? Model.GetResolvedCornerHeight(FlatCorner) : 0.0f;
		MinSurfaceZ = FMath::Min(MinSurfaceZ, CornerZ);
		MaxSurfaceZ = FMath::Max(MaxSurfaceZ, CornerZ);
	}

	if ((MaxSurfaceZ - MinSurfaceZ) < WaterSettings.WaterGapFillMinSlopeDelta) { return false; }
	return MinSurfaceZ <= (WaterSettings.WaterSurfaceZ - WaterSettings.WaterGapFillBelowSurfaceTolerance);
}

bool FHexMeshBuilder::ShouldDrawGridEdge(const FHexGridModel& Model, int32 Q, int32 R, int32 EdgeIndex) const
{
	if (EdgeIndex == 3 || EdgeIndex == 4 || EdgeIndex == 5) { return true; }
	int32 NQ = 0;
	int32 NR = 0;
	return !Model.GetNeighbourCoord(Q, R, EdgeIndex, NQ, NR);
}

void FHexMeshBuilder::AddHexTile(const FHexGridModel& Model, int32 Q, int32 R, TArray<FHexMeshSection>& MeshSections) const
{
	if (!Model.IsValidTile(Q, R)) { return; }
	const FHexTileData& Tile = Model.GetTiles()[Model.GetTileIndex(Q, R)];
	const int32 SectionIndex = GetSectionIndexForTileType(Tile.TileType);
	if (!MeshSections.IsValidIndex(SectionIndex)) { return; }

	FHexMeshSection& Section = MeshSections[SectionIndex];
	const FVector FlatCenter = Model.GetHexCenter(Q, R);
	const FVector Center(FlatCenter.X, FlatCenter.Y, Tile.Height);
	const FVector2D CenterUV(0.5f, 0.5f);

	TArray<FVector> Corners;
	TArray<FVector2D> CornerUVs;
	Corners.SetNum(6);
	CornerUVs.SetNum(6);

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector FlatCorner = FlatCenter + Model.GetHexCornerOffset(CornerIndex);
		const float CornerHeight = Model.UsesHeightOffsets() ? Model.GetResolvedCornerHeight(FlatCorner) : 0.0f;
		Corners[CornerIndex] = FVector(FlatCorner.X, FlatCorner.Y, CornerHeight);
		CornerUVs[CornerIndex] = Model.GetHexCornerUV(CornerIndex);
	}

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 NextCornerIndex = (CornerIndex + 1) % 6;
		AddTriangle(Section, Center, Corners[NextCornerIndex], Corners[CornerIndex], CenterUV, CornerUVs[NextCornerIndex], CornerUVs[CornerIndex]);
	}
}

void FHexMeshBuilder::AddFlatWaterHex(const FHexGridModel& Model, const FHexWaterSettings& WaterSettings, int32 Q, int32 R, FHexMeshSection& Section) const
{
	const float WaterZ = WaterSettings.WaterSurfaceZ + WaterSettings.WaterSurfaceRenderOffset;
	const FVector FlatCenter = Model.GetHexCenter(Q, R);
	const FVector Center(FlatCenter.X, FlatCenter.Y, WaterZ);
	const FVector2D CenterUV(0.5f, 0.5f);

	TArray<FVector> Corners;
	TArray<FVector2D> CornerUVs;
	Corners.SetNum(6);
	CornerUVs.SetNum(6);

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector FlatCorner = FlatCenter + Model.GetHexCornerOffset(CornerIndex);
		Corners[CornerIndex] = FVector(FlatCorner.X, FlatCorner.Y, WaterZ);
		CornerUVs[CornerIndex] = Model.GetHexCornerUV(CornerIndex);
	}

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 NextCornerIndex = (CornerIndex + 1) % 6;
		AddTriangle(Section, Center, Corners[NextCornerIndex], Corners[CornerIndex], CenterUV, CornerUVs[NextCornerIndex], CornerUVs[CornerIndex]);
	}
}

void FHexMeshBuilder::AddTriangle(FHexMeshSection& Section, const FVector& A, const FVector& B, const FVector& C, const FVector2D& UVA, const FVector2D& UVB, const FVector2D& UVC) const
{
	FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
	if (Normal.Z < 0.0f) { Normal *= -1.0f; }
	if (Normal.IsNearlyZero()) { Normal = FVector::UpVector; }
	AddVertex(Section, A, UVA, Normal);
	AddVertex(Section, B, UVB, Normal);
	AddVertex(Section, C, UVC, Normal);
}

void FHexMeshBuilder::AddVertex(FHexMeshSection& Section, const FVector& Position, const FVector2D& UV, const FVector& Normal) const
{
	const int32 NewIndex = Section.Vertices.Num();
	Section.Vertices.Add(Position);
	Section.Triangles.Add(NewIndex);
	Section.Normals.Add(Normal);
	Section.UVs.Add(UV);
	Section.VertexColors.Add(FColor::White);
	Section.Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
}

void FHexMeshBuilder::AddGridEdgeQuad(FHexMeshSection& Section, const FVector& A, const FVector& B, float Width) const
{
	const FVector FlatDirection(B.X - A.X, B.Y - A.Y, 0.0f);
	const FVector Direction = FlatDirection.GetSafeNormal();
	if (Direction.IsNearlyZero()) { return; }

	const FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
	const FVector HalfWidth = Perpendicular * (Width * 0.5f);

	const int32 StartIndex = Section.Vertices.Num();
	Section.Vertices.Add(A + HalfWidth);
	Section.Vertices.Add(B + HalfWidth);
	Section.Vertices.Add(B - HalfWidth);
	Section.Vertices.Add(A - HalfWidth);

	Section.Triangles.Append({ StartIndex + 0, StartIndex + 1, StartIndex + 2, StartIndex + 0, StartIndex + 2, StartIndex + 3 });
	for (int32 i = 0; i < 4; ++i)
	{
		Section.Normals.Add(FVector::UpVector);
		Section.VertexColors.Add(FColor::Black);
		Section.Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}
	Section.UVs.Append({ FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 1.0f), FVector2D(0.0f, 1.0f) });
}

int32 FHexMeshBuilder::GetSectionIndexForTileType(EHexTileType TileType)
{
	switch (TileType)
	{
	case EHexTileType::Grassland: return 0;
	case EHexTileType::Plains: return 1;
	case EHexTileType::Desert: return 2;
	case EHexTileType::Tundra: return 3;
	case EHexTileType::Snow: return 4;
	case EHexTileType::Coast: return 5;
	case EHexTileType::Ocean: return 6;
	case EHexTileType::Mountain: return 7;
	case EHexTileType::Lake: return 8;
	default: return 0;
	}
}

int32 FHexMeshBuilder::GetSectionCount()
{
	return 9;
}
