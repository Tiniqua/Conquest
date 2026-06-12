#include "ModularHexGridActor.h"

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
	
	EnsureDefaultGenerationRules();
	ConfigureMeshComponents();
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

	RiverGenerator.Generate(
		GridModel,
		RiverGenerationSettings,
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

		if (!bGenerateFogOfWar)
		{
			FogOfWarMesh->ClearAllMeshSections();
		}
	}

	if (bGenerateFogOfWar && FogOfWarMesh && FogOfWarMesh->GetNumSections() <= 0)
	{
		MeshBuilder.BuildFogOfWarMesh(
			FogOfWarMesh,
			GridModel,
			FogOfWarSettings
		);
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
