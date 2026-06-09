#include "WorldGenProceduralActor.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogWorldGenProcedural, Log, All);

AWorldGenProceduralActor::AWorldGenProceduralActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WorldMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WorldMesh"));
	WorldMesh->SetupAttachment(SceneRoot);
	WorldMesh->bUseAsyncCooking = true;
	WorldMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AWorldGenProceduralActor::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		RebuildWorld();
	}
}

void AWorldGenProceduralActor::RebuildWorld()
{
	GenerateTileData();
	GenerateMesh();
}

void AWorldGenProceduralActor::GenerateTileData()
{
	Tiles.Reset();

	const int32 TotalTiles = GridWidth * GridHeight;
	if (TotalTiles <= 0)
	{
		return;
	}

	Tiles.SetNum(TotalTiles);

	FRandomStream RandomStream(RandomSeed);

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			EWorldGenTileType NewTileType = EWorldGenTileType::Grassland;

			const float WaterRoll = RandomStream.FRand();
			const float FeatureRoll = RandomStream.FRand();
			const float TerrainRoll = RandomStream.FRand();

			if (WaterRoll < WaterChance)
			{
				NewTileType = TerrainRoll < 0.35f
					? EWorldGenTileType::Ocean
					: EWorldGenTileType::Coast;
			}
			else if (FeatureRoll < MountainChance)
			{
				NewTileType = EWorldGenTileType::Mountain;
			}
			else
			{
				EWorldGenBaseTerrain BaseTerrain = EWorldGenBaseTerrain::Grassland;

				if (TerrainRoll < 0.28f)
				{
					BaseTerrain = EWorldGenBaseTerrain::Grassland;
				}
				else if (TerrainRoll < 0.50f)
				{
					BaseTerrain = EWorldGenBaseTerrain::Plains;
				}
				else if (TerrainRoll < 0.68f)
				{
					BaseTerrain = EWorldGenBaseTerrain::Desert;
				}
				else if (TerrainRoll < 0.86f)
				{
					BaseTerrain = EWorldGenBaseTerrain::Tundra;
				}
				else
				{
					BaseTerrain = EWorldGenBaseTerrain::Snow;
				}

				const bool bMakeHill = RandomStream.FRand() < HillChance;

				NewTileType = bMakeHill
					? FWorldGenRuleTypes::GetHillVariantForBaseTerrain(BaseTerrain)
					: FWorldGenRuleTypes::GetFlatVariantForBaseTerrain(BaseTerrain);
			}

			Tiles[GetTileIndex(Q, R)].TileType = NewTileType;
		}
	}
}

void AWorldGenProceduralActor::GenerateMesh()
{
	if (!WorldMesh)
	{
		return;
	}

	WorldMesh->ClearAllMeshSections();

	const int32 SectionCount = FWorldGenRuleTypes::GetAllTileTypes().Num();

	TArray<FWorldGenMeshSectionData> MeshSections;
	MeshSections.SetNum(SectionCount);

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			AddTileGeometry(Q, R, MeshSections);
		}
	}

	for (int32 SectionIndex = 0; SectionIndex < MeshSections.Num(); ++SectionIndex)
	{
		FWorldGenMeshSectionData& Section = MeshSections[SectionIndex];

		WorldMesh->CreateMeshSection(
			SectionIndex,
			Section.Vertices,
			Section.Triangles,
			Section.Normals,
			Section.UVs,
			Section.VertexColors,
			Section.Tangents,
			false
		);

		if (TileMaterials.IsValidIndex(SectionIndex))
		{
			WorldMesh->SetMaterial(SectionIndex, TileMaterials[SectionIndex]);
		}
	}

	if (bLogGenerationDebug)
	{
		LogGenerationDebug(MeshSections);
	}
}

void AWorldGenProceduralActor::AddTileGeometry(
	int32 Q,
	int32 R,
	TArray<FWorldGenMeshSectionData>& MeshSections
)
{
	const FWorldGenTileData* Tile = GetTile(Q, R);
	if (!Tile)
	{
		return;
	}

	const FWorldGenTileRule TileRule = GetRule(Tile->TileType);
	const FVector Center = GetHexCenter(Q, R);

	const float FullRadius = HexRadius;
	const float InsetRadius = HexRadius * (1.0f - TransitionInsetRatio);

	TArray<FVector> PlateauCorners;
	TArray<FVector2D> PlateauUVs;

	PlateauCorners.SetNum(6);
	PlateauUVs.SetNum(6);

	// Stores the final outer/border vertices used by each edge.
	// EdgeBorderStart[EdgeIndex] is the corner at EdgeIndex.
	// EdgeBorderEnd[EdgeIndex] is the corner at EdgeIndex + 1.
	//
	// These are needed because two edges that touch the same corner can legally
	// resolve that corner to different Z heights. The corner patch pass fills
	// the hole between them.
	TArray<FVector> EdgeBorderStart;
	TArray<FVector> EdgeBorderEnd;

	EdgeBorderStart.SetNum(6);
	EdgeBorderEnd.SetNum(6);

	// First decide the plateau shape.
	// Same-neighbour and non-owned-transition edges stay full size.
	// Owned-transition edges pull their related corners inward.
	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 PreviousEdge = (CornerIndex + 5) % 6;
		const int32 NextEdge = CornerIndex;

		bool bInsetFromPreviousEdge = false;
		bool bInsetFromNextEdge = false;

		for (const int32 TestEdge : { PreviousEdge, NextEdge })
		{
			int32 NeighbourQ = 0;
			int32 NeighbourR = 0;

			if (!GetNeighbourCoord(Q, R, TestEdge, NeighbourQ, NeighbourR))
			{
				continue;
			}

			const FWorldGenTileData* NeighbourTile = GetTile(NeighbourQ, NeighbourR);
			if (!NeighbourTile)
			{
				continue;
			}

			const FWorldGenTransitionRule TransitionRule =
				FWorldGenRuleTypes::GetTransitionRule(
					Tile->TileType,
					NeighbourTile->TileType,
					FlatHeight,
					HillHeight,
					MountainHeight,
					CoastHeight,
					OceanHeight
				);

			if (TransitionRule.bHasTransition && TransitionRule.bThisTileOwnsTransition)
			{
				if (TestEdge == PreviousEdge)
				{
					bInsetFromPreviousEdge = true;
				}
				else
				{
					bInsetFromNextEdge = true;
				}
			}
		}

		const bool bCornerNeedsInset = bInsetFromPreviousEdge || bInsetFromNextEdge;
		const float CornerRadius = bCornerNeedsInset ? InsetRadius : FullRadius;
		const float UVScale = bCornerNeedsInset ? (1.0f - TransitionInsetRatio) : 1.0f;

		PlateauCorners[CornerIndex] =
			Center + GetHexCornerOffset(CornerIndex, CornerRadius, TileRule.Height);

		PlateauUVs[CornerIndex] = GetHexCornerUV(CornerIndex, UVScale);
	}

	const int32 TileSectionIndex = GetSectionIndexForTile(Tile->TileType);
	if (!MeshSections.IsValidIndex(TileSectionIndex))
	{
		return;
	}

	FWorldGenMeshSectionData& TileSection = MeshSections[TileSectionIndex];

	const FVector CenterTop = Center + FVector(0.0f, 0.0f, TileRule.Height);
	const FVector2D CenterUV(0.5f, 0.5f);

	// Plateau.
	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 NextCorner = (CornerIndex + 1) % 6;

		AddTriangle(
			TileSection,
			CenterTop,
			PlateauCorners[NextCorner],
			PlateauCorners[CornerIndex],
			CenterUV,
			PlateauUVs[NextCorner],
			PlateauUVs[CornerIndex]
		);
	}

	// Edges.
	for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
	{
		const int32 A = EdgeIndex;
		const int32 B = (EdgeIndex + 1) % 6;

		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;

		const bool bHasNeighbour = GetNeighbourCoord(Q, R, EdgeIndex, NeighbourQ, NeighbourR);
		const FWorldGenTileData* NeighbourTile = bHasNeighbour ? GetTile(NeighbourQ, NeighbourR) : nullptr;

		// Border edges or invalid neighbours stay at this tile's own height.
		if (!NeighbourTile)
		{
			const FVector BorderA = Center + GetHexCornerOffset(A, FullRadius, TileRule.Height);
			const FVector BorderB = Center + GetHexCornerOffset(B, FullRadius, TileRule.Height);

			EdgeBorderStart[EdgeIndex] = BorderA;
			EdgeBorderEnd[EdgeIndex] = BorderB;

			AddQuad(
				TileSection,
				PlateauCorners[A],
				PlateauCorners[B],
				BorderB,
				BorderA,
				PlateauUVs[A],
				PlateauUVs[B],
				GetHexCornerUV(B),
				GetHexCornerUV(A)
			);

			continue;
		}

		const FWorldGenTransitionRule TransitionRule =
			FWorldGenRuleTypes::GetTransitionRule(
				Tile->TileType,
				NeighbourTile->TileType,
				FlatHeight,
				HillHeight,
				MountainHeight,
				CoastHeight,
				OceanHeight
			);

		// Seamless edge: full border at this tile's height.
		if (!TransitionRule.bHasTransition)
		{
			const FVector BorderA = Center + GetHexCornerOffset(A, FullRadius, TileRule.Height);
			const FVector BorderB = Center + GetHexCornerOffset(B, FullRadius, TileRule.Height);

			EdgeBorderStart[EdgeIndex] = BorderA;
			EdgeBorderEnd[EdgeIndex] = BorderB;

			AddQuad(
				TileSection,
				PlateauCorners[A],
				PlateauCorners[B],
				BorderB,
				BorderA,
				PlateauUVs[A],
				PlateauUVs[B],
				GetHexCornerUV(B),
				GetHexCornerUV(A)
			);

			continue;
		}

		// This tile does not own the transition.
		// It remains full-size at its own height and lets the owner create the slope.
		if (!TransitionRule.bThisTileOwnsTransition)
		{
			const FVector BorderA = Center + GetHexCornerOffset(A, FullRadius, TileRule.Height);
			const FVector BorderB = Center + GetHexCornerOffset(B, FullRadius, TileRule.Height);

			EdgeBorderStart[EdgeIndex] = BorderA;
			EdgeBorderEnd[EdgeIndex] = BorderB;

			AddQuad(
				TileSection,
				PlateauCorners[A],
				PlateauCorners[B],
				BorderB,
				BorderA,
				PlateauUVs[A],
				PlateauUVs[B],
				GetHexCornerUV(B),
				GetHexCornerUV(A)
			);

			continue;
		}

		// This tile owns the transition.
		// Its inner edge is at this tile's height. Its border edge is at the neighbour height.
		const int32 TransitionSectionIndex =
			GetSectionIndexForTile(TransitionRule.TransitionMaterialTileType);

		if (!MeshSections.IsValidIndex(TransitionSectionIndex))
		{
			const FVector BorderA = Center + GetHexCornerOffset(A, FullRadius, TileRule.Height);
			const FVector BorderB = Center + GetHexCornerOffset(B, FullRadius, TileRule.Height);

			EdgeBorderStart[EdgeIndex] = BorderA;
			EdgeBorderEnd[EdgeIndex] = BorderB;

			continue;
		}

		FWorldGenMeshSectionData& TransitionSection = MeshSections[TransitionSectionIndex];

		const FVector BorderA =
			Center + GetHexCornerOffset(A, FullRadius, TransitionRule.OtherHeight);

		const FVector BorderB =
			Center + GetHexCornerOffset(B, FullRadius, TransitionRule.OtherHeight);

		EdgeBorderStart[EdgeIndex] = BorderA;
		EdgeBorderEnd[EdgeIndex] = BorderB;

		AddQuad(
			TransitionSection,
			PlateauCorners[A],
			PlateauCorners[B],
			BorderB,
			BorderA,
			PlateauUVs[A],
			PlateauUVs[B],
			GetHexCornerUV(B),
			GetHexCornerUV(A)
		);
	}

	// Corner seam patches.
	//
	// This fills cracks where the previous edge and next edge disagree about
	// the same hex corner's border height.
	//
	// Example:
	// One side of a corner may slope to water height.
	// The other side may slope to flat/hill height.
	// Both are valid edge decisions, but without this patch there is an open
	// triangular gap between those two border points and the plateau corner.
	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 PreviousEdge = (CornerIndex + 5) % 6;
		const int32 NextEdge = CornerIndex;

		const FVector PreviousEdgeCorner = EdgeBorderEnd[PreviousEdge];
		const FVector NextEdgeCorner = EdgeBorderStart[NextEdge];
		const FVector InnerCorner = PlateauCorners[CornerIndex];

		const bool bBorderCornersDiffer =
			!PreviousEdgeCorner.Equals(NextEdgeCorner, 0.1f);

		const bool bInnerIsDifferentFromPrevious =
			!InnerCorner.Equals(PreviousEdgeCorner, 0.1f);

		const bool bInnerIsDifferentFromNext =
			!InnerCorner.Equals(NextEdgeCorner, 0.1f);

		if (bBorderCornersDiffer && bInnerIsDifferentFromPrevious && bInnerIsDifferentFromNext)
		{
			AddTriangle(
				TileSection,
				InnerCorner,
				NextEdgeCorner,
				PreviousEdgeCorner,
				PlateauUVs[CornerIndex],
				GetHexCornerUV(CornerIndex),
				GetHexCornerUV(CornerIndex)
			);
		}
	}
}

void AWorldGenProceduralActor::AddTriangle(
	FWorldGenMeshSectionData& Section,
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector2D& UVA,
	const FVector2D& UVB,
	const FVector2D& UVC
)
{
	const FVector Cross = FVector::CrossProduct(B - A, C - A);

	if (Cross.SizeSquared() <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(
			LogWorldGenProcedural,
			Warning,
			TEXT("Skipped degenerate triangle. A:%s B:%s C:%s"),
			*A.ToString(),
			*B.ToString(),
			*C.ToString()
		);

		return;
	}

	const FVector RawNormal = Cross.GetSafeNormal();

	// UE procedural meshes usually need the opposite winding from the
	// mathematical "normal points up" order for top-facing visibility.
	//
	// If the cross product points upward, swap B/C so the top side renders.
	if (RawNormal.Z > 0.0f)
	{
		const FVector FixedNormal = CalculateNormal(A, C, B);

		AddVertex(Section, A, UVA, FixedNormal);
		AddVertex(Section, C, UVC, FixedNormal);
		AddVertex(Section, B, UVB, FixedNormal);
	}
	else
	{
		const FVector FixedNormal = CalculateNormal(A, B, C);

		AddVertex(Section, A, UVA, FixedNormal);
		AddVertex(Section, B, UVB, FixedNormal);
		AddVertex(Section, C, UVC, FixedNormal);
	}
}

void AWorldGenProceduralActor::AddQuad(
	FWorldGenMeshSectionData& Section,
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector& D,
	const FVector2D& UVA,
	const FVector2D& UVB,
	const FVector2D& UVC,
	const FVector2D& UVD
)
{
	AddTriangle(Section, A, B, C, UVA, UVB, UVC);
	AddTriangle(Section, A, C, D, UVA, UVC, UVD);
}

void AWorldGenProceduralActor::AddVertex(
	FWorldGenMeshSectionData& Section,
	const FVector& Position,
	const FVector2D& UV,
	const FVector& Normal
)
{
	const int32 NewIndex = Section.Vertices.Num();

	Section.Vertices.Add(Position);
	Section.Triangles.Add(NewIndex);
	Section.Normals.Add(Normal);
	Section.UVs.Add(UV);
	Section.VertexColors.Add(FColor::White);
	Section.Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
}

FVector AWorldGenProceduralActor::CalculateNormal(
	const FVector& A,
	const FVector& B,
	const FVector& C
) const
{
	if (bUseStylizedUpNormals)
	{
		return FVector::UpVector;
	}

	FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();

	// Even if triangle winding is reversed for UE visibility,
	// keep lighting normals pointing generally upward.
	if (Normal.Z < 0.0f)
	{
		Normal *= -1.0f;
	}

	return (Normal + FVector::UpVector * 0.75f).GetSafeNormal();
}

FVector AWorldGenProceduralActor::GetHexCenter(int32 Q, int32 R) const
{
	const float HexWidth = FMath::Sqrt(3.0f) * HexRadius;
	const float VerticalSpacing = 1.5f * HexRadius;

	const float X = HexWidth * (static_cast<float>(Q) + 0.5f * static_cast<float>(R & 1));
	const float Y = VerticalSpacing * static_cast<float>(R);

	return FVector(X, Y, 0.0f);
}

FVector AWorldGenProceduralActor::GetHexCornerOffset(
	int32 CornerIndex,
	float Radius,
	float Z
) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	return FVector(
		Radius * FMath::Cos(AngleRad),
		Radius * FMath::Sin(AngleRad),
		Z
	);
}

FVector2D AWorldGenProceduralActor::GetHexCornerUV(
	int32 CornerIndex,
	float Scale
) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	return FVector2D(
		0.5f + 0.5f * Scale * FMath::Cos(AngleRad),
		0.5f + 0.5f * Scale * FMath::Sin(AngleRad)
	);
}

bool AWorldGenProceduralActor::GetNeighbourCoord(
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	int32& OutQ,
	int32& OutR
) const
{
	const bool bOddRow = (R & 1) != 0;

	switch (EdgeIndex)
	{
	case 0: // NE
		OutQ = bOddRow ? Q + 1 : Q;
		OutR = R - 1;
		break;

	case 1: // NW
		OutQ = bOddRow ? Q : Q - 1;
		OutR = R - 1;
		break;

	case 2: // W
		OutQ = Q - 1;
		OutR = R;
		break;

	case 3: // SW
		OutQ = bOddRow ? Q : Q - 1;
		OutR = R + 1;
		break;

	case 4: // SE
		OutQ = bOddRow ? Q + 1 : Q;
		OutR = R + 1;
		break;

	case 5: // E
		OutQ = Q + 1;
		OutR = R;
		break;

	default:
		return false;
	}

	return IsValidTile(OutQ, OutR);
}

bool AWorldGenProceduralActor::IsValidTile(int32 Q, int32 R) const
{
	return Q >= 0 && Q < GridWidth && R >= 0 && R < GridHeight;
}

int32 AWorldGenProceduralActor::GetTileIndex(int32 Q, int32 R) const
{
	return R * GridWidth + Q;
}

const FWorldGenTileData* AWorldGenProceduralActor::GetTile(int32 Q, int32 R) const
{
	if (!IsValidTile(Q, R))
	{
		return nullptr;
	}

	const int32 Index = GetTileIndex(Q, R);

	if (!Tiles.IsValidIndex(Index))
	{
		return nullptr;
	}

	return &Tiles[Index];
}

FWorldGenTileRule AWorldGenProceduralActor::GetRule(EWorldGenTileType TileType) const
{
	return FWorldGenRuleTypes::GetTileRule(
		TileType,
		FlatHeight,
		HillHeight,
		MountainHeight,
		CoastHeight,
		OceanHeight
	);
}

int32 AWorldGenProceduralActor::GetSectionIndexForTile(EWorldGenTileType TileType) const
{
	return GetRule(TileType).MaterialSlot;
}

void AWorldGenProceduralActor::LogGenerationDebug(
	const TArray<FWorldGenMeshSectionData>& MeshSections
) const
{
	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("========== WORLD GEN DEBUG START ==========")
	);

	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("Grid: %dx%d | HexRadius: %.2f | Seed: %d"),
		GridWidth,
		GridHeight,
		HexRadius,
		RandomSeed
	);

	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("Heights | Flat: %.2f | Hill: %.2f | Mountain: %.2f | Coast: %.2f | Ocean: %.2f"),
		FlatHeight,
		HillHeight,
		MountainHeight,
		CoastHeight,
		OceanHeight
	);

	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("Generation Chances | Water: %.2f | Hill: %.2f | Mountain: %.2f"),
		WaterChance,
		HillChance,
		MountainChance
	);

	int32 ProblemEdgeCount = 0;
	int32 TransitionEdgeCount = 0;
	int32 SeamlessEdgeCount = 0;
	int32 MissingNeighbourEdgeCount = 0;

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			const FWorldGenTileData* Tile = GetTile(Q, R);
			if (!Tile)
			{
				continue;
			}

			if (bLogEveryTile)
			{
				LogTileDebug(Q, R);
			}

			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				int32 NeighbourQ = 0;
				int32 NeighbourR = 0;

				if (!GetNeighbourCoord(Q, R, EdgeIndex, NeighbourQ, NeighbourR))
				{
					++MissingNeighbourEdgeCount;
					continue;
				}

				const FWorldGenTileData* NeighbourTile = GetTile(NeighbourQ, NeighbourR);
				if (!NeighbourTile)
				{
					++MissingNeighbourEdgeCount;
					continue;
				}

				const FWorldGenTransitionRule TransitionRule =
					FWorldGenRuleTypes::GetTransitionRule(
						Tile->TileType,
						NeighbourTile->TileType,
						FlatHeight,
						HillHeight,
						MountainHeight,
						CoastHeight,
						OceanHeight
					);

				if (TransitionRule.bHasTransition)
				{
					++TransitionEdgeCount;
				}
				else
				{
					++SeamlessEdgeCount;
				}

				bool bProblemFound = false;

				if (bLogEveryEdge)
				{
					LogEdgeDebug(Q, R, EdgeIndex, bProblemFound);
				}
				else
				{
					LogEdgeDebug(Q, R, EdgeIndex, bProblemFound);
				}

				if (bProblemFound)
				{
					++ProblemEdgeCount;
				}
			}
		}
	}

	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("Edge Summary | Seamless: %d | Transitions: %d | Missing/Border Edges: %d | Problems: %d"),
		SeamlessEdgeCount,
		TransitionEdgeCount,
		MissingNeighbourEdgeCount,
		ProblemEdgeCount
	);

	if (bValidateGeneratedMesh)
	{
		ValidateMeshSections(MeshSections);
	}

	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("========== WORLD GEN DEBUG END ==========")
	);
}

void AWorldGenProceduralActor::LogTileDebug(
	int32 Q,
	int32 R
) const
{
	const FWorldGenTileData* Tile = GetTile(Q, R);
	if (!Tile)
	{
		UE_LOG(
			LogWorldGenProcedural,
			Warning,
			TEXT("[Tile Q:%d R:%d] INVALID TILE"),
			Q,
			R
		);
		return;
	}

	const FWorldGenTileRule Rule = GetRule(Tile->TileType);

	UE_LOG(
		LogWorldGenProcedural,
		Log,
		TEXT("[Tile Q:%d R:%d] Type:%s Height:%.2f Base:%d Elevation:%d Water:%s Blocks:%s MaterialSlot:%d Priority:%d"),
		Q,
		R,
		*TileTypeToString(Tile->TileType),
		Rule.Height,
		static_cast<int32>(Rule.BaseTerrain),
		static_cast<int32>(Rule.ElevationType),
		Rule.bIsWater ? TEXT("true") : TEXT("false"),
		Rule.bBlocksMovement ? TEXT("true") : TEXT("false"),
		Rule.MaterialSlot,
		Rule.TransitionPriority
	);
}

void AWorldGenProceduralActor::LogEdgeDebug(
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	bool& bOutProblemFound
) const
{
	bOutProblemFound = false;

	const FWorldGenTileData* Tile = GetTile(Q, R);
	if (!Tile)
	{
		bOutProblemFound = true;
		return;
	}

	const FWorldGenTileRule ThisRule = GetRule(Tile->TileType);

	int32 NeighbourQ = 0;
	int32 NeighbourR = 0;

	if (!GetNeighbourCoord(Q, R, EdgeIndex, NeighbourQ, NeighbourR))
	{
		if (bLogEveryEdge && !bLogOnlyProblemEdges)
		{
			UE_LOG(
				LogWorldGenProcedural,
				Log,
				TEXT("[Edge Q:%d R:%d %s] Border edge. No neighbour."),
				Q,
				R,
				*EdgeIndexToString(EdgeIndex)
			);
		}

		return;
	}

	const FWorldGenTileData* NeighbourTile = GetTile(NeighbourQ, NeighbourR);
	if (!NeighbourTile)
	{
		bOutProblemFound = true;

		UE_LOG(
			LogWorldGenProcedural,
			Warning,
			TEXT("[Edge Q:%d R:%d %s] Neighbour coord Q:%d R:%d exists but tile lookup failed."),
			Q,
			R,
			*EdgeIndexToString(EdgeIndex),
			NeighbourQ,
			NeighbourR
		);

		return;
	}

	const FWorldGenTileRule OtherRule = GetRule(NeighbourTile->TileType);

	const FWorldGenTransitionRule TransitionRule =
		FWorldGenRuleTypes::GetTransitionRule(
			Tile->TileType,
			NeighbourTile->TileType,
			FlatHeight,
			HillHeight,
			MountainHeight,
			CoastHeight,
			OceanHeight
		);

	const bool bSameType = Tile->TileType == NeighbourTile->TileType;
	const bool bSameHeight = FMath::IsNearlyEqual(
		ThisRule.Height,
		OtherRule.Height,
		DebugHeightTolerance
	);

	const bool bSeamless = !TransitionRule.bHasTransition;

	TArray<FString> Problems;

	if (bSameType && TransitionRule.bHasTransition)
	{
		Problems.Add(TEXT("Same tile type produced transition"));
	}

	if (TransitionRule.bHasTransition && bSameHeight)
	{
		Problems.Add(TEXT("Transition exists but heights are equal/nearly equal"));
	}

	if (!TransitionRule.bHasTransition && !bSameHeight)
	{
		Problems.Add(TEXT("No transition but heights differ"));
	}

	if (TransitionRule.bHasTransition &&
		TransitionRule.TransitionMaterialTileType != Tile->TileType &&
		TransitionRule.TransitionMaterialTileType != NeighbourTile->TileType)
	{
		Problems.Add(TEXT("Transition material owner is not one of the two tiles"));
	}

	if (TransitionRule.bThisTileOwnsTransition &&
		TransitionRule.TransitionMaterialTileType != Tile->TileType)
	{
		Problems.Add(TEXT("This tile owns transition but material owner differs"));
	}

	bOutProblemFound = Problems.Num() > 0;

	if (bLogOnlyProblemEdges && !bOutProblemFound)
	{
		return;
	}

	if (!bLogEveryEdge && !bOutProblemFound)
	{
		return;
	}

	const FString ProblemString = Problems.Num() > 0
		? FString::Join(Problems, TEXT(" | "))
		: TEXT("None");

	if (bOutProblemFound)
	{
		UE_LOG(
			LogWorldGenProcedural,
			Warning,
			TEXT("[Edge Q:%d R:%d %s] This:%s H:%.2f -> Other Q:%d R:%d Type:%s H:%.2f | SameType:%s SameHeight:%s Seamless:%s Transition:%s OwnerThis:%s MaterialOwner:%s Style:%s | Interp %.2f -> %.2f | Problems:%s"),
			Q,
			R,
			*EdgeIndexToString(EdgeIndex),
			*TileTypeToString(Tile->TileType),
			ThisRule.Height,
			NeighbourQ,
			NeighbourR,
			*TileTypeToString(NeighbourTile->TileType),
			OtherRule.Height,
			bSameType ? TEXT("true") : TEXT("false"),
			bSameHeight ? TEXT("true") : TEXT("false"),
			bSeamless ? TEXT("true") : TEXT("false"),
			TransitionRule.bHasTransition ? TEXT("true") : TEXT("false"),
			TransitionRule.bThisTileOwnsTransition ? TEXT("true") : TEXT("false"),
			*TileTypeToString(TransitionRule.TransitionMaterialTileType),
			*TransitionStyleToString(TransitionRule.TransitionStyle),
			TransitionRule.ThisHeight,
			TransitionRule.OtherHeight,
			*ProblemString
		);
	}
	else
	{
		UE_LOG(
			LogWorldGenProcedural,
			Log,
			TEXT("[Edge Q:%d R:%d %s] This:%s H:%.2f -> Other Q:%d R:%d Type:%s H:%.2f | SameType:%s SameHeight:%s Seamless:%s Transition:%s OwnerThis:%s MaterialOwner:%s Style:%s | Interp %.2f -> %.2f | Problems:%s"),
			Q,
			R,
			*EdgeIndexToString(EdgeIndex),
			*TileTypeToString(Tile->TileType),
			ThisRule.Height,
			NeighbourQ,
			NeighbourR,
			*TileTypeToString(NeighbourTile->TileType),
			OtherRule.Height,
			bSameType ? TEXT("true") : TEXT("false"),
			bSameHeight ? TEXT("true") : TEXT("false"),
			bSeamless ? TEXT("true") : TEXT("false"),
			TransitionRule.bHasTransition ? TEXT("true") : TEXT("false"),
			TransitionRule.bThisTileOwnsTransition ? TEXT("true") : TEXT("false"),
			*TileTypeToString(TransitionRule.TransitionMaterialTileType),
			*TransitionStyleToString(TransitionRule.TransitionStyle),
			TransitionRule.ThisHeight,
			TransitionRule.OtherHeight,
			*ProblemString
		);
	}
}

void AWorldGenProceduralActor::ValidateMeshSections(
	const TArray<FWorldGenMeshSectionData>& MeshSections
) const
{
	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("----- Mesh Validation Start -----")
	);

	int32 TotalVertices = 0;
	int32 TotalTriangles = 0;
	int32 TotalDownwardFacingTriangles = 0;
	int32 TotalDegenerateTriangles = 0;

	for (int32 SectionIndex = 0; SectionIndex < MeshSections.Num(); ++SectionIndex)
	{
		const FWorldGenMeshSectionData& Section = MeshSections[SectionIndex];

		const int32 VertexCount = Section.Vertices.Num();
		const int32 TriangleIndexCount = Section.Triangles.Num();
		const int32 TriangleCount = TriangleIndexCount / 3;

		TotalVertices += VertexCount;
		TotalTriangles += TriangleCount;

		int32 DownwardFacingTriangles = 0;
		int32 DegenerateTriangles = 0;
		int32 InvalidTriangleIndices = 0;

		for (int32 TriangleIndex = 0; TriangleIndex + 2 < TriangleIndexCount; TriangleIndex += 3)
		{
			const int32 IndexA = Section.Triangles[TriangleIndex];
			const int32 IndexB = Section.Triangles[TriangleIndex + 1];
			const int32 IndexC = Section.Triangles[TriangleIndex + 2];

			if (!Section.Vertices.IsValidIndex(IndexA) ||
				!Section.Vertices.IsValidIndex(IndexB) ||
				!Section.Vertices.IsValidIndex(IndexC))
			{
				++InvalidTriangleIndices;
				continue;
			}

			const FVector& A = Section.Vertices[IndexA];
			const FVector& B = Section.Vertices[IndexB];
			const FVector& C = Section.Vertices[IndexC];

			const FVector Cross = FVector::CrossProduct(B - A, C - A);
			const float AreaSize = Cross.Size();

			if (AreaSize <= KINDA_SMALL_NUMBER)
			{
				++DegenerateTriangles;
				continue;
			}

			const FVector FaceNormal = Cross / AreaSize;

			if (FaceNormal.Z < -0.01f)
			{
				++DownwardFacingTriangles;
			}
		}

		TotalDownwardFacingTriangles += DownwardFacingTriangles;
		TotalDegenerateTriangles += DegenerateTriangles;

		const bool bHasMaterial = TileMaterials.IsValidIndex(SectionIndex) && TileMaterials[SectionIndex] != nullptr;

		if (VertexCount > 0 || TriangleCount > 0 || !bHasMaterial)
		{
			UE_LOG(
				LogWorldGenProcedural,
				Warning,
				TEXT("[Section %d] Verts:%d Tris:%d Material:%s DownwardFaces:%d Degenerate:%d InvalidIndices:%d"),
				SectionIndex,
				VertexCount,
				TriangleCount,
				bHasMaterial ? TEXT("Assigned") : TEXT("MISSING"),
				DownwardFacingTriangles,
				DegenerateTriangles,
				InvalidTriangleIndices
			);
		}

		if (TriangleIndexCount % 3 != 0)
		{
			UE_LOG(
				LogWorldGenProcedural,
				Error,
				TEXT("[Section %d] Triangle index count is not divisible by 3: %d"),
				SectionIndex,
				TriangleIndexCount
			);
		}

		if (Section.Normals.Num() != Section.Vertices.Num())
		{
			UE_LOG(
				LogWorldGenProcedural,
				Error,
				TEXT("[Section %d] Normal count mismatch. Normals:%d Vertices:%d"),
				SectionIndex,
				Section.Normals.Num(),
				Section.Vertices.Num()
			);
		}

		if (Section.UVs.Num() != Section.Vertices.Num())
		{
			UE_LOG(
				LogWorldGenProcedural,
				Error,
				TEXT("[Section %d] UV count mismatch. UVs:%d Vertices:%d"),
				SectionIndex,
				Section.UVs.Num(),
				Section.Vertices.Num()
			);
		}
	}

	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("Mesh Totals | Verts:%d Tris:%d DownwardFaces:%d Degenerate:%d"),
		TotalVertices,
		TotalTriangles,
		TotalDownwardFacingTriangles,
		TotalDegenerateTriangles
	);

	UE_LOG(
		LogWorldGenProcedural,
		Warning,
		TEXT("----- Mesh Validation End -----")
	);
}

FString AWorldGenProceduralActor::TileTypeToString(EWorldGenTileType TileType) const
{
	const UEnum* Enum = StaticEnum<EWorldGenTileType>();
	if (!Enum)
	{
		return TEXT("Unknown");
	}

	return Enum->GetNameStringByValue(static_cast<int64>(TileType));
}

FString AWorldGenProceduralActor::TransitionStyleToString(
	EWorldGenTransitionStyle TransitionStyle
) const
{
	const UEnum* Enum = StaticEnum<EWorldGenTransitionStyle>();
	if (!Enum)
	{
		return TEXT("Unknown");
	}

	return Enum->GetNameStringByValue(static_cast<int64>(TransitionStyle));
}

FString AWorldGenProceduralActor::EdgeIndexToString(int32 EdgeIndex) const
{
	switch (EdgeIndex)
	{
	case 0:
		return TEXT("NE");
	case 1:
		return TEXT("NW");
	case 2:
		return TEXT("W");
	case 3:
		return TEXT("SW");
	case 4:
		return TEXT("SE");
	case 5:
		return TEXT("E");
	default:
		return FString::Printf(TEXT("InvalidEdge_%d"), EdgeIndex);
	}
}