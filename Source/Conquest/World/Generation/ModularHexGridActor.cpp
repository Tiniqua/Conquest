#include "ModularHexGridActor.h"

#include "ConquestGameSetupTypes.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "HexMapTypePresets.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Conquest/UI/ConquestCityWorldLabelWidget.h"
#include "Conquest/UI/ConquestTileHealthBarWidget.h"
#include "Conquest/UI/ConquestUnitWorldIconWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AModularHexGridActor::AModularHexGridActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FClassFinder<UConquestUnitWorldIconWidget> UnitWorldIconWidgetFinder(
		TEXT("/Game/UI/WBP_UnitIcon")
	);
	if (UnitWorldIconWidgetFinder.Succeeded())
	{
		UnitWorldIconWidgetClass = UnitWorldIconWidgetFinder.Class;
	}

	TileHealthBarWidgetClass = UConquestTileHealthBarWidget::StaticClass();

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

	TileYieldBorderMeshes.Reserve(5);
	TileYieldFillMeshes.Reserve(5);
	TileYieldTextMeshes.Reserve(5);
	for (int32 YieldLayerIndex = 0; YieldLayerIndex < 5; ++YieldLayerIndex)
	{
		const FName BorderComponentName(*FString::Printf(TEXT("TileYieldBorderMesh_%d"), YieldLayerIndex));
		UProceduralMeshComponent* YieldBorderMesh = CreateDefaultSubobject<UProceduralMeshComponent>(
			BorderComponentName
		);
		YieldBorderMesh->SetupAttachment(SceneRoot);
		YieldBorderMesh->bUseAsyncCooking = true;
		TileYieldBorderMeshes.Add(YieldBorderMesh);

		const FName FillComponentName(*FString::Printf(TEXT("TileYieldFillMesh_%d"), YieldLayerIndex));
		UProceduralMeshComponent* YieldFillMesh = CreateDefaultSubobject<UProceduralMeshComponent>(
			FillComponentName
		);
		YieldFillMesh->SetupAttachment(SceneRoot);
		YieldFillMesh->bUseAsyncCooking = true;
		TileYieldFillMeshes.Add(YieldFillMesh);

		const FName TextComponentName(*FString::Printf(TEXT("TileYieldTextMesh_%d"), YieldLayerIndex));
		UProceduralMeshComponent* YieldTextMesh = CreateDefaultSubobject<UProceduralMeshComponent>(
			TextComponentName
		);
		YieldTextMesh->SetupAttachment(SceneRoot);
		YieldTextMesh->bUseAsyncCooking = true;
		TileYieldTextMeshes.Add(YieldTextMesh);
	}

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
	CivilisationBorderMesh->TranslucencySortPriority = CivilisationBorderTranslucencySortPriority;

	CivilisationBorderFillMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CivilisationBorderFillMesh"));
	CivilisationBorderFillMesh->SetupAttachment(SceneRoot);
	CivilisationBorderFillMesh->bUseAsyncCooking = true;
	CivilisationBorderFillMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CivilisationBorderFillMesh->SetCastShadow(false);
	CivilisationBorderFillMesh->bCastDynamicShadow = false;
	CivilisationBorderFillMesh->bCastStaticShadow = false;
	CivilisationBorderFillMesh->CastShadow = false;
	CivilisationBorderFillMesh->TranslucencySortPriority = CivilisationBorderFillTranslucencySortPriority;

	ExpansionCandidateMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ExpansionCandidateMesh"));
	ExpansionCandidateMesh->SetupAttachment(SceneRoot);
	ExpansionCandidateMesh->bUseAsyncCooking = true;
	ExpansionCandidateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExpansionCandidateMesh->SetCastShadow(false);
	ExpansionCandidateMesh->bCastDynamicShadow = false;
	ExpansionCandidateMesh->bCastStaticShadow = false;
	ExpansionCandidateMesh->CastShadow = false;
	ExpansionCandidateMesh->TranslucencySortPriority = 6;
	
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

UInstancedStaticMeshComponent* AModularHexGridActor::EnsureCityPlaceholderMeshComponent(UStaticMesh* OverrideMesh, UMaterialInterface* OverrideMaterial)
{
	UStaticMesh* EffectiveMesh = OverrideMesh ? OverrideMesh : CityPlaceholderMesh.Get();
	UMaterialInterface* EffectiveMaterial = OverrideMaterial ? OverrideMaterial : CityPlaceholderMaterialOverride.Get();

	if (!EffectiveMesh)
	{
		return nullptr;
	}

	const int32 VisualKey = HashCombine(
		PointerHash(EffectiveMesh),
		PointerHash(EffectiveMaterial)
	);

	if (TObjectPtr<UInstancedStaticMeshComponent>* ExistingComponent = CityPlaceholderMeshComponentsByVisualKey.Find(VisualKey))
	{
		return ExistingComponent->Get();
	}

	const FName ComponentName = MakeUniqueObjectName(
		this,
		UInstancedStaticMeshComponent::StaticClass(),
		TEXT("CityPlaceholderMesh")
	);

	UInstancedStaticMeshComponent* NewCityPlaceholderMeshComponent =
		NewObject<UInstancedStaticMeshComponent>(this, ComponentName);

	if (!NewCityPlaceholderMeshComponent)
	{
		return nullptr;
	}

	NewCityPlaceholderMeshComponent->SetStaticMesh(EffectiveMesh);

	if (EffectiveMaterial)
	{
		const int32 MaterialSlotCount = EffectiveMesh->GetStaticMaterials().Num();

		for (int32 MaterialIndex = 0; MaterialIndex < MaterialSlotCount; ++MaterialIndex)
		{
			NewCityPlaceholderMeshComponent->SetMaterial(
				MaterialIndex,
				EffectiveMaterial
			);
		}
	}

	NewCityPlaceholderMeshComponent->SetupAttachment(SceneRoot);
	NewCityPlaceholderMeshComponent->RegisterComponent();

	NewCityPlaceholderMeshComponent->SetMobility(EComponentMobility::Static);
	NewCityPlaceholderMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewCityPlaceholderMeshComponent->SetGenerateOverlapEvents(false);

	NewCityPlaceholderMeshComponent->bCastDynamicShadow = true;
	NewCityPlaceholderMeshComponent->bCastStaticShadow = true;
	NewCityPlaceholderMeshComponent->CastShadow = true;

	AddInstanceComponent(NewCityPlaceholderMeshComponent);

	CityPlaceholderMeshComponentsByVisualKey.Add(VisualKey, NewCityPlaceholderMeshComponent);

	if (!OverrideMesh && !OverrideMaterial)
	{
		CityPlaceholderMeshComponent = NewCityPlaceholderMeshComponent;
	}

	return NewCityPlaceholderMeshComponent;
}

FTransform AModularHexGridActor::BuildCityPlaceholderTransform(
	const FIntPoint& Coord,
	bool bOverrideScale,
	const FVector& OverrideScale
) const
{
	const FVector FlatCenter = GridModel.GetHexCenter(Coord.X, Coord.Y);

	float SurfaceZ = 0.0f;

	const FHexTileData* Tile = GridModel.GetTile(Coord);
	if (Tile)
	{
		SurfaceZ = Tile->Height;
	}

	FVector LocalSurfaceLocation(
		FlatCenter.X,
		FlatCenter.Y,
		SurfaceZ
	);

	if (UWorld* World = GetWorld())
	{
		const float TraceHeight = FMath::Max(100.0f, CityPlaceholderGroundTraceHeight);
		const FVector LocalTraceStart(FlatCenter.X, FlatCenter.Y, SurfaceZ + TraceHeight);
		const FVector LocalTraceEnd(FlatCenter.X, FlatCenter.Y, SurfaceZ - TraceHeight);

		const FVector WorldTraceStart = GetActorTransform().TransformPosition(LocalTraceStart);
		const FVector WorldTraceEnd = GetActorTransform().TransformPosition(LocalTraceEnd);

		FHitResult HitResult;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CityPlaceholderGroundTrace), false, this);

		if (World->LineTraceSingleByChannel(
			HitResult,
			WorldTraceStart,
			WorldTraceEnd,
			ECC_Visibility,
			QueryParams
		))
		{
			LocalSurfaceLocation = GetActorTransform().InverseTransformPosition(HitResult.ImpactPoint);
		}
	}

	const FVector FinalScale = bOverrideScale ? OverrideScale : CityPlaceholderScale;

	return FTransform(
		CityPlaceholderRotation,
		LocalSurfaceLocation + CityPlaceholderOffset,
		FinalScale
	);
}

FTransform AModularHexGridActor::BuildCityWorldLabelTransform(const FIntPoint& Coord) const
{
	const FTransform SurfaceTransform = BuildCityPlaceholderTransform(Coord, true, FVector::OneVector);
	return FTransform(
		CityWorldLabelRotation,
		SurfaceTransform.GetLocation() + CityWorldLabelOffset,
		FVector(CityWorldLabelScale, CityWorldLabelScale, CityWorldLabelScale)
	);
}

FTransform AModularHexGridActor::BuildTileHealthBarTransform(const FIntPoint& Coord) const
{
	const FTransform SurfaceTransform = BuildCityPlaceholderTransform(Coord, true, FVector::OneVector);
	return FTransform(
		TileHealthBarRotation,
		SurfaceTransform.GetLocation() + TileHealthBarOffset,
		FVector(TileHealthBarScale, TileHealthBarScale, TileHealthBarScale)
	);
}

void AModularHexGridActor::AddCityPlaceholder(
	int32 CityId,
	const FIntPoint& Coord,
	UStaticMesh* OverrideMesh,
	UMaterialInterface* OverrideMaterial,
	bool bOverrideScale,
	const FVector& OverrideScale
)
{
	if (CityId == INDEX_NONE)
	{
		return;
	}

	if (!GridModel.IsValidTile(Coord.X, Coord.Y))
	{
		return;
	}

	UStaticMesh* EffectiveMesh = OverrideMesh ? OverrideMesh : CityPlaceholderMesh.Get();
	UMaterialInterface* EffectiveMaterial = OverrideMaterial ? OverrideMaterial : CityPlaceholderMaterialOverride.Get();
	UInstancedStaticMeshComponent* CityMeshComponent = EnsureCityPlaceholderMeshComponent(OverrideMesh, OverrideMaterial);

	if (!CityMeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddCityPlaceholder failed: CityPlaceholderMeshComponent is null. Assign CityPlaceholderMesh on the grid actor."));
		return;
	}

	if (CityIdToPlaceholderInstanceIndex.Contains(CityId))
	{
		return;
	}

	const FTransform InstanceTransform = BuildCityPlaceholderTransform(Coord, bOverrideScale, OverrideScale);
	const int32 InstanceIndex = CityMeshComponent->AddInstance(InstanceTransform);
	const int32 VisualKey = HashCombine(
		PointerHash(EffectiveMesh),
		PointerHash(EffectiveMaterial)
	);

	CityIdToPlaceholderInstanceIndex.Add(CityId, InstanceIndex);
	CityIdToPlaceholderVisualKey.Add(CityId, VisualKey);
	CityMeshComponent->MarkRenderStateDirty();
}

void AModularHexGridActor::AddOrUpdateCityWorldLabel(
	int32 CityId,
	const FIntPoint& Coord,
	FName CityName,
	int32 Population,
	int32 CurrentHealth,
	int32 MaxHealth,
	int32 Strength,
	float GrowthPercent,
	UMaterialInterface* CivilisationThemeMaterial,
	FLinearColor CivilisationThemeColor
)
{
	if (CityId == INDEX_NONE || !CityWorldLabelWidgetClass)
	{
		return;
	}

	UWidgetComponent* LabelComponent = nullptr;
	if (TObjectPtr<UWidgetComponent>* ExistingComponent = CityWorldLabelComponents.Find(CityId))
	{
		LabelComponent = ExistingComponent->Get();
	}

	bool bShouldRemainVisible = true;
	if (!LabelComponent)
	{
		const FName ComponentName = MakeUniqueObjectName(
			this,
			UWidgetComponent::StaticClass(),
			TEXT("CityWorldLabel")
		);

		LabelComponent = NewObject<UWidgetComponent>(this, ComponentName);
		if (!LabelComponent)
		{
			return;
		}

		LabelComponent->SetupAttachment(SceneRoot);
		LabelComponent->RegisterComponent();
		LabelComponent->SetWidgetClass(CityWorldLabelWidgetClass);
		LabelComponent->SetWidgetSpace(EWidgetSpace::World);
		LabelComponent->SetDrawSize(CityWorldLabelDrawSize);
		LabelComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LabelComponent->SetGenerateOverlapEvents(false);
		LabelComponent->SetCastShadow(false);
		LabelComponent->bCastDynamicShadow = false;
		LabelComponent->bCastStaticShadow = false;
		LabelComponent->CastShadow = false;

		AddInstanceComponent(LabelComponent);
		CityWorldLabelComponents.Add(CityId, LabelComponent);
	}
	else
	{
		bShouldRemainVisible = LabelComponent->IsVisible();
	}

	LabelComponent->SetRelativeTransform(BuildCityWorldLabelTransform(Coord));
	LabelComponent->InitWidget();
	LabelComponent->SetVisibility(bShouldRemainVisible);
	UpdateCityWorldLabel(
		CityId,
		CityName,
		Population,
		CurrentHealth,
		MaxHealth,
		Strength,
		GrowthPercent,
		CivilisationThemeMaterial,
		CivilisationThemeColor
	);
}

void AModularHexGridActor::UpdateCityWorldLabel(
	int32 CityId,
	FName CityName,
	int32 Population,
	int32 CurrentHealth,
	int32 MaxHealth,
	int32 Strength,
	float GrowthPercent,
	UMaterialInterface* CivilisationThemeMaterial,
	FLinearColor CivilisationThemeColor
)
{
	TObjectPtr<UWidgetComponent>* ExistingComponent = CityWorldLabelComponents.Find(CityId);
	UWidgetComponent* LabelComponent = ExistingComponent ? ExistingComponent->Get() : nullptr;
	if (!LabelComponent)
	{
		return;
	}

	if (UConquestCityWorldLabelWidget* LabelWidget = Cast<UConquestCityWorldLabelWidget>(LabelComponent->GetWidget()))
	{
		LabelWidget->SetCityLabel(
			CityName,
			Population,
			CurrentHealth,
			MaxHealth,
			Strength,
			GrowthPercent,
			CivilisationThemeMaterial,
			CivilisationThemeColor
		);
	}
}

void AModularHexGridActor::SetCityWorldLabelVisible(int32 CityId, bool bVisible)
{
	TObjectPtr<UWidgetComponent>* ExistingComponent = CityWorldLabelComponents.Find(CityId);
	UWidgetComponent* LabelComponent = ExistingComponent ? ExistingComponent->Get() : nullptr;
	if (!LabelComponent)
	{
		return;
	}

	LabelComponent->SetVisibility(bVisible);
}

void AModularHexGridActor::AddOrUpdateTileHealthBar(
	const FIntPoint& Coord,
	int32 CurrentHealth,
	int32 MaxHealth,
	int32 CombatStrength
)
{
	if (!TileHealthBarWidgetClass || !GridModel.IsValidTile(Coord.X, Coord.Y))
	{
		return;
	}

	const int32 ClampedMaxHealth = FMath::Max(1, MaxHealth);
	const int32 ClampedCurrentHealth = FMath::Clamp(CurrentHealth, 0, ClampedMaxHealth);
	if (ClampedCurrentHealth >= ClampedMaxHealth)
	{
		RemoveTileHealthBar(Coord);
		return;
	}

	UWidgetComponent* HealthBarComponent = nullptr;
	if (TObjectPtr<UWidgetComponent>* ExistingComponent = TileHealthBarComponents.Find(Coord))
	{
		HealthBarComponent = ExistingComponent->Get();
	}

	if (!HealthBarComponent)
	{
		const FName ComponentName = MakeUniqueObjectName(
			this,
			UWidgetComponent::StaticClass(),
			TEXT("TileHealthBar")
		);

		HealthBarComponent = NewObject<UWidgetComponent>(this, ComponentName);
		if (!HealthBarComponent)
		{
			return;
		}

		HealthBarComponent->SetupAttachment(SceneRoot);
		HealthBarComponent->RegisterComponent();
		HealthBarComponent->SetWidgetClass(TileHealthBarWidgetClass);
		HealthBarComponent->SetWidgetSpace(EWidgetSpace::World);
		HealthBarComponent->SetDrawSize(TileHealthBarDrawSize);
		HealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HealthBarComponent->SetGenerateOverlapEvents(false);
		HealthBarComponent->SetCastShadow(false);
		HealthBarComponent->bCastDynamicShadow = false;
		HealthBarComponent->bCastStaticShadow = false;
		HealthBarComponent->CastShadow = false;

		AddInstanceComponent(HealthBarComponent);
		TileHealthBarComponents.Add(Coord, HealthBarComponent);
	}

	HealthBarComponent->SetRelativeTransform(BuildTileHealthBarTransform(Coord));
	HealthBarComponent->InitWidget();
	HealthBarComponent->SetVisibility(true);

	if (UConquestTileHealthBarWidget* HealthBarWidget = Cast<UConquestTileHealthBarWidget>(HealthBarComponent->GetWidget()))
	{
		HealthBarWidget->SetTileHealth(ClampedCurrentHealth, ClampedMaxHealth, CombatStrength);
	}
}

void AModularHexGridActor::RemoveTileHealthBar(const FIntPoint& Coord)
{
	TObjectPtr<UWidgetComponent>* ExistingComponent = TileHealthBarComponents.Find(Coord);
	UWidgetComponent* HealthBarComponent = ExistingComponent ? ExistingComponent->Get() : nullptr;
	if (HealthBarComponent)
	{
		HealthBarComponent->DestroyComponent();
	}

	TileHealthBarComponents.Remove(Coord);
}

void AModularHexGridActor::ClearTileHealthBars()
{
	for (const TPair<FIntPoint, TObjectPtr<UWidgetComponent>>& Pair : TileHealthBarComponents)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}

	TileHealthBarComponents.Reset();
}

void AModularHexGridActor::ClearCityPlaceholders()
{
	CityIdToPlaceholderInstanceIndex.Reset();
	CityIdToPlaceholderVisualKey.Reset();

	for (const TPair<int32, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : CityPlaceholderMeshComponentsByVisualKey)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearInstances();
			Pair.Value->MarkRenderStateDirty();
		}
	}

	for (const TPair<int32, TObjectPtr<UWidgetComponent>>& Pair : CityWorldLabelComponents)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}

	CityWorldLabelComponents.Reset();
	ClearTileHealthBars();
}

void AModularHexGridActor::RebuildCivilisationBorders(int32 OwnerPlayerId, UMaterialInterface* BorderMaterial, UMaterialInterface* BorderFillMaterial)
{
	ClearCivilisationBorders();

	TArray<FIntPoint> OwnedTiles;

	if (OwnerPlayerId != INDEX_NONE)
	{
		for (int32 R = 0; R < GridModel.GetGridHeight(); ++R)
		{
			for (int32 Q = 0; Q < GridModel.GetGridWidth(); ++Q)
			{
				const FHexTileData* Tile = GridModel.GetTile(Q, R);
				if (Tile && Tile->Gameplay.OwnerPlayerId == OwnerPlayerId)
				{
					OwnedTiles.Add(FIntPoint(Q, R));
				}
			}
		}
	}

	RebuildCivilisationBordersForTiles(OwnedTiles, BorderMaterial, BorderFillMaterial);
}

void AModularHexGridActor::ClearCivilisationBorders()
{
	if (CivilisationBorderMesh)
	{
		CivilisationBorderMesh->ClearAllMeshSections();
		CivilisationBorderMesh->SetVisibility(false);
	}

	if (CivilisationBorderFillMesh)
	{
		CivilisationBorderFillMesh->ClearAllMeshSections();
		CivilisationBorderFillMesh->SetVisibility(false);
	}
}

void AModularHexGridActor::RebuildCivilisationBordersForTiles(const TArray<FIntPoint>& OwnedTiles, UMaterialInterface* BorderMaterial, UMaterialInterface* BorderFillMaterial)
{
	if (!CivilisationBorderMesh)
	{
		return;
	}

	if (OwnedTiles.Num() <= 0)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	TArray<FVector> FillVertices;
	TArray<int32> FillTriangles;
	TArray<FVector> FillNormals;
	TArray<FVector2D> FillUVs;
	TArray<FColor> FillVertexColors;
	TArray<FProcMeshTangent> FillTangents;

	struct FBorderEdgeRecord
	{
		FIntPoint TileCoord = FIntPoint::ZeroValue;
		FVector TileCenter = FVector::ZeroVector;
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		FVector FlatStart = FVector::ZeroVector;
		FVector FlatEnd = FVector::ZeroVector;
	};

	struct FBorderEdgeBucket
	{
		int32 Count = 0;
		FBorderEdgeRecord FirstEdge;
	};

	TMap<FHexEdgeKey, FBorderEdgeBucket> EdgeBuckets;
	TMap<FHexVertexKey, FVector> BoundaryVertexCaps;
	TArray<FBorderEdgeRecord> BoundaryEdges;
	TSet<FIntPoint> UniqueOwnedTiles;

	for (const FIntPoint& Coord : OwnedTiles)
	{
		if (GridModel.IsValidTile(Coord.X, Coord.Y))
		{
			UniqueOwnedTiles.Add(Coord);
		}
	}

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

		// Border edges can be emitted in either winding direction around a hex.
		// Add the reverse faces as well so one-sided materials never drop an edge.
		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 1);
		Triangles.Add(StartIndex + 2);

		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex + 3);

		for (int32 i = 0; i < 4; ++i)
		{
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D::ZeroVector);
			VertexColors.Add(FColor::White);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}
	};

	auto AddFillHex = [&](
		const FVector& Center,
		const TArray<FVector>& Corners
	)
	{
		if (Corners.Num() != 6)
		{
			return;
		}

		const int32 StartIndex = FillVertices.Num();

		FillVertices.Add(Center);
		FillNormals.Add(FVector::UpVector);
		FillUVs.Add(FVector2D(0.5f, 0.5f));
		FillVertexColors.Add(FColor::White);
		FillTangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const FVector& Corner = Corners[CornerIndex];
			FillVertices.Add(Corner);
			FillNormals.Add(FVector::UpVector);
			FillUVs.Add(GridModel.GetHexCornerUV(CornerIndex));
			FillVertexColors.Add(FColor::White);
			FillTangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}

		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const int32 CurrentIndex = StartIndex + 1 + CornerIndex;
			const int32 NextIndex = StartIndex + 1 + ((CornerIndex + 1) % 6);

			FillTriangles.Add(StartIndex);
			FillTriangles.Add(NextIndex);
			FillTriangles.Add(CurrentIndex);

			FillTriangles.Add(StartIndex);
			FillTriangles.Add(CurrentIndex);
			FillTriangles.Add(NextIndex);
		}
	};

	auto AddBoundaryVertexCap = [&BoundaryVertexCaps, this](const FVector& FlatVertex, const FVector& RaisedVertex)
	{
		const FHexVertexKey VertexKey = GridModel.MakeVertexKey(FlatVertex);
		if (FVector* ExistingCap = BoundaryVertexCaps.Find(VertexKey))
		{
			ExistingCap->Z = FMath::Max(ExistingCap->Z, RaisedVertex.Z);
			return;
		}

		BoundaryVertexCaps.Add(VertexKey, RaisedVertex);
	};

	for (const FIntPoint& Coord : UniqueOwnedTiles)
	{
		const FVector Center = GridModel.GetHexCenter(Coord.X, Coord.Y);
		const FHexTileData* Tile = GridModel.GetTile(Coord);
		if (!Tile)
		{
			continue;
		}

		const float FillScale = FMath::Clamp(CivilisationBorderFillHexScale, 0.01f, 1.0f);

		TArray<FVector> Corners;
		TArray<FVector> FillCorners;
		TArray<FVector> FlatCorners;
		Corners.SetNum(6);
		FillCorners.SetNum(6);
		FlatCorners.SetNum(6);

		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const FVector FlatCorner = Center + GridModel.GetHexCornerOffset(CornerIndex);
			FlatCorners[CornerIndex] = FlatCorner;

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

			const FVector FillFlatCorner = Center + (GridModel.GetHexCornerOffset(CornerIndex) * FillScale);
			const float FillCornerZ = FMath::Lerp(
				Tile->Height,
				CornerZ,
				FillScale
			);

			FillCorners[CornerIndex] = FVector(
				FillFlatCorner.X,
				FillFlatCorner.Y,
				FillCornerZ + CivilisationBorderFillSurfaceOffset
			);
		}

		AddFillHex(
			FVector(Center.X, Center.Y, Tile->Height + CivilisationBorderFillSurfaceOffset),
			FillCorners
		);

		for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
		{
			const int32 NextCornerIndex = (EdgeIndex + 1) % 6;
			const FHexVertexKey StartKey = GridModel.MakeVertexKey(FlatCorners[EdgeIndex]);
			const FHexVertexKey EndKey = GridModel.MakeVertexKey(FlatCorners[NextCornerIndex]);
			const FHexEdgeKey EdgeKey = FHexEdgeKey::Make(StartKey, EndKey);

			FBorderEdgeBucket& Bucket = EdgeBuckets.FindOrAdd(EdgeKey);
			++Bucket.Count;

			if (Bucket.Count == 1)
			{
				Bucket.FirstEdge.TileCoord = Coord;
				Bucket.FirstEdge.TileCenter = Center;
				Bucket.FirstEdge.Start = Corners[EdgeIndex];
				Bucket.FirstEdge.End = Corners[NextCornerIndex];
				Bucket.FirstEdge.FlatStart = FlatCorners[EdgeIndex];
				Bucket.FirstEdge.FlatEnd = FlatCorners[NextCornerIndex];
			}
		}
	}

	for (const TPair<FHexEdgeKey, FBorderEdgeBucket>& Pair : EdgeBuckets)
	{
		const FBorderEdgeBucket& Bucket = Pair.Value;

		if (Bucket.Count == 1)
		{
			BoundaryEdges.Add(Bucket.FirstEdge);
			AddBoundaryVertexCap(Bucket.FirstEdge.FlatStart, Bucket.FirstEdge.Start);
			AddBoundaryVertexCap(Bucket.FirstEdge.FlatEnd, Bucket.FirstEdge.End);
			continue;
		}

		if (Bucket.Count > 2)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Civilisation border edge bucket has %d instances. This suggests duplicate owned coords or unsuitable vertex key precision."),
				Bucket.Count
			);
		}
	}

	BoundaryEdges.Sort([](const FBorderEdgeRecord& Left, const FBorderEdgeRecord& Right)
	{
		if (Left.TileCoord.Y != Right.TileCoord.Y)
		{
			return Left.TileCoord.Y < Right.TileCoord.Y;
		}

		if (Left.TileCoord.X != Right.TileCoord.X)
		{
			return Left.TileCoord.X < Right.TileCoord.X;
		}

		if (!FMath::IsNearlyEqual(Left.FlatStart.X, Right.FlatStart.X))
		{
			return Left.FlatStart.X < Right.FlatStart.X;
		}

		return Left.FlatStart.Y < Right.FlatStart.Y;
	});

	for (const FBorderEdgeRecord& BorderEdge : BoundaryEdges)
	{
		FVector ToCenter = BorderEdge.TileCenter - ((BorderEdge.Start + BorderEdge.End) * 0.5f);
		ToCenter.Z = 0.0f;
		ToCenter.Normalize();

		if (ToCenter.IsNearlyZero())
		{
			continue;
		}

		const FVector HalfWidth = ToCenter * (CivilisationBorderEdgeWidth * 0.5f);

		AddQuad(
			BorderEdge.Start - HalfWidth,
			BorderEdge.End - HalfWidth,
			BorderEdge.End + HalfWidth,
			BorderEdge.Start + HalfWidth
		);
	}

	for (const TPair<FHexVertexKey, FVector>& BoundaryVertexCap : BoundaryVertexCaps)
	{
		const FVector CapCenter = BoundaryVertexCap.Value;
		const int32 SegmentCount = 8;
		const int32 CenterIndex = Vertices.Num();

		Vertices.Add(CapCenter);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(0.5f, 0.5f));
		VertexColors.Add(FColor::White);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float Angle = (2.0f * PI * static_cast<float>(SegmentIndex)) / static_cast<float>(SegmentCount);
			Vertices.Add(FVector(
				CapCenter.X + FMath::Cos(Angle) * CivilisationBorderVertexRadius,
				CapCenter.Y + FMath::Sin(Angle) * CivilisationBorderVertexRadius,
				CapCenter.Z
			));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D::ZeroVector);
			VertexColors.Add(FColor::White);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const int32 CurrentIndex = CenterIndex + 1 + SegmentIndex;
			const int32 NextIndex = CenterIndex + 1 + ((SegmentIndex + 1) % SegmentCount);

			Triangles.Add(CenterIndex);
			Triangles.Add(NextIndex);
			Triangles.Add(CurrentIndex);

			Triangles.Add(CenterIndex);
			Triangles.Add(CurrentIndex);
			Triangles.Add(NextIndex);
		}
	}

	if (CivilisationBorderFillMesh)
	{
		if (FillVertices.Num() > 0)
		{
			const int32 FillSectionIndex = CivilisationBorderFillMesh->GetNumSections();
			CivilisationBorderFillMesh->CreateMeshSection(
				FillSectionIndex,
				FillVertices,
				FillTriangles,
				FillNormals,
				FillUVs,
				FillVertexColors,
				FillTangents,
				false
			);

			UMaterialInterface* EffectiveFillMaterial = BorderFillMaterial ? BorderFillMaterial : DefaultCivilisationBorderFillMaterial.Get();
			if (EffectiveFillMaterial)
			{
				CivilisationBorderFillMesh->SetMaterial(FillSectionIndex, EffectiveFillMaterial);
			}

			CivilisationBorderFillMesh->SetVisibility(true);
		}
	}

	if (Vertices.Num() <= 0)
	{
		return;
	}

	const int32 BorderSectionIndex = CivilisationBorderMesh->GetNumSections();
	CivilisationBorderMesh->CreateMeshSection(
		BorderSectionIndex,
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
		CivilisationBorderMesh->SetMaterial(BorderSectionIndex, EffectiveMaterial);
	}

	CivilisationBorderMesh->SetVisibility(true);
}

void AModularHexGridActor::RebuildExpansionCandidateHighlights(const TArray<FIntPoint>& CandidateCoords, UMaterialInterface* HighlightMaterial)
{
	if (!ExpansionCandidateMesh)
	{
		return;
	}

	ExpansionCandidateMesh->ClearAllMeshSections();

	if (CandidateCoords.Num() <= 0)
	{
		ExpansionCandidateMesh->SetVisibility(false);
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

	for (const FIntPoint& Coord : CandidateCoords)
	{
		if (!GridModel.IsValidTile(Coord.X, Coord.Y))
		{
			continue;
		}

		const FVector Center = GridModel.GetHexCenter(Coord.X, Coord.Y);

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
				CornerZ + ExpansionCandidateSurfaceOffset
			);
		}

		for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
		{
			const FVector A = Corners[EdgeIndex];
			const FVector B = Corners[(EdgeIndex + 1) % 6];

			FVector ToCenter = Center - ((A + B) * 0.5f);
			ToCenter.Z = 0.0f;
			ToCenter.Normalize();

			const FVector HalfWidth = ToCenter * ExpansionCandidateEdgeWidth * 0.5f;

			AddQuad(
				A - HalfWidth,
				B - HalfWidth,
				B + HalfWidth,
				A + HalfWidth
			);
		}
	}

	if (Vertices.Num() <= 0)
	{
		ExpansionCandidateMesh->SetVisibility(false);
		return;
	}

	ExpansionCandidateMesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false
	);

	UMaterialInterface* EffectiveMaterial = HighlightMaterial ? HighlightMaterial : ExpansionCandidateMaterial.Get();
	if (EffectiveMaterial)
	{
		ExpansionCandidateMesh->SetMaterial(0, EffectiveMaterial);
	}

	ExpansionCandidateMesh->SetVisibility(true);
}

void AModularHexGridActor::ClearExpansionCandidateHighlights()
{
	if (ExpansionCandidateMesh)
	{
		ExpansionCandidateMesh->ClearAllMeshSections();
		ExpansionCandidateMesh->SetVisibility(false);
	}
}

int32 AModularHexGridActor::GetTileYieldValue(const FHexYield& Yield, EConquestYieldType YieldType) const
{
	switch (YieldType)
	{
	case EConquestYieldType::Food:
		return Yield.Food;
	case EConquestYieldType::Production:
		return Yield.Production;
	case EConquestYieldType::Gold:
		return Yield.Gold;
	case EConquestYieldType::Science:
		return Yield.Science;
	case EConquestYieldType::Culture:
		return Yield.Culture;
	default:
		return 0;
	}
}

EConquestYieldType AModularHexGridActor::GetTileYieldTypeForLayer(int32 LayerIndex) const
{
	switch (LayerIndex)
	{
	case 0:
		return EConquestYieldType::Food;
	case 1:
		return EConquestYieldType::Production;
	case 2:
		return EConquestYieldType::Science;
	case 3:
		return EConquestYieldType::Culture;
	case 4:
		return EConquestYieldType::Gold;
	default:
		return EConquestYieldType::Food;
	}
}

int32 AModularHexGridActor::GetTileYieldLayerIndex(EConquestYieldType YieldType) const
{
	switch (YieldType)
	{
	case EConquestYieldType::Food:
		return 0;
	case EConquestYieldType::Production:
		return 1;
	case EConquestYieldType::Science:
		return 2;
	case EConquestYieldType::Culture:
		return 3;
	case EConquestYieldType::Gold:
		return 4;
	default:
		return INDEX_NONE;
	}
}

float AModularHexGridActor::GetTileYieldOverlayZ(const FHexTileData& Tile) const
{
	return bUseFixedTileYieldHeight
		? FixedTileYieldHeight
		: Tile.Height + TileYieldSurfaceOffset;
}

FLinearColor AModularHexGridActor::GetTileYieldColor(EConquestYieldType YieldType) const
{
	switch (YieldType)
	{
	case EConquestYieldType::Food:
		return FoodYieldColor;
	case EConquestYieldType::Production:
		return ProductionYieldColor;
	case EConquestYieldType::Gold:
		return GoldYieldColor;
	case EConquestYieldType::Science:
		return ScienceYieldColor;
	case EConquestYieldType::Culture:
		return CultureYieldColor;
	default:
		return FLinearColor::White;
	}
}

void AModularHexGridActor::ClearTileYieldOverlay()
{
	for (TObjectPtr<UProceduralMeshComponent>& YieldBorderMesh : TileYieldBorderMeshes)
	{
		if (YieldBorderMesh)
		{
			YieldBorderMesh->ClearAllMeshSections();
			YieldBorderMesh->SetVisibility(false);
		}
	}

	for (TObjectPtr<UProceduralMeshComponent>& YieldFillMesh : TileYieldFillMeshes)
	{
		if (YieldFillMesh)
		{
			YieldFillMesh->ClearAllMeshSections();
			YieldFillMesh->SetVisibility(false);
		}
	}

	for (TObjectPtr<UProceduralMeshComponent>& YieldTextMesh : TileYieldTextMeshes)
	{
		if (YieldTextMesh)
		{
			YieldTextMesh->ClearAllMeshSections();
			YieldTextMesh->SetVisibility(false);
		}
	}

	TileYieldSectionIndicesByLayer.Reset();
	TileYieldBorderMaterialInstances.Reset();
	TileYieldFillMaterialInstances.Reset();
	TileYieldTextMaterialInstances.Reset();
}

void AModularHexGridActor::SetTileYieldLens(EConquestYieldType YieldType)
{
	if (bIsTileYieldLensTransitioning)
	{
		return;
	}

	if (
		YieldType != EConquestYieldType::Food &&
		YieldType != EConquestYieldType::Production &&
		YieldType != EConquestYieldType::Gold &&
		YieldType != EConquestYieldType::Science &&
		YieldType != EConquestYieldType::Culture
	)
	{
		ClearTileYieldLens();
		return;
	}

	if (
		bShowTileYieldOverlay &&
		bHasActiveTileYieldLens &&
		ActiveTileYieldLens == YieldType
	)
	{
		return;
	}

	bIsTileYieldLensTransitioning = true;
	ActiveTileYieldLens = YieldType;
	LastSelectedTileYieldLens = YieldType;
	bHasActiveTileYieldLens = true;
	bShowTileYieldOverlay = true;
	UpdateTileYieldOverlayVisibility();
	bIsTileYieldLensTransitioning = false;
	OnTileYieldLensTransitionFinished.Broadcast(ActiveTileYieldLens);
}

void AModularHexGridActor::ClearTileYieldLens()
{
	if (bIsTileYieldLensTransitioning)
	{
		return;
	}

	bIsTileYieldLensTransitioning = true;
	bHasActiveTileYieldLens = false;
	bShowTileYieldOverlay = false;
	UpdateTileYieldOverlayVisibility();
	bIsTileYieldLensTransitioning = false;
	OnTileYieldLensTransitionFinished.Broadcast(LastSelectedTileYieldLens);
}

void AModularHexGridActor::SetTileYieldOverlayVisible(bool bVisible)
{
	bShowTileYieldOverlay = bVisible;

	if (!bShowTileYieldOverlay || !bHasActiveTileYieldLens)
	{
		UpdateTileYieldOverlayVisibility();
		return;
	}

	UpdateTileYieldOverlayVisibility();
}

void AModularHexGridActor::ToggleTileYieldOverlay()
{
	if (bIsTileYieldLensTransitioning)
	{
		return;
	}

	if (bShowTileYieldOverlay && bHasActiveTileYieldLens)
	{
		ClearTileYieldLens();
	}
	else
	{
		SetTileYieldLens(LastSelectedTileYieldLens);
	}
}

void AModularHexGridActor::ToggleSpecificTileYieldLens(EConquestYieldType YieldType)
{
	if (bIsTileYieldLensTransitioning)
	{
		return;
	}

	if (
		bShowTileYieldOverlay &&
		bHasActiveTileYieldLens &&
		ActiveTileYieldLens == YieldType
	)
	{
		return;
	}

	SetTileYieldLens(YieldType);
}

void AModularHexGridActor::RebuildTileYieldOverlay()
{
	BuildAllTileYieldOverlays();
}

bool AModularHexGridActor::IsTileDiscoveredForYieldOverlay(const FIntPoint& Coord) const
{
	return !bGenerateFogOfWar || LocallyRevealedFogTiles.Contains(Coord);
}

bool AModularHexGridActor::IsTileInYieldHoverRange(const FIntPoint& Coord) const
{
	if (HoveredQ == INDEX_NONE || HoveredR == INDEX_NONE)
	{
		return false;
	}

	return GridModel.GetHexDistance(FIntPoint(HoveredQ, HoveredR), Coord) <= TileYieldHoverRevealRadius;
}

void AModularHexGridActor::BuildAllTileYieldOverlays()
{
	ClearTileYieldOverlay();

	TArray<FVector> BorderVertices;
	TArray<int32> BorderTriangles;
	TArray<FVector> BorderNormals;
	TArray<FVector2D> BorderUVs;
	TArray<FColor> BorderVertexColors;
	TArray<FProcMeshTangent> BorderTangents;

	TArray<FVector> FillVertices;
	TArray<int32> FillTriangles;
	TArray<FVector> FillNormals;
	TArray<FVector2D> FillUVs;
	TArray<FColor> FillVertexColors;
	TArray<FProcMeshTangent> FillTangents;

	auto AddHex = [](
		const FVector& Center,
		float Radius,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		const int32 CenterIndex = Vertices.Num();
		Vertices.Add(Center);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(0.5f, 0.5f));
		VertexColors.Add(Color);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

		int32 CornerIndices[6];
		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const float AngleRadians = FMath::DegreesToRadians(60.0f * static_cast<float>(CornerIndex));
			const FVector Corner(
				Center.X + FMath::Cos(AngleRadians) * Radius,
				Center.Y + FMath::Sin(AngleRadians) * Radius,
				Center.Z
			);

			CornerIndices[CornerIndex] = Vertices.Num();
			Vertices.Add(Corner);
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(
				0.5f + FMath::Cos(AngleRadians) * 0.5f,
				0.5f + FMath::Sin(AngleRadians) * 0.5f
			));
			VertexColors.Add(Color);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}

		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const int32 NextCornerIndex = (CornerIndex + 1) % 6;
			Triangles.Add(CenterIndex);
			Triangles.Add(CornerIndices[NextCornerIndex]);
			Triangles.Add(CornerIndices[CornerIndex]);
		}
	};

	auto AddRect = [](
		const FVector& Center,
		float HalfWidth,
		float HalfHeight,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		const int32 StartIndex = Vertices.Num();
		Vertices.Add(FVector(Center.X - HalfWidth, Center.Y - HalfHeight, Center.Z));
		Vertices.Add(FVector(Center.X + HalfWidth, Center.Y - HalfHeight, Center.Z));
		Vertices.Add(FVector(Center.X + HalfWidth, Center.Y + HalfHeight, Center.Z));
		Vertices.Add(FVector(Center.X - HalfWidth, Center.Y + HalfHeight, Center.Z));

		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex + 1);
		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 3);
		Triangles.Add(StartIndex + 2);

		for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
		{
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D::ZeroVector);
			VertexColors.Add(Color);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}
	};

	auto AddDigit = [&AddRect](
		int32 Digit,
		const FVector& Center,
		float Height,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		static const bool SegmentMasks[10][7] =
		{
			{ true, true, true, true, true, true, false },
			{ false, true, true, false, false, false, false },
			{ true, true, false, true, true, false, true },
			{ true, true, true, true, false, false, true },
			{ false, true, true, false, false, true, true },
			{ true, false, true, true, false, true, true },
			{ true, false, true, true, true, true, true },
			{ true, true, true, false, false, false, false },
			{ true, true, true, true, true, true, true },
			{ true, true, true, true, false, true, true }
		};

		const float Width = Height * 0.55f;
		const float Thickness = Height * 0.13f;
		const float HalfHeight = Height * 0.5f;
		const float HalfWidth = Width * 0.5f;
		const float VerticalSegmentHalfHeight = (Height - Thickness * 3.0f) * 0.25f;
		const float HorizontalHalfWidth = HalfWidth - Thickness * 0.5f;
		const float VisualRightX = -HalfWidth + Thickness * 0.5f;
		const float VisualLeftX = -VisualRightX;
		const float UpperY = HalfHeight * 0.5f;
		const float LowerY = -HalfHeight * 0.5f;

		const auto& Mask = SegmentMasks[FMath::Clamp(Digit, 0, 9)];
		if (Mask[0]) AddRect(Center + FVector(0.0f, HalfHeight - Thickness * 0.5f, 0.0f), HorizontalHalfWidth, Thickness * 0.5f, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[1]) AddRect(Center + FVector(VisualRightX, UpperY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[2]) AddRect(Center + FVector(VisualRightX, LowerY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[3]) AddRect(Center + FVector(0.0f, -HalfHeight + Thickness * 0.5f, 0.0f), HorizontalHalfWidth, Thickness * 0.5f, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[4]) AddRect(Center + FVector(VisualLeftX, LowerY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[5]) AddRect(Center + FVector(VisualLeftX, UpperY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[6]) AddRect(Center, HorizontalHalfWidth, Thickness * 0.5f, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
	};

	auto AddNumber = [&AddDigit](
		int32 Value,
		const FVector& Center,
		float Height,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		const FString Digits = FString::FromInt(FMath::Max(0, Value));
		const float DigitWidth = Height * 0.65f;
		const float TotalWidth = DigitWidth * static_cast<float>(Digits.Len());
		for (int32 DigitIndex = 0; DigitIndex < Digits.Len(); ++DigitIndex)
		{
			const int32 Digit = Digits[DigitIndex] - TCHAR('0');
			const float OffsetX = -TotalWidth * 0.5f + DigitWidth * 0.5f + DigitWidth * static_cast<float>(DigitIndex);
			AddDigit(Digit, Center + FVector(OffsetX, 0.0f, 0.0f), Height, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		}
	};

	const float IconRadius = FMath::Max(1.0f, GridModel.GetHexRadius() * TileYieldHexScale);
	const float FillRadius = FMath::Max(1.0f, IconRadius - FMath::Max(0.0f, TileYieldBorderWidth));
	const FLinearColor BorderLinearColor = FLinearColor::Black;
	const FColor BorderColor = BorderLinearColor.ToFColor(true);

	TileYieldBorderMaterialInstances.SetNum(TileYieldBorderMeshes.Num());
	TileYieldFillMaterialInstances.SetNum(TileYieldFillMeshes.Num());

	for (int32 LayerIndex = 0; LayerIndex < 5; ++LayerIndex)
	{
		if (
			!TileYieldBorderMeshes.IsValidIndex(LayerIndex) ||
			!TileYieldFillMeshes.IsValidIndex(LayerIndex) ||
			!TileYieldTextMeshes.IsValidIndex(LayerIndex)
		)
		{
			continue;
		}

		UProceduralMeshComponent* YieldBorderMesh = TileYieldBorderMeshes[LayerIndex].Get();
		UProceduralMeshComponent* YieldFillMesh = TileYieldFillMeshes[LayerIndex].Get();
		UProceduralMeshComponent* YieldTextMesh = TileYieldTextMeshes[LayerIndex].Get();
		if (!YieldBorderMesh || !YieldFillMesh || !YieldTextMesh)
		{
			continue;
		}

		BorderVertices.Reset();
		BorderTriangles.Reset();
		BorderNormals.Reset();
		BorderUVs.Reset();
		BorderVertexColors.Reset();
		BorderTangents.Reset();
		FillVertices.Reset();
		FillTriangles.Reset();
		FillNormals.Reset();
		FillUVs.Reset();
		FillVertexColors.Reset();
		FillTangents.Reset();
		TArray<FVector> TextVertices;
		TArray<int32> TextTriangles;
		TArray<FVector> TextNormals;
		TArray<FVector2D> TextUVs;
		TArray<FColor> TextVertexColors;
		TArray<FProcMeshTangent> TextTangents;

		const EConquestYieldType YieldType = GetTileYieldTypeForLayer(LayerIndex);
		const FLinearColor FillLinearColor = GetTileYieldColor(YieldType);
		const FColor FillColor = FillLinearColor.ToFColor(true);
		TMap<FIntPoint, int32>& SectionIndices = TileYieldSectionIndicesByLayer.FindOrAdd(LayerIndex);

		UMaterialInterface* BorderSourceMaterial = bUseTileYieldFillMaterialForBorder && TileYieldFillMaterial
			? TileYieldFillMaterial.Get()
			: TileYieldBorderMaterial.Get();
		UMaterialInterface* EffectiveBorderMaterial = BorderSourceMaterial;
		if (BorderSourceMaterial)
		{
			UMaterialInstanceDynamic* BorderMaterialInstance = UMaterialInstanceDynamic::Create(BorderSourceMaterial, this);
			TileYieldBorderMaterialInstances[LayerIndex] = BorderMaterialInstance;
			if (BorderMaterialInstance)
			{
				BorderMaterialInstance->SetVectorParameterValue(
					TileYieldMaterialColorParameterName,
					BorderLinearColor
				);
				EffectiveBorderMaterial = BorderMaterialInstance;
			}
		}

		UMaterialInterface* EffectiveFillMaterial = TileYieldFillMaterial.Get();
		if (TileYieldFillMaterial)
		{
			UMaterialInstanceDynamic* FillMaterialInstance = UMaterialInstanceDynamic::Create(TileYieldFillMaterial, this);
			TileYieldFillMaterialInstances[LayerIndex] = FillMaterialInstance;
			if (FillMaterialInstance)
			{
				FillMaterialInstance->SetVectorParameterValue(
					TileYieldMaterialColorParameterName,
					FillLinearColor
				);
				EffectiveFillMaterial = FillMaterialInstance;
			}
		}

		UMaterialInterface* TextSourceMaterial = TileYieldTextMaterial
			? TileYieldTextMaterial.Get()
			: TileYieldFillMaterial.Get();
		UMaterialInterface* EffectiveTextMaterial = TextSourceMaterial;
		if (TextSourceMaterial)
		{
			if (TileYieldTextMaterialInstances.Num() < TileYieldTextMeshes.Num())
			{
				TileYieldTextMaterialInstances.SetNum(TileYieldTextMeshes.Num());
			}

			UMaterialInstanceDynamic* TextMaterialInstance = UMaterialInstanceDynamic::Create(TextSourceMaterial, this);
			TileYieldTextMaterialInstances[LayerIndex] = TextMaterialInstance;
			if (TextMaterialInstance)
			{
				TextMaterialInstance->SetVectorParameterValue(
					TileYieldMaterialColorParameterName,
					TileYieldTextColor
				);
				EffectiveTextMaterial = TextMaterialInstance;
			}
		}

		for (int32 R = 0; R < GridModel.GetGridHeight(); ++R)
		{
			for (int32 Q = 0; Q < GridModel.GetGridWidth(); ++Q)
			{
				const FIntPoint Coord(Q, R);
				const FHexTileData* Tile = GridModel.GetTile(Q, R);
				if (!Tile)
				{
					continue;
				}

				const int32 YieldValue = GetTileYieldValue(Tile->FinalYield, YieldType);
				if (YieldValue <= 0)
				{
					continue;
				}

				const int32 SectionIndex = SectionIndices.Num();
				SectionIndices.Add(Coord, SectionIndex);

				const FVector FlatCenter = GridModel.GetHexCenter(Q, R);
				const FVector BorderCenter(FlatCenter.X, FlatCenter.Y, GetTileYieldOverlayZ(*Tile));
				const FVector FillCenter = BorderCenter + FVector(0.0f, 0.0f, 0.5f);

				AddHex(
					BorderCenter,
					IconRadius,
					BorderColor,
					BorderVertices,
					BorderTriangles,
					BorderNormals,
					BorderUVs,
					BorderVertexColors,
					BorderTangents
				);

				AddHex(
					FillCenter,
					FillRadius,
					FillColor,
					FillVertices,
					FillTriangles,
					FillNormals,
					FillUVs,
					FillVertexColors,
					FillTangents
				);

				YieldBorderMesh->CreateMeshSection(
					SectionIndex,
					BorderVertices,
					BorderTriangles,
					BorderNormals,
					BorderUVs,
					BorderVertexColors,
					BorderTangents,
					false
				);

				YieldFillMesh->CreateMeshSection(
					SectionIndex,
					FillVertices,
					FillTriangles,
					FillNormals,
					FillUVs,
					FillVertexColors,
					FillTangents,
					false
				);

				YieldBorderMesh->SetMeshSectionVisible(SectionIndex, false);
				YieldFillMesh->SetMeshSectionVisible(SectionIndex, false);
				if (EffectiveBorderMaterial)
				{
					YieldBorderMesh->SetMaterial(SectionIndex, EffectiveBorderMaterial);
				}
				if (EffectiveFillMaterial)
				{
					YieldFillMesh->SetMaterial(SectionIndex, EffectiveFillMaterial);
				}

				BorderVertices.Reset();
				BorderTriangles.Reset();
				BorderNormals.Reset();
				BorderUVs.Reset();
				BorderVertexColors.Reset();
				BorderTangents.Reset();
				FillVertices.Reset();
				FillTriangles.Reset();
				FillNormals.Reset();
				FillUVs.Reset();
				FillVertexColors.Reset();
				FillTangents.Reset();

				if (bShowTileYieldText)
				{
					AddNumber(
						YieldValue,
						FillCenter + FVector(0.0f, 0.0f, TileYieldTextHeightOffset),
						TileYieldTextWorldSize,
						TileYieldTextColor.ToFColor(true),
						TextVertices,
						TextTriangles,
						TextNormals,
						TextUVs,
						TextVertexColors,
						TextTangents
					);

					YieldTextMesh->CreateMeshSection(
						SectionIndex,
						TextVertices,
						TextTriangles,
						TextNormals,
						TextUVs,
						TextVertexColors,
						TextTangents,
						false
					);
					YieldTextMesh->SetMeshSectionVisible(SectionIndex, false);
					if (EffectiveTextMaterial)
					{
						YieldTextMesh->SetMaterial(SectionIndex, EffectiveTextMaterial);
					}
					TextVertices.Reset();
					TextTriangles.Reset();
					TextNormals.Reset();
					TextUVs.Reset();
					TextVertexColors.Reset();
					TextTangents.Reset();
				}
			}
		}

		YieldBorderMesh->SetVisibility(false);
		YieldFillMesh->SetVisibility(false);
		YieldTextMesh->SetVisibility(false);
	}

	UpdateTileYieldOverlayVisibility();
}

void AModularHexGridActor::UpdateTileYieldOverlayForTile(const FIntPoint& Coord)
{
	const FHexTileData* Tile = GridModel.GetTile(Coord);
	if (!Tile)
	{
		return;
	}

	const float IconRadius = FMath::Max(1.0f, GridModel.GetHexRadius() * TileYieldHexScale);
	const float FillRadius = FMath::Max(1.0f, IconRadius - FMath::Max(0.0f, TileYieldBorderWidth));
	const FLinearColor BorderLinearColor = FLinearColor::Black;
	const FColor BorderColor = BorderLinearColor.ToFColor(true);
	const FVector FlatCenter = GridModel.GetHexCenter(Coord.X, Coord.Y);
	const FVector BorderCenter(FlatCenter.X, FlatCenter.Y, GetTileYieldOverlayZ(*Tile));
	const FVector FillCenter = BorderCenter + FVector(0.0f, 0.0f, 0.5f);

	auto AddHex = [](
		const FVector& Center,
		float Radius,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		const int32 CenterIndex = Vertices.Num();
		Vertices.Add(Center);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(0.5f, 0.5f));
		VertexColors.Add(Color);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

		int32 CornerIndices[6];
		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const float AngleRadians = FMath::DegreesToRadians(60.0f * static_cast<float>(CornerIndex));
			const FVector Corner(
				Center.X + FMath::Cos(AngleRadians) * Radius,
				Center.Y + FMath::Sin(AngleRadians) * Radius,
				Center.Z
			);

			CornerIndices[CornerIndex] = Vertices.Num();
			Vertices.Add(Corner);
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(
				0.5f + FMath::Cos(AngleRadians) * 0.5f,
				0.5f + FMath::Sin(AngleRadians) * 0.5f
			));
			VertexColors.Add(Color);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}

		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const int32 NextCornerIndex = (CornerIndex + 1) % 6;
			Triangles.Add(CenterIndex);
			Triangles.Add(CornerIndices[NextCornerIndex]);
			Triangles.Add(CornerIndices[CornerIndex]);
		}
	};

	auto AddRect = [](
		const FVector& Center,
		float HalfWidth,
		float HalfHeight,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		const int32 StartIndex = Vertices.Num();
		Vertices.Add(FVector(Center.X - HalfWidth, Center.Y - HalfHeight, Center.Z));
		Vertices.Add(FVector(Center.X + HalfWidth, Center.Y - HalfHeight, Center.Z));
		Vertices.Add(FVector(Center.X + HalfWidth, Center.Y + HalfHeight, Center.Z));
		Vertices.Add(FVector(Center.X - HalfWidth, Center.Y + HalfHeight, Center.Z));

		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex + 1);
		Triangles.Add(StartIndex + 0);
		Triangles.Add(StartIndex + 3);
		Triangles.Add(StartIndex + 2);

		for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
		{
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D::ZeroVector);
			VertexColors.Add(Color);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}
	};

	auto AddDigit = [&AddRect](
		int32 Digit,
		const FVector& Center,
		float Height,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		static const bool SegmentMasks[10][7] =
		{
			{ true, true, true, true, true, true, false },
			{ false, true, true, false, false, false, false },
			{ true, true, false, true, true, false, true },
			{ true, true, true, true, false, false, true },
			{ false, true, true, false, false, true, true },
			{ true, false, true, true, false, true, true },
			{ true, false, true, true, true, true, true },
			{ true, true, true, false, false, false, false },
			{ true, true, true, true, true, true, true },
			{ true, true, true, true, false, true, true }
		};

		const float Width = Height * 0.55f;
		const float Thickness = Height * 0.13f;
		const float HalfHeight = Height * 0.5f;
		const float HalfWidth = Width * 0.5f;
		const float VerticalSegmentHalfHeight = (Height - Thickness * 3.0f) * 0.25f;
		const float HorizontalHalfWidth = HalfWidth - Thickness * 0.5f;
		const float VisualRightX = -HalfWidth + Thickness * 0.5f;
		const float VisualLeftX = -VisualRightX;
		const float UpperY = HalfHeight * 0.5f;
		const float LowerY = -HalfHeight * 0.5f;

		const auto& Mask = SegmentMasks[FMath::Clamp(Digit, 0, 9)];
		if (Mask[0]) AddRect(Center + FVector(0.0f, HalfHeight - Thickness * 0.5f, 0.0f), HorizontalHalfWidth, Thickness * 0.5f, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[1]) AddRect(Center + FVector(VisualRightX, UpperY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[2]) AddRect(Center + FVector(VisualRightX, LowerY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[3]) AddRect(Center + FVector(0.0f, -HalfHeight + Thickness * 0.5f, 0.0f), HorizontalHalfWidth, Thickness * 0.5f, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[4]) AddRect(Center + FVector(VisualLeftX, LowerY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[5]) AddRect(Center + FVector(VisualLeftX, UpperY, 0.0f), Thickness * 0.5f, VerticalSegmentHalfHeight, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		if (Mask[6]) AddRect(Center, HorizontalHalfWidth, Thickness * 0.5f, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
	};

	auto AddNumber = [&AddDigit](
		int32 Value,
		const FVector& Center,
		float Height,
		const FColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents
	)
	{
		const FString Digits = FString::FromInt(FMath::Max(0, Value));
		const float DigitWidth = Height * 0.65f;
		const float TotalWidth = DigitWidth * static_cast<float>(Digits.Len());
		for (int32 DigitIndex = 0; DigitIndex < Digits.Len(); ++DigitIndex)
		{
			const int32 Digit = Digits[DigitIndex] - TCHAR('0');
			const float OffsetX = -TotalWidth * 0.5f + DigitWidth * 0.5f + DigitWidth * static_cast<float>(DigitIndex);
			AddDigit(Digit, Center + FVector(OffsetX, 0.0f, 0.0f), Height, Color, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		}
	};

	for (int32 LayerIndex = 0; LayerIndex < 5; ++LayerIndex)
	{
		if (
			!TileYieldBorderMeshes.IsValidIndex(LayerIndex) ||
			!TileYieldFillMeshes.IsValidIndex(LayerIndex) ||
			!TileYieldTextMeshes.IsValidIndex(LayerIndex)
		)
		{
			continue;
		}

		UProceduralMeshComponent* YieldBorderMesh = TileYieldBorderMeshes[LayerIndex].Get();
		UProceduralMeshComponent* YieldFillMesh = TileYieldFillMeshes[LayerIndex].Get();
		UProceduralMeshComponent* YieldTextMesh = TileYieldTextMeshes[LayerIndex].Get();
		if (!YieldBorderMesh || !YieldFillMesh || !YieldTextMesh)
		{
			continue;
		}

		const EConquestYieldType YieldType = GetTileYieldTypeForLayer(LayerIndex);
		const int32 YieldValue = GetTileYieldValue(Tile->FinalYield, YieldType);
		TMap<FIntPoint, int32>& SectionIndices = TileYieldSectionIndicesByLayer.FindOrAdd(LayerIndex);

		int32* ExistingSectionIndex = SectionIndices.Find(Coord);
		if (YieldValue <= 0)
		{
			if (ExistingSectionIndex)
			{
				YieldBorderMesh->SetMeshSectionVisible(*ExistingSectionIndex, false);
				YieldFillMesh->SetMeshSectionVisible(*ExistingSectionIndex, false);
				YieldTextMesh->SetMeshSectionVisible(*ExistingSectionIndex, false);
			}
			continue;
		}

		const int32 SectionIndex = ExistingSectionIndex ? *ExistingSectionIndex : SectionIndices.Num();
		if (!ExistingSectionIndex)
		{
			SectionIndices.Add(Coord, SectionIndex);
		}

		TArray<FVector> BorderVertices;
		TArray<int32> BorderTriangles;
		TArray<FVector> BorderNormals;
		TArray<FVector2D> BorderUVs;
		TArray<FColor> BorderVertexColors;
		TArray<FProcMeshTangent> BorderTangents;

		TArray<FVector> FillVertices;
		TArray<int32> FillTriangles;
		TArray<FVector> FillNormals;
		TArray<FVector2D> FillUVs;
		TArray<FColor> FillVertexColors;
		TArray<FProcMeshTangent> FillTangents;

		TArray<FVector> TextVertices;
		TArray<int32> TextTriangles;
		TArray<FVector> TextNormals;
		TArray<FVector2D> TextUVs;
		TArray<FColor> TextVertexColors;
		TArray<FProcMeshTangent> TextTangents;

		AddHex(
			BorderCenter,
			IconRadius,
			BorderColor,
			BorderVertices,
			BorderTriangles,
			BorderNormals,
			BorderUVs,
			BorderVertexColors,
			BorderTangents
		);

		AddHex(
			FillCenter,
			FillRadius,
			GetTileYieldColor(YieldType).ToFColor(true),
			FillVertices,
			FillTriangles,
			FillNormals,
			FillUVs,
			FillVertexColors,
			FillTangents
		);

		YieldBorderMesh->CreateMeshSection(
			SectionIndex,
			BorderVertices,
			BorderTriangles,
			BorderNormals,
			BorderUVs,
			BorderVertexColors,
			BorderTangents,
			false
		);

		YieldFillMesh->CreateMeshSection(
			SectionIndex,
			FillVertices,
			FillTriangles,
			FillNormals,
			FillUVs,
			FillVertexColors,
			FillTangents,
			false
		);

		if (TileYieldBorderMaterialInstances.IsValidIndex(LayerIndex) && TileYieldBorderMaterialInstances[LayerIndex])
		{
			YieldBorderMesh->SetMaterial(SectionIndex, TileYieldBorderMaterialInstances[LayerIndex]);
		}
		else
		{
			UMaterialInterface* BorderSourceMaterial = bUseTileYieldFillMaterialForBorder && TileYieldFillMaterial
				? TileYieldFillMaterial.Get()
				: TileYieldBorderMaterial.Get();
			if (BorderSourceMaterial)
			{
				YieldBorderMesh->SetMaterial(SectionIndex, BorderSourceMaterial);
			}
		}

		if (TileYieldFillMaterialInstances.IsValidIndex(LayerIndex) && TileYieldFillMaterialInstances[LayerIndex])
		{
			YieldFillMesh->SetMaterial(SectionIndex, TileYieldFillMaterialInstances[LayerIndex]);
		}
		else if (TileYieldFillMaterial)
		{
			YieldFillMesh->SetMaterial(SectionIndex, TileYieldFillMaterial);
		}

		if (bShowTileYieldText)
		{
			AddNumber(
				YieldValue,
				FillCenter + FVector(0.0f, 0.0f, TileYieldTextHeightOffset),
				TileYieldTextWorldSize,
				TileYieldTextColor.ToFColor(true),
				TextVertices,
				TextTriangles,
				TextNormals,
				TextUVs,
				TextVertexColors,
				TextTangents
			);

			YieldTextMesh->CreateMeshSection(
				SectionIndex,
				TextVertices,
				TextTriangles,
				TextNormals,
				TextUVs,
				TextVertexColors,
				TextTangents,
				false
			);
			if (TileYieldTextMaterialInstances.IsValidIndex(LayerIndex) && TileYieldTextMaterialInstances[LayerIndex])
			{
				YieldTextMesh->SetMaterial(SectionIndex, TileYieldTextMaterialInstances[LayerIndex]);
			}
			else if (TileYieldTextMaterial)
			{
				YieldTextMesh->SetMaterial(SectionIndex, TileYieldTextMaterial);
			}
			else if (TileYieldFillMaterial)
			{
				YieldTextMesh->SetMaterial(SectionIndex, TileYieldFillMaterial);
			}
		}
		else
		{
			YieldTextMesh->SetMeshSectionVisible(SectionIndex, false);
		}
	}

	UpdateTileYieldOverlayVisibility();
}

void AModularHexGridActor::UpdateTileYieldOverlayVisibility()
{
	const int32 ActiveLayerIndex = bHasActiveTileYieldLens
		? GetTileYieldLayerIndex(ActiveTileYieldLens)
		: INDEX_NONE;

	for (int32 LayerIndex = 0; LayerIndex < 5; ++LayerIndex)
	{
		const bool bLayerVisible =
			bShowTileYieldOverlay &&
			bHasActiveTileYieldLens &&
			LayerIndex == ActiveLayerIndex;

		UProceduralMeshComponent* YieldBorderMesh = TileYieldBorderMeshes.IsValidIndex(LayerIndex)
			? TileYieldBorderMeshes[LayerIndex].Get()
			: nullptr;
		UProceduralMeshComponent* YieldFillMesh = TileYieldFillMeshes.IsValidIndex(LayerIndex)
			? TileYieldFillMeshes[LayerIndex].Get()
			: nullptr;
		UProceduralMeshComponent* YieldTextMesh = TileYieldTextMeshes.IsValidIndex(LayerIndex)
			? TileYieldTextMeshes[LayerIndex].Get()
			: nullptr;

		if (YieldBorderMesh)
		{
			YieldBorderMesh->SetVisibility(bLayerVisible);
		}

		if (YieldFillMesh)
		{
			YieldFillMesh->SetVisibility(bLayerVisible);
		}

		if (YieldTextMesh)
		{
			YieldTextMesh->SetVisibility(bShowTileYieldText && bLayerVisible);
		}

		TMap<FIntPoint, int32>* SectionIndices = TileYieldSectionIndicesByLayer.Find(LayerIndex);
		if (SectionIndices)
		{
			for (const TPair<FIntPoint, int32>& Pair : *SectionIndices)
			{
				const bool bSectionVisible =
					bLayerVisible &&
					IsTileDiscoveredForYieldOverlay(Pair.Key) &&
					IsTileInYieldHoverRange(Pair.Key);

				if (YieldBorderMesh)
				{
					YieldBorderMesh->SetMeshSectionVisible(Pair.Value, bSectionVisible);
				}

				if (YieldFillMesh)
				{
					YieldFillMesh->SetMeshSectionVisible(Pair.Value, bSectionVisible);
				}

				if (YieldTextMesh)
				{
					YieldTextMesh->SetMeshSectionVisible(Pair.Value, bShowTileYieldText && bSectionVisible);
				}
			}
		}
	}
}

void AModularHexGridActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AModularHexGridActor, ReplicatedSetupSettings);
}

void AModularHexGridActor::ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings)
{
	ReplicatedSetupSettings = SetupSettings;

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

void AModularHexGridActor::OnRep_ReplicatedSetupSettings()
{
	ApplyGameSetupSettings(ReplicatedSetupSettings);

	if (AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr)
	{
		if (ConquestGS->ActiveGridActor == this)
		{
			ConquestGS->ApplyReplicatedTileImprovements();
			ConquestGS->RebuildCityVisualsFromReplicatedState();
			ConquestGS->RebuildUnitVisualsFromReplicatedState();
			ConquestGS->OnConquestStateChanged.Broadcast();
		}
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
	LocallyRevealedFogTiles.Reset();

	GridModel.Initialize(SizeSettings, HeightSettings, TileResourceData, ResourceSetData, ImprovementTable);

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
	RebuildTileYieldOverlay();


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

	ImprovementMeshBuilder.BuildImprovementMeshes(
		this,
		SceneRoot,
		GridModel,
		ImprovementMeshComponents
	);
	

	if (bGenerateFogOfWar)
	{
		RebuildFogOfWarMesh();
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
	if (CivilisationBorderFillMesh)
	{
		CivilisationBorderFillMesh->ClearAllMeshSections();
		CivilisationBorderFillMesh->SetVisibility(false);
	}
	ClearExpansionCandidateHighlights();
}

void AModularHexGridActor::RegenerateGridWithNewRandomSeed()
{
	GenerationSettings.RandomSeed = FMath::RandRange(1, TNumericLimits<int32>::Max() - 1);
	if (HasAuthority())
	{
		ReplicatedSetupSettings.RandomSeed = GenerationSettings.RandomSeed;
		ForceNetUpdate();
	}

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
	UpdateTileYieldOverlayVisibility();
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

	UpdateTileYieldOverlayVisibility();
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
		if (bGenerateFogOfWar && FogOfWarMesh->GetNumSections() <= 0)
		{
			RebuildFogOfWarMesh();
		}

		FogOfWarMesh->SetVisibility(bGenerateFogOfWar);
	}
}

void AModularHexGridActor::ResetLocalFogOfWar(bool bVisible)
{
	LocallyRevealedFogTiles.Reset();
	bGenerateFogOfWar = bVisible;
	RebuildFogOfWarMesh();
	UpdateTileYieldOverlayVisibility();
}

void AModularHexGridActor::RevealFogOfWarAroundTile(FIntPoint CenterCoord, int32 Radius)
{
	if (!GridModel.IsValidTile(CenterCoord.X, CenterCoord.Y))
	{
		return;
	}

	const int32 SafeRadius = FMath::Max(0, Radius);
	bool bRevealedAnyTile = false;
	for (const FIntPoint& Coord : GridModel.GetCoordsInRange(CenterCoord, SafeRadius))
	{
		if (GridModel.IsValidTile(Coord.X, Coord.Y))
		{
			const int32 PreviousCount = LocallyRevealedFogTiles.Num();
			LocallyRevealedFogTiles.Add(Coord);
			bRevealedAnyTile = bRevealedAnyTile || LocallyRevealedFogTiles.Num() != PreviousCount;
		}
	}

	bGenerateFogOfWar = true;
	if (bRevealedAnyTile)
	{
		RebuildFogOfWarMesh();
		UpdateTileYieldOverlayVisibility();
	}
	else if (FogOfWarMesh)
	{
		FogOfWarMesh->SetVisibility(true);
		UpdateTileYieldOverlayVisibility();
	}
}

void AModularHexGridActor::RebuildFogOfWarMesh()
{
	if (!FogOfWarMesh)
	{
		return;
	}

	if (!bGenerateFogOfWar)
	{
		FogOfWarMesh->ClearAllMeshSections();
		FogOfWarMesh->SetVisibility(false);
		return;
	}

	MeshBuilder.BuildFogOfWarMesh(
		FogOfWarMesh,
		GridModel,
		FogOfWarSettings,
		&LocallyRevealedFogTiles
	);

	FogOfWarMesh->SetVisibility(true);
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
		RebuildPlacedTileVisualMeshes();
		UpdateTileYieldOverlayForTile(FIntPoint(Q, R));

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

void AModularHexGridActor::RefreshPlacedTileVisualMeshes()
{
	MeshBuilder.BuildTerrainMesh(GridMesh, GridModel, TileResourceData);
	RebuildPlacedTileVisualMeshes();
	RebuildTileYieldOverlay();
}

void AModularHexGridActor::RebuildPlacedTileVisualMeshes()
{
	FeatureMeshBuilder.BuildFeatureMeshes(
		this,
		SceneRoot,
		GridModel,
		TileResourceData,
		GenerationSettings.RandomSeed,
		FeatureMeshComponents
	);

	ImprovementMeshBuilder.BuildImprovementMeshes(
		this,
		SceneRoot,
		GridModel,
		ImprovementMeshComponents
	);
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

	for (TObjectPtr<UProceduralMeshComponent>& YieldBorderMesh : TileYieldBorderMeshes)
	{
		if (YieldBorderMesh)
		{
			YieldBorderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			YieldBorderMesh->SetVisibility(false);
			YieldBorderMesh->SetCastShadow(false);
			YieldBorderMesh->bCastDynamicShadow = false;
			YieldBorderMesh->bCastStaticShadow = false;
			YieldBorderMesh->CastShadow = false;
			YieldBorderMesh->TranslucencySortPriority = TileYieldBorderTranslucencySortPriority;
		}
	}

	for (TObjectPtr<UProceduralMeshComponent>& YieldFillMesh : TileYieldFillMeshes)
	{
		if (YieldFillMesh)
		{
			YieldFillMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			YieldFillMesh->SetVisibility(false);
			YieldFillMesh->SetCastShadow(false);
			YieldFillMesh->bCastDynamicShadow = false;
			YieldFillMesh->bCastStaticShadow = false;
			YieldFillMesh->CastShadow = false;
			YieldFillMesh->TranslucencySortPriority = TileYieldFillTranslucencySortPriority;
		}
	}

	for (TObjectPtr<UProceduralMeshComponent>& YieldTextMesh : TileYieldTextMeshes)
	{
		if (YieldTextMesh)
		{
			YieldTextMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			YieldTextMesh->SetVisibility(false);
			YieldTextMesh->SetCastShadow(false);
			YieldTextMesh->bCastDynamicShadow = false;
			YieldTextMesh->bCastStaticShadow = false;
			YieldTextMesh->CastShadow = false;
			YieldTextMesh->TranslucencySortPriority = TileYieldTextTranslucencySortPriority;
		}
	}

	if (CivilisationBorderMesh)
	{
		CivilisationBorderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CivilisationBorderMesh->SetCastShadow(false);
		CivilisationBorderMesh->bCastDynamicShadow = false;
		CivilisationBorderMesh->bCastStaticShadow = false;
		CivilisationBorderMesh->CastShadow = false;
		CivilisationBorderMesh->TranslucencySortPriority = CivilisationBorderTranslucencySortPriority;
		CivilisationBorderMesh->SetVisibility(false);
	}

	if (CivilisationBorderFillMesh)
	{
		CivilisationBorderFillMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CivilisationBorderFillMesh->SetCastShadow(false);
		CivilisationBorderFillMesh->bCastDynamicShadow = false;
		CivilisationBorderFillMesh->bCastStaticShadow = false;
		CivilisationBorderFillMesh->CastShadow = false;
		CivilisationBorderFillMesh->TranslucencySortPriority = CivilisationBorderFillTranslucencySortPriority;
		CivilisationBorderFillMesh->SetVisibility(false);
	}

	if (ExpansionCandidateMesh)
	{
		ExpansionCandidateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ExpansionCandidateMesh->SetCastShadow(false);
		ExpansionCandidateMesh->bCastDynamicShadow = false;
		ExpansionCandidateMesh->bCastStaticShadow = false;
		ExpansionCandidateMesh->CastShadow = false;
		ExpansionCandidateMesh->TranslucencySortPriority = 6;
		ExpansionCandidateMesh->SetVisibility(false);
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
