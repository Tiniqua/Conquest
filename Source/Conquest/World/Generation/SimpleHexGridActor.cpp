#include "SimpleHexGridActor.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"

ASimpleHexGridActor::ASimpleHexGridActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GridMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GridMesh"));
	GridMesh->SetupAttachment(SceneRoot);
	GridMesh->bUseAsyncCooking = true;
	GridMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ASimpleHexGridActor::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		RebuildGrid();
	}
}

void ASimpleHexGridActor::RebuildGrid()
{
	GenerateTileData();
	GenerateMesh();
}

void ASimpleHexGridActor::GenerateTileData()
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
			ESimpleHexTileType NewTileType = ESimpleHexTileType::Grassland;

			const float WaterRoll = RandomStream.FRand();
			const float MountainRoll = RandomStream.FRand();
			const float TerrainRoll = RandomStream.FRand();

			if (WaterRoll < WaterChance)
			{
				NewTileType = TerrainRoll < 0.5f
					? ESimpleHexTileType::Ocean
					: ESimpleHexTileType::Coast;
			}
			else if (MountainRoll < MountainChance)
			{
				NewTileType = ESimpleHexTileType::Mountain;
			}
			else
			{
				if (TerrainRoll < 0.30f)
				{
					NewTileType = ESimpleHexTileType::Grassland;
				}
				else if (TerrainRoll < 0.52f)
				{
					NewTileType = ESimpleHexTileType::Plains;
				}
				else if (TerrainRoll < 0.70f)
				{
					NewTileType = ESimpleHexTileType::Desert;
				}
				else if (TerrainRoll < 0.88f)
				{
					NewTileType = ESimpleHexTileType::Tundra;
				}
				else
				{
					NewTileType = ESimpleHexTileType::Snow;
				}
			}

			Tiles[GetTileIndex(Q, R)].TileType = NewTileType;
		}
	}
}

void ASimpleHexGridActor::GenerateMesh()
{
	if (!GridMesh)
	{
		return;
	}

	GridMesh->ClearAllMeshSections();

	TArray<FSimpleHexMeshSection> MeshSections;
	MeshSections.SetNum(GetSectionCount());

	for (int32 R = 0; R < GridHeight; ++R)
	{
		for (int32 Q = 0; Q < GridWidth; ++Q)
		{
			AddHexTile(Q, R, MeshSections);
		}
	}

	for (int32 SectionIndex = 0; SectionIndex < MeshSections.Num(); ++SectionIndex)
	{
		FSimpleHexMeshSection& Section = MeshSections[SectionIndex];

		GridMesh->CreateMeshSection(
			SectionIndex,
			Section.Vertices,
			Section.Triangles,
			Section.Normals,
			Section.UVs,
			Section.VertexColors,
			Section.Tangents,
			true
		);

		if (TileMaterials.IsValidIndex(SectionIndex) && TileMaterials[SectionIndex])
		{
			GridMesh->SetMaterial(SectionIndex, TileMaterials[SectionIndex]);
		}
	}
}

void ASimpleHexGridActor::AddHexTile(
	int32 Q,
	int32 R,
	TArray<FSimpleHexMeshSection>& MeshSections
)
{
	if (!IsValidTile(Q, R))
	{
		return;
	}

	const FSimpleHexTileData& Tile = Tiles[GetTileIndex(Q, R)];
	const int32 SectionIndex = GetSectionIndexForTileType(Tile.TileType);

	if (!MeshSections.IsValidIndex(SectionIndex))
	{
		return;
	}

	FSimpleHexMeshSection& Section = MeshSections[SectionIndex];

	const FVector Center = GetHexCenter(Q, R);
	const FVector2D CenterUV(0.5f, 0.5f);

	TArray<FVector> Corners;
	TArray<FVector2D> CornerUVs;

	Corners.SetNum(6);
	CornerUVs.SetNum(6);

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		Corners[CornerIndex] = Center + GetHexCornerOffset(CornerIndex);
		CornerUVs[CornerIndex] = GetHexCornerUV(CornerIndex);
	}

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const int32 NextCornerIndex = (CornerIndex + 1) % 6;

		// Same winding style as your prototype's top faces.
		AddTriangle(
			Section,
			Center,
			Corners[NextCornerIndex],
			Corners[CornerIndex],
			CenterUV,
			CornerUVs[NextCornerIndex],
			CornerUVs[CornerIndex]
		);
	}
}

void ASimpleHexGridActor::AddTriangle(
	FSimpleHexMeshSection& Section,
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector2D& UVA,
	const FVector2D& UVB,
	const FVector2D& UVC
)
{
	AddVertex(Section, A, UVA);
	AddVertex(Section, B, UVB);
	AddVertex(Section, C, UVC);
}

void ASimpleHexGridActor::AddVertex(
	FSimpleHexMeshSection& Section,
	const FVector& Position,
	const FVector2D& UV
)
{
	const int32 NewIndex = Section.Vertices.Num();

	Section.Vertices.Add(Position);
	Section.Triangles.Add(NewIndex);
	Section.Normals.Add(FVector::UpVector);
	Section.UVs.Add(UV);
	Section.VertexColors.Add(FColor::White);
	Section.Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
}

FVector ASimpleHexGridActor::GetHexCenter(int32 Q, int32 R) const
{
	const float HexWidth = FMath::Sqrt(3.0f) * HexRadius;
	const float VerticalSpacing = 1.5f * HexRadius;

	const float X = HexWidth * (static_cast<float>(Q) + 0.5f * static_cast<float>(R & 1));
	const float Y = VerticalSpacing * static_cast<float>(R);

	return FVector(X, Y, 0.0f);
}

FVector ASimpleHexGridActor::GetHexCornerOffset(int32 CornerIndex) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	return FVector(
		HexRadius * FMath::Cos(AngleRad),
		HexRadius * FMath::Sin(AngleRad),
		0.0f
	);
}

FVector2D ASimpleHexGridActor::GetHexCornerUV(int32 CornerIndex) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	return FVector2D(
		0.5f + 0.5f * FMath::Cos(AngleRad),
		0.5f + 0.5f * FMath::Sin(AngleRad)
	);
}

int32 ASimpleHexGridActor::GetTileIndex(int32 Q, int32 R) const
{
	return R * GridWidth + Q;
}

bool ASimpleHexGridActor::IsValidTile(int32 Q, int32 R) const
{
	return Q >= 0 && Q < GridWidth && R >= 0 && R < GridHeight && Tiles.IsValidIndex(GetTileIndex(Q, R));
}

int32 ASimpleHexGridActor::GetSectionIndexForTileType(ESimpleHexTileType TileType) const
{
	return static_cast<int32>(TileType);
}

int32 ASimpleHexGridActor::GetSectionCount() const
{
	return static_cast<int32>(ESimpleHexTileType::Mountain) + 1;
}