#include "Conquest/Units/ConquestUnitActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Conquest/UI/ConquestUnitWorldIconWidget.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "Conquest/World/Generation/ModularHexGridActor.h"

AConquestUnitActor::AConquestUnitActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	UnitMeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("UnitMeshInstances"));
	UnitMeshInstances->SetupAttachment(SceneRoot);
	UnitMeshInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UnitMeshInstances->SetGenerateOverlapEvents(false);
	UnitMeshInstances->bCastDynamicShadow = true;
	UnitMeshInstances->bCastStaticShadow = true;
	UnitMeshInstances->CastShadow = true;

	SelectionMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SelectionMesh"));
	SelectionMesh->SetupAttachment(SceneRoot);
	SelectionMesh->bUseAsyncCooking = true;
	SelectionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionMesh->SetCastShadow(false);
	SelectionMesh->bCastDynamicShadow = false;
	SelectionMesh->bCastStaticShadow = false;
	SelectionMesh->CastShadow = false;
	SelectionMesh->SetVisibility(false);

	UnitWorldIconWidgetClass = UConquestUnitWorldIconWidget::StaticClass();
	UnitWorldIconComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UnitWorldIcon"));
	UnitWorldIconComponent->SetupAttachment(SceneRoot);
	UnitWorldIconComponent->SetWidgetSpace(EWidgetSpace::World);
	UnitWorldIconComponent->SetWidgetClass(UnitWorldIconWidgetClass);
	UnitWorldIconComponent->SetDrawSize(UnitWorldIconDrawSize);
	UnitWorldIconComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UnitWorldIconComponent->SetGenerateOverlapEvents(false);
	UnitWorldIconComponent->SetCastShadow(false);
	UnitWorldIconComponent->bCastDynamicShadow = false;
	UnitWorldIconComponent->bCastStaticShadow = false;
	UnitWorldIconComponent->CastShadow = false;
	UnitWorldIconComponent->SetTwoSided(true);
	UnitWorldIconComponent->SetPivot(FVector2D(0.5f, 0.5f));
	UnitWorldIconComponent->SetTickWhenOffscreen(true);
	UnitWorldIconComponent->SetRelativeTransform(FTransform(
		UnitWorldIconRotation,
		UnitWorldIconOffset,
		FVector(UnitWorldIconScale, UnitWorldIconScale, UnitWorldIconScale)
	));
}

void AConquestUnitActor::InitializeUnit(
	const FConquestUnitState& InUnitState,
	const FConquestUnitRow& InUnitRow,
	AModularHexGridActor* InGridActor,
	const FText& UnitName,
	const FText& CivilisationName,
	FLinearColor UnitDisplayColor,
	UMaterialInterface* UnitIconMaterial
)
{
	GridActor = InGridActor;
	RefreshUnitVisuals(InUnitState, InUnitRow, UnitName, CivilisationName, UnitDisplayColor, UnitIconMaterial);
}

void AConquestUnitActor::RefreshUnitVisuals(
	const FConquestUnitState& InUnitState,
	const FConquestUnitRow& InUnitRow,
	const FText& UnitName,
	const FText& CivilisationName,
	FLinearColor UnitDisplayColor,
	UMaterialInterface* UnitIconMaterial
)
{
	UnitInstanceId = InUnitState.UnitInstanceId;
	OwnerPlayerId = InUnitState.OwnerPlayerId;
	TileCoord = InUnitState.TileCoord;
	CurrentHealth = InUnitState.CurrentHealth;
	MaxHealth = FMath::Max(1, InUnitState.CachedMaxHealth);
	UnitMeshCount = FMath::Max(1, InUnitRow.UnitMeshCount);
	UnitMeshSpacing = FMath::Max(0.0f, InUnitRow.UnitMeshSpacing);
	UnitMeshScale = InUnitRow.UnitMeshScale;
	UnitMeshOffset = InUnitRow.UnitMeshOffset;
	UnitMeshRotation = InUnitRow.UnitMeshRotation;

	if (UnitMeshInstances)
	{
		UnitMeshInstances->SetStaticMesh(InUnitRow.UnitMesh);

		if (InUnitRow.UnitMesh)
		{
			const int32 MaterialSlotCount = FMath::Max(1, InUnitRow.UnitMesh->GetStaticMaterials().Num());
			for (int32 MaterialIndex = 0; MaterialIndex < MaterialSlotCount; ++MaterialIndex)
			{
				UnitMeshInstances->SetMaterial(MaterialIndex, InUnitRow.UnitMeshMaterialOverride);
			}
		}
	}

	if (GridActor)
	{
		UpdateActorTransformForTile();
	}

	RebuildMeshInstances();
	UpdateUnitWorldIcon(UnitName, CivilisationName, UnitDisplayColor, UnitIconMaterial);
}

void AConquestUnitActor::SetSelected(bool bSelected, UMaterialInterface* SelectionMaterial)
{
	if (bSelected)
	{
		RebuildSelectionMesh(SelectionMaterial);
	}
	else
	{
		ClearSelectionMesh();
	}
}

void AConquestUnitActor::MoveToTile(const FIntPoint& NewCoord)
{
	TileCoord = NewCoord;
	UpdateActorTransformForTile();
	RebuildMeshInstances();
	UpdateUnitWorldIconTransform();
	if (SelectionMesh && SelectionMesh->IsVisible())
	{
		RebuildSelectionMesh(SelectionMesh->GetMaterial(0));
	}
}

void AConquestUnitActor::RebuildMeshInstances()
{
	if (!UnitMeshInstances)
	{
		return;
	}

	UnitMeshInstances->ClearInstances();

	if (!UnitMeshInstances->GetStaticMesh() || !GridActor)
	{
		return;
	}

	const int32 VisibleMeshCount = FMath::Clamp(
		FMath::CeilToInt((static_cast<float>(FMath::Max(0, CurrentHealth)) / static_cast<float>(MaxHealth)) * UnitMeshCount),
		CurrentHealth > 0 ? 1 : 0,
		UnitMeshCount
	);

	if (VisibleMeshCount <= 0)
	{
		return;
	}

	const FVector TileCenter = GetTileLocalCenter();
	const TArray<FVector> Offsets = BuildFormationOffsets(VisibleMeshCount);
	const FVector FinalScale = UnitMeshScale / FMath::Sqrt(static_cast<float>(UnitMeshCount));

	for (int32 OffsetIndex = 0; OffsetIndex < Offsets.Num(); ++OffsetIndex)
	{
		const FVector& Offset = Offsets[OffsetIndex];
		const FVector SurfaceLocation = ProjectLocalPointToTerrain(TileCenter + Offset);
		UnitMeshInstances->AddInstance(FTransform(
			BuildInstanceRotation(OffsetIndex),
			GridLocalToActorLocal(SurfaceLocation + UnitMeshOffset),
			FinalScale
		));
	}

	UnitMeshInstances->MarkRenderStateDirty();
}

void AConquestUnitActor::ConfigureUnitWorldIconComponent()
{
	if (!UnitWorldIconComponent || !UnitWorldIconWidgetClass)
	{
		return;
	}

	UnitWorldIconComponent->SetWidgetClass(UnitWorldIconWidgetClass);
	UnitWorldIconComponent->SetWidgetSpace(EWidgetSpace::World);
	UnitWorldIconComponent->SetDrawSize(UnitWorldIconDrawSize);
	UnitWorldIconComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UnitWorldIconComponent->SetGenerateOverlapEvents(false);
	UnitWorldIconComponent->SetCastShadow(false);
	UnitWorldIconComponent->bCastDynamicShadow = false;
	UnitWorldIconComponent->bCastStaticShadow = false;
	UnitWorldIconComponent->CastShadow = false;
	UnitWorldIconComponent->SetTwoSided(true);
	UnitWorldIconComponent->SetPivot(FVector2D(0.5f, 0.5f));
	UnitWorldIconComponent->SetTickWhenOffscreen(true);
	UpdateUnitWorldIconTransform();
}

void AConquestUnitActor::UpdateUnitWorldIconTransform()
{
	if (!UnitWorldIconComponent)
	{
		return;
	}

	UnitWorldIconComponent->SetRelativeTransform(FTransform(
		UnitWorldIconRotation,
		UnitWorldIconOffset,
		FVector(UnitWorldIconScale, UnitWorldIconScale, UnitWorldIconScale)
	));
}

void AConquestUnitActor::UpdateUnitWorldIcon(
	const FText& UnitName,
	const FText& CivilisationName,
	FLinearColor UnitDisplayColor,
	UMaterialInterface* UnitIconMaterial
)
{
	if (!UnitWorldIconComponent || !UnitWorldIconWidgetClass)
	{
		return;
	}

	ConfigureUnitWorldIconComponent();
	UnitWorldIconComponent->InitWidget();
	UnitWorldIconComponent->SetVisibility(true);

	if (UConquestUnitWorldIconWidget* IconWidget = Cast<UConquestUnitWorldIconWidget>(UnitWorldIconComponent->GetWidget()))
	{
		IconWidget->SetUnitIcon(UnitName, CivilisationName, UnitDisplayColor, UnitIconMaterial);
	}
}

void AConquestUnitActor::RebuildSelectionMesh(UMaterialInterface* SelectionMaterial)
{
	if (!SelectionMesh || !GridActor)
	{
		return;
	}

	SelectionMesh->ClearAllMeshSections();

	const FHexGridModel& GridModel = GridActor->GetGridModel();
	if (!GridModel.IsValidTile(TileCoord.X, TileCoord.Y))
	{
		SelectionMesh->SetVisibility(false);
		return;
	}

	const FVector Center = GridModel.GetHexCenter(TileCoord.X, TileCoord.Y);
	constexpr float SurfaceOffset = 18.0f;
	const float EdgeWidth = FMath::Max(6.0f, GridModel.GetHexRadius() * 0.08f);

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
		else if (const FHexTileData* Tile = GridModel.GetTile(TileCoord))
		{
			CornerZ = Tile->Height;
		}

		Corners[CornerIndex] = FVector(FlatCorner.X, FlatCorner.Y, CornerZ + SurfaceOffset);
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	auto AddQuad = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& D)
	{
		const int32 StartIndex = Vertices.Num();
		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		Vertices.Add(D);
		Triangles.Add(StartIndex);
		Triangles.Add(StartIndex + 1);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex + 3);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D::ZeroVector);
			VertexColors.Add(FColor::White);
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}
	};

	for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
	{
		const FVector A = Corners[EdgeIndex];
		const FVector B = Corners[(EdgeIndex + 1) % 6];
		FVector ToCenter = Center - ((A + B) * 0.5f);
		ToCenter.Z = 0.0f;
		ToCenter.Normalize();
		const FVector HalfWidth = ToCenter * EdgeWidth * 0.5f;
		AddQuad(
			GridLocalToActorLocal(A - HalfWidth),
			GridLocalToActorLocal(B - HalfWidth),
			GridLocalToActorLocal(B + HalfWidth),
			GridLocalToActorLocal(A + HalfWidth)
		);
	}

	SelectionMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	if (SelectionMaterial)
	{
		SelectionMesh->SetMaterial(0, SelectionMaterial);
	}
	SelectionMesh->SetVisibility(true);
}

void AConquestUnitActor::ClearSelectionMesh()
{
	if (SelectionMesh)
	{
		SelectionMesh->ClearAllMeshSections();
		SelectionMesh->SetVisibility(false);
	}
}

void AConquestUnitActor::UpdateActorTransformForTile()
{
	if (!GridActor)
	{
		return;
	}

	const FVector SurfaceCenter = ProjectLocalPointToTerrain(GetTileLocalCenter());
	const FTransform& GridTransform = GridActor->GetActorTransform();
	SetActorTransform(FTransform(
		GridTransform.GetRotation(),
		GridTransform.TransformPosition(SurfaceCenter),
		GridTransform.GetScale3D()
	));
}

FVector AConquestUnitActor::GetTileLocalCenter() const
{
	return GridActor
		? GridActor->GetGridModel().GetHexCenter(TileCoord.X, TileCoord.Y)
		: FVector::ZeroVector;
}

FVector AConquestUnitActor::ProjectLocalPointToTerrain(const FVector& LocalPoint) const
{
	if (!GridActor)
	{
		return LocalPoint;
	}

	const FVector WorldTraceStart = GridActor->GetActorTransform().TransformPosition(LocalPoint + FVector(0.0f, 0.0f, 10000.0f));
	const FVector WorldTraceEnd = GridActor->GetActorTransform().TransformPosition(LocalPoint - FVector(0.0f, 0.0f, 10000.0f));

	if (UWorld* World = GetWorld())
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UnitMeshTerrainTrace), false, this);
		if (World->LineTraceSingleByChannel(HitResult, WorldTraceStart, WorldTraceEnd, ECC_Visibility, QueryParams))
		{
			return GridActor->GetActorTransform().InverseTransformPosition(HitResult.ImpactPoint);
		}
	}

	const FHexGridModel& GridModel = GridActor->GetGridModel();
	if (const FHexTileData* Tile = GridModel.GetTile(TileCoord))
	{
		return FVector(LocalPoint.X, LocalPoint.Y, Tile->Height);
	}

	return LocalPoint;
}

FVector AConquestUnitActor::GridLocalToActorLocal(const FVector& GridLocalPoint) const
{
	if (!GridActor)
	{
		return GridLocalPoint;
	}

	return GetActorTransform().InverseTransformPosition(
		GridActor->GetActorTransform().TransformPosition(GridLocalPoint)
	);
}

TArray<FVector> AConquestUnitActor::BuildFormationOffsets(int32 VisibleMeshCount) const
{
	TArray<FVector> Result;
	const float HexRadius = GridActor ? GridActor->GetGridModel().GetHexRadius() : 100.0f;
	const float MaxRadius = HexRadius * 0.52f;
	const float SafeSpacing = FMath::Min(UnitMeshSpacing, MaxRadius);
	const float MinimumSpacing = FMath::Min(SafeSpacing * 0.55f, MaxRadius * 0.45f);

	const int32 Seed = static_cast<int32>(HashCombine(
		::GetTypeHash(UnitInstanceId),
		HashCombine(::GetTypeHash(UnitMeshCount), ::GetTypeHash(OwnerPlayerId))
	));
	FRandomStream RandomStream(Seed);

	if (VisibleMeshCount <= 1)
	{
		const float Radius = MaxRadius * 0.15f * FMath::Sqrt(RandomStream.FRand());
		const float Angle = RandomStream.FRandRange(0.0f, TWO_PI);
		Result.Add(FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f));
		return Result;
	}

	for (int32 Index = 0; Index < VisibleMeshCount; ++Index)
	{
		FVector ChosenOffset = FVector::ZeroVector;
		float BestScore = -1.0f;

		for (int32 Attempt = 0; Attempt < 32; ++Attempt)
		{
			const float Radius = MaxRadius * FMath::Sqrt(RandomStream.FRand());
			const float Angle = RandomStream.FRandRange(0.0f, TWO_PI);
			const FVector Candidate(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);

			float NearestDistance = TNumericLimits<float>::Max();
			for (const FVector& ExistingOffset : Result)
			{
				NearestDistance = FMath::Min(
					NearestDistance,
					FVector::Dist2D(Candidate, ExistingOffset)
				);
			}

			if (Result.Num() <= 0)
			{
				ChosenOffset = Candidate;
				break;
			}

			if (NearestDistance >= MinimumSpacing)
			{
				ChosenOffset = Candidate;
				break;
			}

			if (NearestDistance > BestScore)
			{
				BestScore = NearestDistance;
				ChosenOffset = Candidate;
			}
		}

		Result.Add(ChosenOffset);
	}

	return Result;
}

FRotator AConquestUnitActor::BuildInstanceRotation(int32 InstanceIndex) const
{
	const int32 Seed = static_cast<int32>(HashCombine(
		::GetTypeHash(UnitInstanceId),
		HashCombine(::GetTypeHash(InstanceIndex), ::GetTypeHash(OwnerPlayerId))
	));
	FRandomStream RandomStream(Seed);

	FRotator Result = UnitMeshRotation;
	Result.Yaw += RandomStream.FRandRange(-35.0f, 35.0f);
	return Result;
}
