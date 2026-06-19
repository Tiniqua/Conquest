#include "ModularHexGridActor.h"

#include "ConquestGameSetupTypes.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "HexMapTypePresets.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

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

	CivilisationBorderMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CivilisationBorderMesh"));
	CivilisationBorderMesh->SetupAttachment(SceneRoot);
	CivilisationBorderMesh->bUseAsyncCooking = true;
	CivilisationBorderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CivilisationBorderMesh->SetCastShadow(false);
	CivilisationBorderMesh->bCastDynamicShadow = false;
	CivilisationBorderMesh->bCastStaticShadow = false;
	CivilisationBorderMesh->CastShadow = false;
	CivilisationBorderMesh->TranslucencySortPriority = 4;
	
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

void AModularHexGridActor::EnsureCityPlaceholderMeshComponent()
{
	if (CityPlaceholderMeshComponent)
	{
		return;
	}

	if (!CityPlaceholderMesh)
	{
		return;
	}

	const FName ComponentName = MakeUniqueObjectName(
		this,
		UInstancedStaticMeshComponent::StaticClass(),
		TEXT("CityPlaceholderMesh")
	);

	CityPlaceholderMeshComponent =
		NewObject<UInstancedStaticMeshComponent>(this, ComponentName);

	if (!CityPlaceholderMeshComponent)
	{
		return;
	}

	CityPlaceholderMeshComponent->SetStaticMesh(CityPlaceholderMesh);

	if (CityPlaceholderMaterialOverride)
	{
		const int32 MaterialSlotCount = CityPlaceholderMesh->GetStaticMaterials().Num();

		for (int32 MaterialIndex = 0; MaterialIndex < MaterialSlotCount; ++MaterialIndex)
		{
			CityPlaceholderMeshComponent->SetMaterial(
				MaterialIndex,
				CityPlaceholderMaterialOverride
			);
		}
	}

	CityPlaceholderMeshComponent->SetupAttachment(SceneRoot);
	CityPlaceholderMeshComponent->RegisterComponent();

	CityPlaceholderMeshComponent->SetMobility(EComponentMobility::Static);
	CityPlaceholderMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CityPlaceholderMeshComponent->SetGenerateOverlapEvents(false);

	CityPlaceholderMeshComponent->bCastDynamicShadow = true;
	CityPlaceholderMeshComponent->bCastStaticShadow = true;
	CityPlaceholderMeshComponent->CastShadow = true;

	AddInstanceComponent(CityPlaceholderMeshComponent);
}

FTransform AModularHexGridActor::BuildCityPlaceholderTransform(const FIntPoint& Coord) const
{
	const FVector FlatCenter = GridModel.GetHexCenter(Coord.X, Coord.Y);

	float SurfaceZ = 0.0f;

	const FHexTileData* Tile = GridModel.GetTile(Coord);
	if (Tile)
	{
		SurfaceZ = Tile->Height;
	}

	const FVector Location(
		FlatCenter.X,
		FlatCenter.Y,
		SurfaceZ
	);

	return FTransform(
		CityPlaceholderRotation,
		Location + CityPlaceholderOffset,
		CityPlaceholderScale
	);
}

void AModularHexGridActor::AddCityPlaceholder(int32 CityId, const FIntPoint& Coord)
{
	if (CityId == INDEX_NONE)
	{
		return;
	}

	if (!GridModel.IsValidTile(Coord.X, Coord.Y))
	{
		return;
	}

	EnsureCityPlaceholderMeshComponent();

	if (!CityPlaceholderMeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddCityPlaceholder failed: CityPlaceholderMeshComponent is null. Assign CityPlaceholderMesh on the grid actor."));
		return;
	}

	if (CityIdToPlaceholderInstanceIndex.Contains(CityId))
	{
		return;
	}

	const FTransform InstanceTransform = BuildCityPlaceholderTransform(Coord);
	const int32 InstanceIndex = CityPlaceholderMeshComponent->AddInstance(InstanceTransform);

	CityIdToPlaceholderInstanceIndex.Add(CityId, InstanceIndex);
	CityPlaceholderMeshComponent->MarkRenderStateDirty();
}

void AModularHexGridActor::ClearCityPlaceholders()
{
	CityIdToPlaceholderInstanceIndex.Reset();

	if (CityPlaceholderMeshComponent)
	{
		CityPlaceholderMeshComponent->ClearInstances();
		CityPlaceholderMeshComponent->MarkRenderStateDirty();
	}
}

void AModularHexGridActor::RebuildCivilisationBorders(int32 OwnerPlayerId, UMaterialInterface* BorderMaterial)
{
	if (!CivilisationBorderMesh)
	{
		return;
	}

	CivilisationBorderMesh->ClearAllMeshSections();

	if (OwnerPlayerId == INDEX_NONE)
	{
		CivilisationBorderMesh->SetVisibility(false);
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

	for (int32 R = 0; R < GridModel.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < GridModel.GetGridWidth(); ++Q)
		{
			const FHexTileData* Tile = GridModel.GetTile(Q, R);
			if (!Tile || Tile->Gameplay.OwnerPlayerId != OwnerPlayerId)
			{
				continue;
			}

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
					CornerZ + CivilisationBorderSurfaceOffset
				);
			}

			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				int32 NeighborQ = INDEX_NONE;
				int32 NeighborR = INDEX_NONE;
				const bool bHasNeighbor = GridModel.GetNeighbourCoord(Q, R, EdgeIndex, NeighborQ, NeighborR);
				const FHexTileData* NeighborTile = bHasNeighbor ? GridModel.GetTile(NeighborQ, NeighborR) : nullptr;

				if (NeighborTile && NeighborTile->Gameplay.OwnerPlayerId == OwnerPlayerId)
				{
					continue;
				}

				const FVector A = Corners[EdgeIndex];
				const FVector B = Corners[(EdgeIndex + 1) % 6];

				FVector ToCenter = Center - ((A + B) * 0.5f);
				ToCenter.Z = 0.0f;
				ToCenter.Normalize();

				const FVector HalfWidth = ToCenter * CivilisationBorderEdgeWidth * 0.5f;

				AddQuad(
					A - HalfWidth,
					B - HalfWidth,
					B + HalfWidth,
					A + HalfWidth
				);
			}
		}
	}

	if (Vertices.Num() <= 0)
	{
		CivilisationBorderMesh->SetVisibility(false);
		return;
	}

	CivilisationBorderMesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false
	);

	UMaterialInterface* EffectiveMaterial = BorderMaterial ? BorderMaterial : DefaultCivilisationBorderMaterial.Get();
	if (EffectiveMaterial)
	{
		CivilisationBorderMesh->SetMaterial(0, EffectiveMaterial);
	}

	CivilisationBorderMesh->SetVisibility(true);
}

void AModularHexGridActor::ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings)
{
	SizeSettings = SetupSettings.SizeSettings;

	SetMapTypePreset(SetupSettings.MapTypePreset);

	GenerationSettings.RandomSeed = SetupSettings.RandomSeed;
	GenerationSettings.TemperatureSettings = SetupSettings.TemperatureSettings;
	if (SetupSettings.bUseCustomMapShapeSettings)
	{
		GenerationSettings.MapShapeSettings = SetupSettings.MapShapeSettings;
	}
	GenerationSettings.bUseCustomMapShapeSettings = SetupSettings.bUseCustomMapShapeSettings;
	GenerationSettings.MountainWeightScale = SetupSettings.MountainWeightScale;

	ResourceGenerationSettings = SetupSettings.ResourceGenerationSettings;

	const FHexSimpleRiverSettings ExistingRiverSettings = RiverSettings;
	RiverSettings = SetupSettings.RiverSettings;

	if (!RiverSettings.RiverMaterial)
	{
		RiverSettings.RiverMaterial = ExistingRiverSettings.RiverMaterial;
	}

	RiverSettings.RiverWidth = ExistingRiverSettings.RiverWidth;
	RiverSettings.RiverSurfaceOffset = ExistingRiverSettings.RiverSurfaceOffset;
	RiverSettings.TranslucencySortPriority = ExistingRiverSettings.TranslucencySortPriority;

	if (HasActorBegunPlay() && bGenerateOnBeginPlay)
	{
		RebuildGrid();
	}
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
		if (!EffectiveGenerationSettings.bUseCustomMapShapeSettings)
		{
			EffectiveGenerationSettings.MapShapeSettings = Preset.Shape;
		}

		if (!EffectiveGenerationSettings.bUseCustomMapShapeSettings)
		{
			EffectiveGenerationSettings.TemperatureSettings.TemperatureBiasStrength = Preset.Shape.TemperatureBiasStrength;
			EffectiveGenerationSettings.TemperatureSettings.PolarFalloffPower = Preset.Shape.PolarFalloffPower;
		}

		for (FHexTileGenerationRule& Rule : EffectiveGenerationSettings.GenerationRules)
		{
			if (const float* OverrideWeight = Preset.TerrainWeights.Find(Rule.TileType))
			{
				Rule.Weight = *OverrideWeight;
			}

			if (Rule.TileType == EHexTileType::Mountain)
			{
				Rule.Weight *= FMath::Max(0.0f, EffectiveGenerationSettings.MountainWeightScale);
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
		RiverSettings,
		EffectiveGenerationSettings.RandomSeed,
		GeneratedRivers
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
	MeshBuilder.BuildSimpleRiverMesh(RiverMesh, GridModel, GeneratedRivers, RiverSettings);
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

	ClearCityPlaceholders();
	if (CivilisationBorderMesh)
	{
		CivilisationBorderMesh->ClearAllMeshSections();
		CivilisationBorderMesh->SetVisibility(false);
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
	RiverSettings.bShowRiverLayer = bVisible;
	if (RiverMesh)
	{
		RiverMesh->SetVisibility(RiverSettings.bShowRiverLayer);
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

		if (const FHexTileData* Tile = GridModel.GetTile(Q, R))
		{
			AConquestGameState* ConquestGameState = GetWorld()
				? GetWorld()->GetGameState<AConquestGameState>()
				: nullptr;

			if (
				ConquestGameState &&
				ConquestGameState->CityManager &&
				Tile->Gameplay.OwningCityId != INDEX_NONE
			)
			{
				ConquestGameState->CityManager->RefreshCityYields(Tile->Gameplay.OwningCityId);
			}
		}
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

	if (RiverMesh)
	{
		RiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RiverMesh->SetVisibility(RiverSettings.bShowRiverLayer);
		RiverMesh->SetCastShadow(false);
		RiverMesh->bCastDynamicShadow = false;
		RiverMesh->bCastStaticShadow = false;
		RiverMesh->CastShadow = false;
		RiverMesh->TranslucencySortPriority = RiverSettings.TranslucencySortPriority;
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

	if (CivilisationBorderMesh)
	{
		CivilisationBorderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CivilisationBorderMesh->SetCastShadow(false);
		CivilisationBorderMesh->bCastDynamicShadow = false;
		CivilisationBorderMesh->bCastStaticShadow = false;
		CivilisationBorderMesh->CastShadow = false;
		CivilisationBorderMesh->TranslucencySortPriority = 4;
		CivilisationBorderMesh->SetVisibility(false);
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

}
