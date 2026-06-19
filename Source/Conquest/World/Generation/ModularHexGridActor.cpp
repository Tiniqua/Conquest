#include "ModularHexGridActor.h"

#include "ConquestGameSetupTypes.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "HexMapTypePresets.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Conquest/UI/ConquestCityWorldLabelWidget.h"

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
	UpdateCityWorldLabel(CityId, CityName, Population, CivilisationThemeMaterial, CivilisationThemeColor);
}

void AModularHexGridActor::UpdateCityWorldLabel(
	int32 CityId,
	FName CityName,
	int32 Population,
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
		LabelWidget->SetCityLabel(CityName, Population, CivilisationThemeMaterial, CivilisationThemeColor);
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
}

void AModularHexGridActor::RebuildCivilisationBorders(int32 OwnerPlayerId, UMaterialInterface* BorderMaterial, UMaterialInterface* BorderFillMaterial)
{
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

void AModularHexGridActor::RebuildCivilisationBordersForTiles(const TArray<FIntPoint>& OwnedTiles, UMaterialInterface* BorderMaterial, UMaterialInterface* BorderFillMaterial)
{
	if (!CivilisationBorderMesh)
	{
		return;
	}

	CivilisationBorderMesh->ClearAllMeshSections();
	if (CivilisationBorderFillMesh)
	{
		CivilisationBorderFillMesh->ClearAllMeshSections();
	}

	if (OwnedTiles.Num() <= 0)
	{
		CivilisationBorderMesh->SetVisibility(false);
		if (CivilisationBorderFillMesh)
		{
			CivilisationBorderFillMesh->SetVisibility(false);
		}
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
			CivilisationBorderFillMesh->CreateMeshSection(
				0,
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
				CivilisationBorderFillMesh->SetMaterial(0, EffectiveFillMaterial);
			}

			CivilisationBorderFillMesh->SetVisibility(true);
		}
		else
		{
			CivilisationBorderFillMesh->SetVisibility(false);
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

	ImprovementMeshBuilder.BuildImprovementMeshes(
		this,
		SceneRoot,
		GridModel,
		ImprovementSetData,
		ImprovementMeshComponents
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
		RebuildPlacedTileVisualMeshes();

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
		ImprovementSetData,
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
