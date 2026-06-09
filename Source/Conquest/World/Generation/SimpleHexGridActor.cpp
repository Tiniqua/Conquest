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

	SetupDefaultGenerationRules();
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

void ASimpleHexGridActor::SetupDefaultGenerationRules()
{
	GenerationRules.Reset();

	GenerationRules.Add({
		ESimpleHexTileType::Ocean,
		8.0f,
		10,
		28,
		{
			ESimpleHexTileType::Ocean,
			ESimpleHexTileType::Coast
		},
		{
			ESimpleHexTileType::Grassland,
			ESimpleHexTileType::Plains,
			ESimpleHexTileType::Desert,
			ESimpleHexTileType::Tundra,
			ESimpleHexTileType::Snow,
			ESimpleHexTileType::Mountain
		},
		true,
		true,
		1.0f
	});

	GenerationRules.Add({
		ESimpleHexTileType::Coast,
		5.0f,
		3,
		10,
		{
			ESimpleHexTileType::Ocean,
			ESimpleHexTileType::Grassland,
			ESimpleHexTileType::Plains
		},
		{
			ESimpleHexTileType::Mountain,
			ESimpleHexTileType::Snow
		},
		true,
		true,
		2.0f
	});

	GenerationRules.Add({
		ESimpleHexTileType::Grassland,
		31.0f,
		8,
		28,
		{
			ESimpleHexTileType::Grassland,
			ESimpleHexTileType::Plains,
			ESimpleHexTileType::Coast
		},
		{
			ESimpleHexTileType::Ocean
		},
		false,
		false,
		0.0f
	});

	GenerationRules.Add({
		ESimpleHexTileType::Plains,
		27.0f,
		8,
		26,
		{
			ESimpleHexTileType::Plains,
			ESimpleHexTileType::Grassland,
			ESimpleHexTileType::Desert
		},
		{
			ESimpleHexTileType::Ocean
		},
		false,
		false,
		0.0f
	});

	GenerationRules.Add({
		ESimpleHexTileType::Desert,
		10.0f,
		5,
		18,
		{
			ESimpleHexTileType::Desert,
			ESimpleHexTileType::Plains
		},
		{
			ESimpleHexTileType::Ocean,
			ESimpleHexTileType::Snow,
			ESimpleHexTileType::Tundra
		},
		false,
		false,
		0.0f
	});

	GenerationRules.Add({
		ESimpleHexTileType::Tundra,
		8.0f,
		5,
		16,
		{
			ESimpleHexTileType::Tundra,
			ESimpleHexTileType::Snow,
			ESimpleHexTileType::Plains
		},
		{
			ESimpleHexTileType::Ocean,
			ESimpleHexTileType::Desert
		},
		false,
		false,
		0.0f
	});

	GenerationRules.Add({
		ESimpleHexTileType::Snow,
		5.0f,
		3,
		12,
		{
			ESimpleHexTileType::Snow,
			ESimpleHexTileType::Tundra,
			ESimpleHexTileType::Mountain
		},
		{
			ESimpleHexTileType::Ocean,
			ESimpleHexTileType::Desert,
			ESimpleHexTileType::Coast
		},
		false,
		false,
		0.0f
	});

	GenerationRules.Add({
		ESimpleHexTileType::Mountain,
		3.0f,
		2,
		8,
		{
			ESimpleHexTileType::Mountain,
			ESimpleHexTileType::Snow,
			ESimpleHexTileType::Tundra
		},
		{
			ESimpleHexTileType::Ocean,
			ESimpleHexTileType::Coast
		},
		false,
		false,
		0.0f
	});
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

	TArray<bool> Assigned;
	Assigned.Init(false, TotalTiles);

	FRandomStream RandomStream(RandomSeed);

	if (GenerationRules.Num() <= 0)
	{
		for (FSimpleHexTileData& Tile : Tiles)
		{
			Tile.TileType = ESimpleHexTileType::Grassland;
		}

		return;
	}

	TMap<ESimpleHexTileType, int32> DesiredCounts;
	BuildDesiredTileCounts(RandomStream, DesiredCounts);

	for (const FSimpleHexTileGenerationRule& Rule : GenerationRules)
	{
		int32 RemainingForType = DesiredCounts.FindRef(Rule.TileType);
		
		int32 FailedClumpAttempts = 0;

		while (RemainingForType > 0)
		{
			const int32 MinClump = FMath::Max(1, Rule.MinClumpSize);
			const int32 MaxClump = FMath::Max(MinClump, Rule.MaxClumpSize);

			const int32 TargetClumpSize = FMath::Min(
				RemainingForType,
				RandomStream.RandRange(MinClump, MaxClump)
			);

			int32 SeedQ = 0;
			int32 SeedR = 0;

			if (!FindSeedTileForRule(Rule, RandomStream, Assigned, SeedQ, SeedR))
			{
				break;
			}

			const int32 PlacedCount = GrowClump(
				Rule,
				SeedQ,
				SeedR,
				TargetClumpSize,
				RandomStream,
				Assigned
			);

			if (PlacedCount <= 0)
			{
				++FailedClumpAttempts;

				if (Rule.bSoftCount && FailedClumpAttempts >= 3)
				{
					break;
				}

				continue;
			}

			RemainingForType -= PlacedCount;

			if (PlacedCount < TargetClumpSize)
			{
				++FailedClumpAttempts;
			}
			else
			{
				FailedClumpAttempts = 0;
			}

			if (Rule.bSoftCount && FailedClumpAttempts >= 3)
			{
				break;
			}
		}
	}

	// Safety fallback. This should usually only fill tiny gaps left by blocked clump growth.
	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		if (!Assigned[Index])
		{
			Tiles[Index].TileType = PickWeightedTileType(RandomStream);
			Assigned[Index] = true;
		}
	}
}

void ASimpleHexGridActor::BuildDesiredTileCounts(
	FRandomStream& RandomStream,
	TMap<ESimpleHexTileType, int32>& OutDesiredCounts
) const
{
	OutDesiredCounts.Reset();

	const int32 TotalTiles = GridWidth * GridHeight;
	if (TotalTiles <= 0 || GenerationRules.Num() <= 0)
	{
		return;
	}

	TArray<float> AdjustedWeights;
	AdjustedWeights.SetNum(GenerationRules.Num());

	float TotalAdjustedWeight = 0.0f;

	for (int32 RuleIndex = 0; RuleIndex < GenerationRules.Num(); ++RuleIndex)
	{
		const FSimpleHexTileGenerationRule& Rule = GenerationRules[RuleIndex];

		const float BaseWeight = FMath::Max(0.0f, Rule.Weight);
		const float RandomScale = RandomStream.FRandRange(
			1.0f - CountRandomness,
			1.0f + CountRandomness
		);

		const float AdjustedWeight = FMath::Max(0.0f, BaseWeight * RandomScale);

		AdjustedWeights[RuleIndex] = AdjustedWeight;
		TotalAdjustedWeight += AdjustedWeight;
	}

	if (TotalAdjustedWeight <= 0.0f)
	{
		OutDesiredCounts.Add(ESimpleHexTileType::Grassland, TotalTiles);
		return;
	}

	int32 CountSoFar = 0;

	for (int32 RuleIndex = 0; RuleIndex < GenerationRules.Num(); ++RuleIndex)
	{
		const FSimpleHexTileGenerationRule& Rule = GenerationRules[RuleIndex];

		int32 Count = 0;

		if (RuleIndex == GenerationRules.Num() - 1)
		{
			Count = TotalTiles - CountSoFar;
		}
		else
		{
			const float Ratio = AdjustedWeights[RuleIndex] / TotalAdjustedWeight;
			Count = FMath::RoundToInt(static_cast<float>(TotalTiles) * Ratio);
			Count = FMath::Clamp(Count, 0, TotalTiles - CountSoFar);
		}

		OutDesiredCounts.FindOrAdd(Rule.TileType) += Count;
		CountSoFar += Count;
	}
}

bool ASimpleHexGridActor::FindSeedTileForRule(
	const FSimpleHexTileGenerationRule& Rule,
	FRandomStream& RandomStream,
	const TArray<bool>& Assigned,
	int32& OutQ,
	int32& OutR
) const
{
	const int32 TotalTiles = GridWidth * GridHeight;
	if (TotalTiles <= 0)
	{
		return false;
	}

	float BestScore = -1.0f;
	int32 BestIndex = INDEX_NONE;

	for (int32 Attempt = 0; Attempt < SeedSelectionAttempts; ++Attempt)
	{
		const int32 CandidateIndex = RandomStream.RandRange(0, TotalTiles - 1);

		if (!Assigned.IsValidIndex(CandidateIndex) || Assigned[CandidateIndex])
		{
			continue;
		}

		const int32 CandidateQ = CandidateIndex % GridWidth;
		const int32 CandidateR = CandidateIndex / GridWidth;

		const float Score = ScoreSeedTileForRule(
			Rule,
			CandidateQ,
			CandidateR,
			Assigned
		);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestIndex = CandidateIndex;
		}
	}

	if (BestIndex != INDEX_NONE)
	{
		OutQ = BestIndex % GridWidth;
		OutR = BestIndex / GridWidth;
		return true;
	}

	// Fallback deterministic scan.
	for (int32 Index = 0; Index < Assigned.Num(); ++Index)
	{
		if (!Assigned[Index])
		{
			OutQ = Index % GridWidth;
			OutR = Index / GridWidth;
			return true;
		}
	}

	return false;
}

float ASimpleHexGridActor::ScoreSeedTileForRule(
	const FSimpleHexTileGenerationRule& Rule,
	int32 Q,
	int32 R,
	const TArray<bool>& Assigned
) const
{
	return ScoreTileForRuleAdjacency(Rule, Q, R, Assigned);
}

float ASimpleHexGridActor::ScoreTileForRuleAdjacency(
	const FSimpleHexTileGenerationRule& Rule,
	int32 Q,
	int32 R,
	const TArray<bool>& Assigned
) const
{
	float Score = 1.0f;

	for (int32 Direction = 0; Direction < 6; ++Direction)
	{
		int32 NeighbourQ = 0;
		int32 NeighbourR = 0;

		if (!GetNeighbourCoord(Q, R, Direction, NeighbourQ, NeighbourR))
		{
			continue;
		}

		const int32 NeighbourIndex = GetTileIndex(NeighbourQ, NeighbourR);

		if (!Tiles.IsValidIndex(NeighbourIndex) ||
			!Assigned.IsValidIndex(NeighbourIndex) ||
			!Assigned[NeighbourIndex])
		{
			continue;
		}

		const ESimpleHexTileType NeighbourType = Tiles[NeighbourIndex].TileType;

		if (NeighbourType == Rule.TileType)
		{
			Score += SameTypeAdjacencyBonus;
		}

		if (Rule.PreferredAdjacentTypes.Contains(NeighbourType))
		{
			Score += PreferredAdjacencyBonus;
		}

		if (Rule.AvoidAdjacentTypes.Contains(NeighbourType))
		{
			if (Rule.TileType == ESimpleHexTileType::Ocean)
			{
				Score -= HardAvoidAdjacencyPenalty;
			}
			else
			{
				Score -= AvoidAdjacencyPenalty;
			}
		}
	}

	return Score;
}

int32 ASimpleHexGridActor::GrowClump(
	const FSimpleHexTileGenerationRule& Rule,
	int32 SeedQ,
	int32 SeedR,
	int32 TargetSize,
	FRandomStream& RandomStream,
	TArray<bool>& Assigned
)
{
	if (!IsValidCoord(SeedQ, SeedR) || TargetSize <= 0)
	{
		return 0;
	}

	TArray<FIntPoint> Frontier;
	Frontier.Add(FIntPoint(SeedQ, SeedR));

	int32 PlacedCount = 0;

	while (Frontier.Num() > 0 && PlacedCount < TargetSize)
	{
		const int32 ChosenFrontierIndex = PickBestFrontierIndex(
			Rule,
			Frontier,
			RandomStream,
			Assigned
		);

		if (!Frontier.IsValidIndex(ChosenFrontierIndex))
		{
			break;
		}

		const FIntPoint Coord = Frontier[ChosenFrontierIndex];
		Frontier.RemoveAtSwap(ChosenFrontierIndex);

		if (!IsValidCoord(Coord.X, Coord.Y))
		{
			continue;
		}

		const int32 TileIndex = GetTileIndex(Coord.X, Coord.Y);

		if (!Assigned.IsValidIndex(TileIndex) || Assigned[TileIndex])
		{
			continue;
		}

		Tiles[TileIndex].TileType = Rule.TileType;
		Assigned[TileIndex] = true;
		++PlacedCount;

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			int32 NeighbourQ = 0;
			int32 NeighbourR = 0;

			if (!GetNeighbourCoord(Coord.X, Coord.Y, Direction, NeighbourQ, NeighbourR))
			{
				continue;
			}

			const int32 NeighbourIndex = GetTileIndex(NeighbourQ, NeighbourR);

			if (Assigned.IsValidIndex(NeighbourIndex) && !Assigned[NeighbourIndex])
			{
				Frontier.AddUnique(FIntPoint(NeighbourQ, NeighbourR));
			}
		}
	}

	return PlacedCount;
}

int32 ASimpleHexGridActor::PickBestFrontierIndex(
	const FSimpleHexTileGenerationRule& Rule,
	const TArray<FIntPoint>& Frontier,
	FRandomStream& RandomStream,
	const TArray<bool>& Assigned
) const
{
	if (Frontier.Num() <= 0)
	{
		return INDEX_NONE;
	}

	TArray<float> Scores;
	Scores.SetNum(Frontier.Num());

	float TotalScore = 0.0f;

	for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
	{
		const FIntPoint Coord = Frontier[FrontierIndex];

		float Score = ScoreTileForRuleAdjacency(
			Rule,
			Coord.X,
			Coord.Y,
			Assigned
		);

		if (Rule.bRejectBadAdjacency && Score < Rule.MinPlacementScore)
		{
			Score = 0.0f;
		}
		else
		{
			Score = FMath::Max(0.05f, Score);
		}

		Scores[FrontierIndex] = Score;
		TotalScore += Score;
	}

	if (TotalScore <= 0.0f)
	{
		return INDEX_NONE;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalScore);

	for (int32 ScoreIndex = 0; ScoreIndex < Scores.Num(); ++ScoreIndex)
	{
		Roll -= Scores[ScoreIndex];

		if (Roll <= 0.0f)
		{
			return ScoreIndex;
		}
	}

	return Frontier.Num() - 1;
}

ESimpleHexTileType ASimpleHexGridActor::PickWeightedTileType(
	FRandomStream& RandomStream
) const
{
	float TotalWeight = 0.0f;

	for (const FSimpleHexTileGenerationRule& Rule : GenerationRules)
	{
		TotalWeight += FMath::Max(0.0f, Rule.Weight);
	}

	if (TotalWeight <= 0.0f)
	{
		return ESimpleHexTileType::Grassland;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);

	for (const FSimpleHexTileGenerationRule& Rule : GenerationRules)
	{
		Roll -= FMath::Max(0.0f, Rule.Weight);

		if (Roll <= 0.0f)
		{
			return Rule.TileType;
		}
	}

	return GenerationRules.Last().TileType;
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

bool ASimpleHexGridActor::GetNeighbourCoord(
	int32 Q,
	int32 R,
	int32 Direction,
	int32& OutQ,
	int32& OutR
) const
{
	const bool bOddRow = (R & 1) != 0;

	switch (Direction)
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

	return IsValidCoord(OutQ, OutR);
}

int32 ASimpleHexGridActor::GetTileIndex(int32 Q, int32 R) const
{
	return R * GridWidth + Q;
}

bool ASimpleHexGridActor::IsValidCoord(int32 Q, int32 R) const
{
	return Q >= 0 && Q < GridWidth && R >= 0 && R < GridHeight;
}

bool ASimpleHexGridActor::IsValidTile(int32 Q, int32 R) const
{
	if (!IsValidCoord(Q, R))
	{
		return false;
	}

	return Tiles.IsValidIndex(GetTileIndex(Q, R));
}

int32 ASimpleHexGridActor::GetSectionIndexForTileType(ESimpleHexTileType TileType) const
{
	return static_cast<int32>(TileType);
}

int32 ASimpleHexGridActor::GetSectionCount() const
{
	return static_cast<int32>(ESimpleHexTileType::Mountain) + 1;
}