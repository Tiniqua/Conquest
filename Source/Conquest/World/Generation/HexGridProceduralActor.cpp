#include "HexGridProceduralActor.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"

AHexGridProceduralActor::AHexGridProceduralActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GridMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GridMesh"));
	GridMesh->SetupAttachment(SceneRoot);
	GridMesh->bUseAsyncCooking = true;
	GridMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AHexGridProceduralActor::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		RebuildGrid();
	}
}

void AHexGridProceduralActor::RebuildGrid()
{
	GenerateTileData();
	BuildSharedCornerHeights();
	GenerateMesh();
}

void AHexGridProceduralActor::GenerateTileData()
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
			const int32 TileIndex = GetTileIndex(Q, R);

			const float Roll = RandomStream.FRand();

			EHexTileType NewType = EHexTileType::FlatGround;

			if (Roll < OceanChance)
			{
				NewType = EHexTileType::Ocean;
			}
			else if (Roll < OceanChance + HillChance)
			{
				NewType = EHexTileType::Hill;
			}

			Tiles[TileIndex].TileType = NewType;
			Tiles[TileIndex].Height = GetTileHeight(NewType);
		}
	}
}

void AHexGridProceduralActor::BuildSharedCornerHeights()
{
	SharedCornerHeights.Reset();

	TMap<FIntPoint, TArray<float>> CornerSamples;

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			if (!GetTile(Q, R))
			{
				continue;
			}

			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const FVector2D CornerXY = GetHexCornerWorldXY(Q, R, CornerIndex);
				const FIntPoint CornerKey = MakeCornerKey(CornerXY);

				TArray<float>& Samples = CornerSamples.FindOrAdd(CornerKey);

				// A hex corner belongs to two edges of this tile.
				// Edge CornerIndex starts at this corner.
				// Edge CornerIndex - 1 ends at this corner.
				const int32 PreviousEdgeIndex = (CornerIndex + 5) % 6;
				const int32 NextEdgeIndex = CornerIndex;

				Samples.Add(GetDesiredEdgeHeightForTile(Q, R, PreviousEdgeIndex));
				Samples.Add(GetDesiredEdgeHeightForTile(Q, R, NextEdgeIndex));
			}
		}
	}

	for (const TPair<FIntPoint, TArray<float>>& Pair : CornerSamples)
	{
		SharedCornerHeights.Add(Pair.Key, ResolveCornerHeight(Pair.Value));
	}
}

float AHexGridProceduralActor::ResolveCornerHeight(const TArray<float>& Samples) const
{
	if (Samples.Num() == 0)
	{
		return FlatHeight;
	}

	TArray<float> SortedSamples = Samples;
	SortedSamples.Sort();

	// Median gives stable junctions without letting one odd neighbour dominate every corner.
	// This is intentionally simple and avoids complex per-corner special cases.
	return SortedSamples[SortedSamples.Num() / 2];
}

void AHexGridProceduralActor::GenerateMesh()
{
	if (!GridMesh)
	{
		return;
	}

	GridMesh->ClearAllMeshSections();

	FHexMeshSectionData FlatSection;
	FHexMeshSectionData HillSection;
	FHexMeshSectionData OceanSection;

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			AddTileGeometry(Q, R, FlatSection, HillSection, OceanSection);
		}
	}

	GridMesh->CreateMeshSection(
		0,
		FlatSection.Vertices,
		FlatSection.Triangles,
		FlatSection.Normals,
		FlatSection.UVs,
		FlatSection.VertexColors,
		FlatSection.Tangents,
		true
	);

	GridMesh->CreateMeshSection(
		1,
		HillSection.Vertices,
		HillSection.Triangles,
		HillSection.Normals,
		HillSection.UVs,
		HillSection.VertexColors,
		HillSection.Tangents,
		true
	);

	GridMesh->CreateMeshSection(
		2,
		OceanSection.Vertices,
		OceanSection.Triangles,
		OceanSection.Normals,
		OceanSection.UVs,
		OceanSection.VertexColors,
		OceanSection.Tangents,
		true
	);

	GridMesh->SetMaterial(0, FlatGroundMaterial);
	GridMesh->SetMaterial(1, HillMaterial);
	GridMesh->SetMaterial(2, OceanMaterial);
}

void AHexGridProceduralActor::AddTileGeometry(
	int32 Q,
	int32 R,
	FHexMeshSectionData& FlatSection,
	FHexMeshSectionData& HillSection,
	FHexMeshSectionData& OceanSection
)
{
	const FHexTileData* Tile = GetTile(Q, R);
	if (!Tile)
	{
		return;
	}

	FHexMeshSectionData& TileSection = GetSectionForTileType(
		Tile->TileType,
		FlatSection,
		HillSection,
		OceanSection
	);

	const FVector Center = GetHexCenter(Q, R);
	const float TileHeight = Tile->Height;

	const FVector CenterTop = Center + FVector(0.0f, 0.0f, TileHeight);
	const FVector2D CenterUV(0.5f, 0.5f);

	TArray<FVector> InnerCorners;
	TArray<FVector> OuterCorners;
	TArray<FVector2D> InnerUVs;
	TArray<FVector2D> OuterUVs;

	InnerCorners.SetNum(6);
	OuterCorners.SetNum(6);
	InnerUVs.SetNum(6);
	OuterUVs.SetNum(6);

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		InnerCorners[CornerIndex] =
			Center + GetHexCornerOffset(CornerIndex, HexRadius * InnerPlateauScale, TileHeight);

		const float OuterCornerHeight = GetSharedCornerHeight(Q, R, CornerIndex);
		OuterCorners[CornerIndex] =
			Center + GetHexCornerOffset(CornerIndex, HexRadius, OuterCornerHeight);

		InnerUVs[CornerIndex] = GetHexCornerUV(CornerIndex, InnerPlateauScale);
		OuterUVs[CornerIndex] = GetHexCornerUV(CornerIndex, 1.0f);
	}

	// Center plateau.
	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 NextCornerIndex = (CornerIndex + 1) % 6;

		AddPlateauTriangle(
			TileSection,
			CenterTop,
			InnerCorners[NextCornerIndex],
			InnerCorners[CornerIndex],
			CenterUV,
			InnerUVs[NextCornerIndex],
			InnerUVs[CornerIndex]
		);
	}

	// Outer edge/taper band.
	for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
	{
		const int32 NextCornerIndex = (EdgeIndex + 1) % 6;

		AddEdgeBandQuad(
			TileSection,
			InnerCorners[EdgeIndex],
			InnerCorners[NextCornerIndex],
			OuterCorners[EdgeIndex],
			OuterCorners[NextCornerIndex],
			InnerUVs[EdgeIndex],
			InnerUVs[NextCornerIndex],
			OuterUVs[EdgeIndex],
			OuterUVs[NextCornerIndex]
		);
	}
}

void AHexGridProceduralActor::AddPlateauTriangle(
	FHexMeshSectionData& Section,
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector2D& UVA,
	const FVector2D& UVB,
	const FVector2D& UVC
)
{
	AddTriangleVisibleFromAbove(Section, A, B, C, UVA, UVB, UVC);
}

void AHexGridProceduralActor::AddEdgeBandQuad(
	FHexMeshSectionData& Section,
	const FVector& InnerA,
	const FVector& InnerB,
	const FVector& OuterA,
	const FVector& OuterB,
	const FVector2D& UVInnerA,
	const FVector2D& UVInnerB,
	const FVector2D& UVOuterA,
	const FVector2D& UVOuterB
)
{
	AddTriangleVisibleFromAbove(
		Section,
		InnerA,
		InnerB,
		OuterB,
		UVInnerA,
		UVInnerB,
		UVOuterB
	);

	AddTriangleVisibleFromAbove(
		Section,
		InnerA,
		OuterB,
		OuterA,
		UVInnerA,
		UVOuterB,
		UVOuterA
	);
}

void AHexGridProceduralActor::AddTriangleVisibleFromAbove(
	FHexMeshSectionData& Section,
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector2D& UVA,
	const FVector2D& UVB,
	const FVector2D& UVC
)
{
	const FVector RawNormal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();

	// Unreal's visible face winding for procedural meshes is usually the reverse
	// of the simple "normal points up" order people expect when building XY geometry.
	// If the mathematical normal points up, reverse B/C for reliable top visibility.
	if (RawNormal.Z > 0.0f)
	{
		const FVector Normal = CalculateTriangleNormal(A, C, B);

		AddVertex(Section, A, UVA, Normal);
		AddVertex(Section, C, UVC, Normal);
		AddVertex(Section, B, UVB, Normal);
	}
	else
	{
		const FVector Normal = CalculateTriangleNormal(A, B, C);

		AddVertex(Section, A, UVA, Normal);
		AddVertex(Section, B, UVB, Normal);
		AddVertex(Section, C, UVC, Normal);
	}
}

void AHexGridProceduralActor::AddVertex(
	FHexMeshSectionData& Section,
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

FVector AHexGridProceduralActor::CalculateTriangleNormal(
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

	if (Normal.Z < 0.0f)
	{
		Normal *= -1.0f;
	}

	// Prevent very dark transition slopes.
	return (Normal + FVector::UpVector * 0.75f).GetSafeNormal();
}

FHexMeshSectionData& AHexGridProceduralActor::GetSectionForTileType(
	EHexTileType TileType,
	FHexMeshSectionData& FlatSection,
	FHexMeshSectionData& HillSection,
	FHexMeshSectionData& OceanSection
) const
{
	switch (TileType)
	{
	case EHexTileType::Hill:
		return HillSection;

	case EHexTileType::Ocean:
		return OceanSection;

	case EHexTileType::FlatGround:
	default:
		return FlatSection;
	}
}

FVector AHexGridProceduralActor::GetHexCenter(int32 Q, int32 R) const
{
	// Pointy-top, odd-r offset layout.
	const float HexWidth = FMath::Sqrt(3.0f) * HexRadius;
	const float VerticalSpacing = 1.5f * HexRadius;

	const float X = HexWidth * (static_cast<float>(Q) + 0.5f * static_cast<float>(R & 1));
	const float Y = VerticalSpacing * static_cast<float>(R);

	return FVector(X, Y, 0.0f);
}

FVector AHexGridProceduralActor::GetHexCornerOffset(
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

FVector2D AHexGridProceduralActor::GetHexCornerWorldXY(
	int32 Q,
	int32 R,
	int32 CornerIndex
) const
{
	const FVector Center = GetHexCenter(Q, R);
	const FVector CornerOffset = GetHexCornerOffset(CornerIndex, HexRadius);

	return FVector2D(
		Center.X + CornerOffset.X,
		Center.Y + CornerOffset.Y
	);
}

FVector2D AHexGridProceduralActor::GetHexCornerUV(
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

FIntPoint AHexGridProceduralActor::MakeCornerKey(const FVector2D& WorldXY) const
{
	// Round to a stable integer key.
	// Multiplying keeps sub-unit precision without relying on raw float equality.
	return FIntPoint(
		FMath::RoundToInt(WorldXY.X * 100.0f),
		FMath::RoundToInt(WorldXY.Y * 100.0f)
	);
}

float AHexGridProceduralActor::GetTileHeight(EHexTileType TileType) const
{
	switch (TileType)
	{
	case EHexTileType::Hill:
		return HillHeight;

	case EHexTileType::Ocean:
		return OceanHeight;

	case EHexTileType::FlatGround:
	default:
		return FlatHeight;
	}
}

float AHexGridProceduralActor::GetSharedCornerHeight(
	int32 Q,
	int32 R,
	int32 CornerIndex
) const
{
	const FVector2D CornerXY = GetHexCornerWorldXY(Q, R, CornerIndex);
	const FIntPoint CornerKey = MakeCornerKey(CornerXY);

	if (const float* FoundHeight = SharedCornerHeights.Find(CornerKey))
	{
		return *FoundHeight;
	}

	const FHexTileData* Tile = GetTile(Q, R);
	return Tile ? Tile->Height : FlatHeight;
}

float AHexGridProceduralActor::GetDesiredEdgeHeightForTile(
	int32 Q,
	int32 R,
	int32 EdgeIndex
) const
{
	const FHexTileData* Tile = GetTile(Q, R);
	if (!Tile)
	{
		return FlatHeight;
	}

	int32 NeighbourQ = 0;
	int32 NeighbourR = 0;

	if (!GetNeighbourCoord(Q, R, EdgeIndex, NeighbourQ, NeighbourR))
	{
		return Tile->Height;
	}

	const FHexTileData* NeighbourTile = GetTile(NeighbourQ, NeighbourR);
	if (!NeighbourTile)
	{
		return Tile->Height;
	}

	if (Tile->TileType == NeighbourTile->TileType ||
		FMath::IsNearlyEqual(Tile->Height, NeighbourTile->Height, 0.1f))
	{
		return Tile->Height;
	}

	// Different terrain types:
	// The transition owner slopes to the other tile height.
	// The non-owner stays full-size at its own height.
	if (IsTransitionOwner(Tile->TileType, NeighbourTile->TileType))
	{
		return NeighbourTile->Height;
	}

	return Tile->Height;
}

bool AHexGridProceduralActor::IsTransitionOwner(
	EHexTileType ThisType,
	EHexTileType OtherType
) const
{
	if (ThisType == OtherType)
	{
		return false;
	}

	// Explicit art rules:
	// Ocean -> Flat uses ocean material.
	if (ThisType == EHexTileType::Ocean && OtherType == EHexTileType::FlatGround)
	{
		return true;
	}

	if (ThisType == EHexTileType::FlatGround && OtherType == EHexTileType::Ocean)
	{
		return false;
	}

	// Flat -> Hill uses hill material.
	if (ThisType == EHexTileType::Hill && OtherType == EHexTileType::FlatGround)
	{
		return true;
	}

	if (ThisType == EHexTileType::FlatGround && OtherType == EHexTileType::Hill)
	{
		return false;
	}

	// Ocean -> Hill fallback:
	// Hill owns the transition so cliffs/raised land get the hill material.
	if (ThisType == EHexTileType::Hill && OtherType == EHexTileType::Ocean)
	{
		return true;
	}

	if (ThisType == EHexTileType::Ocean && OtherType == EHexTileType::Hill)
	{
		return false;
	}

	return false;
}

bool AHexGridProceduralActor::GetNeighbourCoord(
	int32 Q,
	int32 R,
	int32 EdgeIndex,
	int32& OutQ,
	int32& OutR
) const
{
	// Corner order:
	// 0 = upper-right
	// 1 = top
	// 2 = upper-left
	// 3 = lower-left
	// 4 = bottom
	// 5 = lower-right
	//
	// Edge order:
	// 0 = NE, between corners 0 and 1
	// 1 = NW, between corners 1 and 2
	// 2 = W,  between corners 2 and 3
	// 3 = SW, between corners 3 and 4
	// 4 = SE, between corners 4 and 5
	// 5 = E,  between corners 5 and 0

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

bool AHexGridProceduralActor::IsValidTile(int32 Q, int32 R) const
{
	return Q >= 0 && Q < GridWidth && R >= 0 && R < GridHeight;
}

int32 AHexGridProceduralActor::GetTileIndex(int32 Q, int32 R) const
{
	return R * GridWidth + Q;
}

const FHexTileData* AHexGridProceduralActor::GetTile(int32 Q, int32 R) const
{
	if (!IsValidTile(Q, R))
	{
		return nullptr;
	}

	const int32 TileIndex = GetTileIndex(Q, R);

	if (!Tiles.IsValidIndex(TileIndex))
	{
		return nullptr;
	}

	return &Tiles[TileIndex];
}