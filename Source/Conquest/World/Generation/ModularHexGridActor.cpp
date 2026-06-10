#include "ModularHexGridActor.h"

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

	HexGridOverlayMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HexGridOverlayMesh"));
	HexGridOverlayMesh->SetupAttachment(SceneRoot);
	HexGridOverlayMesh->bUseAsyncCooking = true;

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

	Generator.Generate(GridModel, GenerationSettings);

	ResourceGenerator.Generate(
		GridModel,
		ResourceSetData,
		ResourceGenerationSettings,
		GenerationSettings.RandomSeed
	);

	GridModel.ResolveTileHeights(GenerationSettings.GenerationRules);
	GridModel.ResolveSharedVertexHeights();
	GridModel.ResolveTileYields();

	MeshBuilder.BuildTerrainMesh(GridMesh, GridModel, TileResourceData);
	MeshBuilder.BuildWaterMesh(WaterMesh, GridModel, WaterSettings, TileResourceData);
	MeshBuilder.BuildGridOverlayMesh(HexGridOverlayMesh, GridModel, OverlaySettings, TileResourceData);

	ResourceMeshBuilder.BuildResourceMeshes(
		this,
		SceneRoot,
		GridModel,
		ResourceSetData,
		GenerationSettings.RandomSeed,
		ResourceMeshComponents
	);

}

void AModularHexGridActor::SetHexGridOverlayVisible(bool bVisible)
{
	OverlaySettings.bShowHexGridOverlay = bVisible;
	if (HexGridOverlayMesh)
	{
		HexGridOverlayMesh->SetVisibility(OverlaySettings.bShowHexGridOverlay);
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
}
