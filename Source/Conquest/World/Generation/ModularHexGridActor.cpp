#include "ModularHexGridActor.h"

#include "ConquestGameSetupTypes.h"
#include "HexMapTypePresets.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"

AModularHexGridActor::AModularHexGridActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GridMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GridMesh"));
	GridMesh->SetupAttachment(SceneRoot);
	GridMesh->bUseAsyncCooking = true;
	GridMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	WaterMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterMesh"));
	WaterMesh->SetupAttachment(SceneRoot);
	WaterMesh->bUseAsyncCooking = true;

	RiverMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RiverMesh"));
	RiverMesh->SetupAttachment(SceneRoot);
	RiverMesh->bUseAsyncCooking = true;

	HexGridOverlayMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HexGridOverlayMesh"));
	HexGridOverlayMesh->SetupAttachment(SceneRoot);
	HexGridOverlayMesh->bUseAsyncCooking = true;

	FogOfWarMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FogOfWarMesh"));
	FogOfWarMesh->SetupAttachment(SceneRoot);
	FogOfWarMesh->bUseAsyncCooking = true;

	HoverHighlightMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HoverHighlightMesh"));
	HoverHighlightMesh->SetupAttachment(SceneRoot);
	HoverHighlightMesh->bUseAsyncCooking = true;
	HoverHighlightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoverHighlightMesh->SetCastShadow(false);
	HoverHighlightMesh->bCastDynamicShadow = false;
	HoverHighlightMesh->bCastStaticShadow = false;
	HoverHighlightMesh->CastShadow = false;
	HoverHighlightMesh->TranslucencySortPriority = 5;
	
	if (HoverHighlightMesh)
	{
		HoverHighlightMesh->SetVisibility(false);

		if (HoverHighlightMaterial)
		{
			HoverHighlightMesh->SetMaterial(0, HoverHighlightMaterial);
		}
	}
	
	EnsureDefaultGenerationRules();
	ConfigureMeshComponents();
}

void AModularHexGridActor::ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings)
{
	SizeSettings = SetupSettings.SizeSettings;

	SetMapTypePreset(SetupSettings.MapTypePreset);

	GenerationSettings.RandomSeed = SetupSettings.RandomSeed;
	GenerationSettings.TemperatureSettings = SetupSettings.TemperatureSettings;

	ResourceGenerationSettings = SetupSettings.ResourceGenerationSettings;
	RiverGenerationSettings = SetupSettings.RiverGenerationSettings;
}

void AModularHexGridActor::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		RebuildGrid();
	}
}

void AModularHexGridActor::RebuildGrid()
{
	EnsureDefaultGenerationRules();

	GridModel.Initialize(SizeSettings, HeightSettings, TileResourceData, ResourceSetData, ImprovementSetData);

	FHexGenerationSettings EffectiveGenerationSettings = GenerationSettings;

	FHexMapTypePreset Preset;
	if (FHexMapTypePresets::GetPreset(EffectiveGenerationSettings.MapTypePreset, Preset))
	{
		EffectiveGenerationSettings.MapShapeSettings = Preset.Shape;
		EffectiveGenerationSettings.TemperatureSettings.TemperatureBiasStrength = Preset.Shape.TemperatureBiasStrength;
		EffectiveGenerationSettings.TemperatureSettings.PolarFalloffPower = Preset.Shape.PolarFalloffPower;

		for (FHexTileGenerationRule& Rule : EffectiveGenerationSettings.GenerationRules)
		{
			if (const float* OverrideWeight = Preset.TerrainWeights.Find(Rule.TileType))
			{
				Rule.Weight = *OverrideWeight;
			}
		}
	}

	Generator.Generate(GridModel, EffectiveGenerationSettings);

	GridModel.ResolveTileHeights(EffectiveGenerationSettings.GenerationRules);
	GridModel.ResolveSharedVertexHeights();

	GridModel.ResolveSharedVertexHeightVariance(
		EffectiveGenerationSettings.GenerationRules,
		EffectiveGenerationSettings.RandomSeed
	);

	RiverGenerator.Generate(
		GridModel,
		RiverGenerationSettings,
		EffectiveGenerationSettings.RandomSeed
	);

	FeatureGenerator.Generate(
		GridModel,
		TileResourceData,
		FeatureGenerationSettings,
		EffectiveGenerationSettings.RandomSeed
	);

	ResourceGenerator.Generate(
		GridModel,
		ResourceSetData,
		ResourceGenerationSettings,
		EffectiveGenerationSettings.RandomSeed
	);

	GridModel.ResolveTileYields();

	MeshBuilder.BuildTerrainMesh(GridMesh, GridModel, TileResourceData);
	MeshBuilder.BuildWaterMesh(WaterMesh, GridModel, WaterSettings, TileResourceData);
	MeshBuilder.BuildRiverMesh(RiverMesh, GridModel, RiverGenerationSettings, TileResourceData);
	MeshBuilder.BuildGridOverlayMesh(HexGridOverlayMesh, GridModel, OverlaySettings, TileResourceData);


	FeatureMeshBuilder.BuildFeatureMeshes(
		this,
		SceneRoot,
		GridModel,
		TileResourceData,
		EffectiveGenerationSettings.RandomSeed,
		FeatureMeshComponents
	);
	
	ResourceMeshBuilder.BuildResourceMeshes(
		this,
		SceneRoot,
		GridModel,
		ResourceSetData,
		EffectiveGenerationSettings.RandomSeed,
		ResourceMeshComponents
	);
	

	if (bGenerateFogOfWar)
	{
		MeshBuilder.BuildFogOfWarMesh(
			FogOfWarMesh,
			GridModel,
			FogOfWarSettings
		);
	}
	else if (FogOfWarMesh)
	{
		FogOfWarMesh->ClearAllMeshSections();
		FogOfWarMesh->SetVisibility(false);
	}
}

void AModularHexGridActor::RegenerateGridWithNewRandomSeed()
{
	GenerationSettings.RandomSeed = FMath::RandRange(1, TNumericLimits<int32>::Max() - 1);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Regenerating hex grid with new random seed: %d"),
		GenerationSettings.RandomSeed
	);

	RebuildGrid();
}

void AModularHexGridActor::SetHoveredTile(int32 Q, int32 R)
{
	if (HoveredQ == Q && HoveredR == R)
	{
		return;
	}

	HoveredQ = Q;
	HoveredR = R;

	if (!HoverHighlightMesh || !GridModel.IsValidTile(Q, R))
	{
		ClearHoveredTile();
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	auto AddQuad = [&](
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector& D
)
	{
		const int32 StartIndex = Vertices.Num();

		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		Vertices.Add(D);

		// Flipped winding so the quad faces upward.
		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex + 1);

		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 3);
		Triangles.Add(StartIndex + 2);

		for (int32 i = 0; i < 4; ++i)
		{
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D::ZeroVector);
			VertexColors.Add(FColor::White);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}
	};

	const FVector Center = GridModel.GetHexCenter(Q, R);

	TArray<FVector> Corners;
	Corners.SetNum(6);

	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector FlatCorner = Center + GridModel.GetHexCornerOffset(CornerIndex);

		float CornerZ = 0.0f;

		if (GridModel.UsesHeightOffsets())
		{
			CornerZ = GridModel.GetResolvedCornerHeight(FlatCorner);
			CornerZ += GridModel.GetResolvedCornerHeightVarianceOffset(FlatCorner);
		}

		Corners[CornerIndex] = FVector(
			FlatCorner.X,
			FlatCorner.Y,
			CornerZ + HoverSurfaceOffset
		);
	}

	// Edge strips
	for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
	{
		const FVector A = Corners[EdgeIndex];
		const FVector B = Corners[(EdgeIndex + 1) % 6];

		FVector EdgeDirection = B - A;
		EdgeDirection.Z = 0.0f;
		EdgeDirection.Normalize();

		FVector ToCenter = Center - ((A + B) * 0.5f);
		ToCenter.Z = 0.0f;
		ToCenter.Normalize();

		const FVector HalfWidth = ToCenter * HoverEdgeWidth * 0.5f;

		AddQuad(
			A - HalfWidth,
			B - HalfWidth,
			B + HalfWidth,
			A + HalfWidth
		);
	}

	// Vertex highlights
	for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
	{
		const FVector Corner = Corners[CornerIndex];

		FVector ToCenter = Center - Corner;
		ToCenter.Z = 0.0f;
		ToCenter.Normalize();

		const FVector Right = FVector::CrossProduct(FVector::UpVector, ToCenter).GetSafeNormal();

		const FVector A = Corner + ToCenter * HoverVertexRadius;
		const FVector B = Corner + Right * HoverVertexRadius;
		const FVector C = Corner - ToCenter * HoverVertexRadius;
		const FVector D = Corner - Right * HoverVertexRadius;

		AddQuad(A, B, C, D);
	}

	HoverHighlightMesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false
	);

	if (HoverHighlightMaterial)
	{
		HoverHighlightMesh->SetMaterial(0, HoverHighlightMaterial);
	}

	HoverHighlightMesh->SetVisibility(true);
}

void AModularHexGridActor::ClearHoveredTile()
{
	HoveredQ = INDEX_NONE;
	HoveredR = INDEX_NONE;

	if (HoverHighlightMesh)
	{
		HoverHighlightMesh->ClearAllMeshSections();
		HoverHighlightMesh->SetVisibility(false);
	}
}

void AModularHexGridActor::SetMapTypePreset(EHexMapTypePreset MapTypePreset)
{
	GenerationSettings.MapTypePreset = MapTypePreset;
}

void AModularHexGridActor::SetHexGridOverlayVisible(bool bVisible)
{
	OverlaySettings.bShowHexGridOverlay = bVisible;
	if (HexGridOverlayMesh)
	{
		HexGridOverlayMesh->SetVisibility(OverlaySettings.bShowHexGridOverlay);
	}
}

void AModularHexGridActor::SetFogOfWarVisible(bool bVisible)
{
	bGenerateFogOfWar = bVisible;

	if (FogOfWarMesh)
	{
		FogOfWarMesh->SetVisibility(bGenerateFogOfWar);
	}
}

void AModularHexGridActor::SetWaterLayerVisible(bool bVisible)
{
	WaterSettings.bShowWaterLayer = bVisible;
	if (WaterMesh)
	{
		WaterMesh->SetVisibility(WaterSettings.bShowWaterLayer);
	}
}

void AModularHexGridActor::SetRiverLayerVisible(bool bVisible)
{
	RiverGenerationSettings.bShowRiverLayer = bVisible;

	if (RiverMesh)
	{
		RiverMesh->SetVisibility(RiverGenerationSettings.bShowRiverLayer);
	}
}

void AModularHexGridActor::GetPossibleImprovementIdsForTile(int32 Q, int32 R, TArray<FName>& OutImprovementIds) const
{
	GridModel.GetPossibleImprovementIdsForTile(Q, R, OutImprovementIds);
}

bool AModularHexGridActor::SetTileImprovement(int32 Q, int32 R, FName ImprovementId)
{
	const bool bChanged = GridModel.SetTileImprovement(Q, R, ImprovementId);
	if (bChanged)
	{
		// Terrain shape does not change yet, but this makes material/yield-driven visual changes easy later.
		MeshBuilder.BuildTerrainMesh(GridMesh, GridModel, TileResourceData);
	}
	return bChanged;
}

bool AModularHexGridActor::GetTileAtWorldLocation(
	const FVector& WorldLocation,
	int32& OutQ,
	int32& OutR,
	FHexTileData& OutTileData
) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);

	const TArray<FHexTileData>& Tiles = GridModel.GetTiles();

	float BestDistanceSq = TNumericLimits<float>::Max();
	int32 BestQ = INDEX_NONE;
	int32 BestR = INDEX_NONE;

	for (int32 R = 0; R < GridModel.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < GridModel.GetGridWidth(); ++Q)
		{
			if (!GridModel.IsValidTile(Q, R))
			{
				continue;
			}

			const FVector Center = GridModel.GetHexCenter(Q, R);
			const float DistanceSq = FVector::DistSquared2D(LocalLocation, Center);

			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestQ = Q;
				BestR = R;
			}
		}
	}

	if (BestQ == INDEX_NONE || BestR == INDEX_NONE)
	{
		return false;
	}

	// Basic range guard so we do not report a tile when hovering far outside the map.
	const float MaxDistance = GridModel.GetHexRadius();
	if (BestDistanceSq > FMath::Square(MaxDistance))
	{
		return false;
	}

	const int32 TileIndex = GridModel.GetTileIndex(BestQ, BestR);
	if (!Tiles.IsValidIndex(TileIndex))
	{
		return false;
	}

	OutQ = BestQ;
	OutR = BestR;
	OutTileData = Tiles[TileIndex];

	return true;
}

void AModularHexGridActor::EnsureDefaultGenerationRules()
{
	if (GenerationSettings.GenerationRules.Num() <= 0)
	{
		FHexMapGenerator::SetupDefaultGenerationRules(GenerationSettings.GenerationRules);
	}
}

void AModularHexGridActor::ConfigureMeshComponents()
{
	if (WaterMesh)
	{
		WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WaterMesh->SetVisibility(WaterSettings.bShowWaterLayer);
		WaterMesh->SetCastShadow(false);
		WaterMesh->bCastDynamicShadow = false;
		WaterMesh->bCastStaticShadow = false;
		WaterMesh->CastShadow = false;
		WaterMesh->TranslucencySortPriority = 1;
	}

	if (HexGridOverlayMesh)
	{
		HexGridOverlayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HexGridOverlayMesh->SetVisibility(OverlaySettings.bShowHexGridOverlay);
		HexGridOverlayMesh->SetCastShadow(false);
		HexGridOverlayMesh->bCastDynamicShadow = false;
		HexGridOverlayMesh->bCastStaticShadow = false;
		HexGridOverlayMesh->CastShadow = false;
		HexGridOverlayMesh->TranslucencySortPriority = 2;
	}

	if (FogOfWarMesh)
	{
		FogOfWarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FogOfWarMesh->SetVisibility(bGenerateFogOfWar);
		FogOfWarMesh->SetCastShadow(false);
		FogOfWarMesh->bCastDynamicShadow = false;
		FogOfWarMesh->bCastStaticShadow = false;
		FogOfWarMesh->CastShadow = false;
		FogOfWarMesh->TranslucencySortPriority = FogOfWarSettings.TranslucencySortPriority;
	}

	if (GridMesh)
	{
		GridMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GridMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		GridMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		GridMesh->bUseComplexAsSimpleCollision = true;
	}

	if (RiverMesh)
	{
		RiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RiverMesh->SetVisibility(RiverGenerationSettings.bShowRiverLayer);
		RiverMesh->SetCastShadow(false);
		RiverMesh->bCastDynamicShadow = false;
		RiverMesh->bCastStaticShadow = false;
		RiverMesh->CastShadow = false;
		RiverMesh->TranslucencySortPriority = RiverGenerationSettings.TranslucencySortPriority;
	}
}
