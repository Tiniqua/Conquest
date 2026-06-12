#include "HexFeatureMeshBuilder.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

#include "HexGridModel.h"
#include "HexTileResourceData.h"
#include "HexTileTypes.h"

void FHexFeatureMeshBuilder::BuildFeatureMeshes(
	AActor* Owner,
	USceneComponent* AttachParent,
	const FHexGridModel& GridModel,
	const UHexTileResourceData* TileData,
	int32 RandomSeed,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutFeatureMeshComponents
)
{
	ClearFeatureMeshes(InOutFeatureMeshComponents);

	if (!Owner || !AttachParent || !TileData)
	{
		return;
	}

	TMap<FMeshComponentKey, UInstancedStaticMeshComponent*> MeshToComponent;

	const TArray<FHexTileData>& Tiles = GridModel.GetTiles();

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		const FHexTileData& Tile = Tiles[TileIndex];

		if (Tile.Features.Num() <= 0)
		{
			continue;
		}

		int32 Q = 0;
		int32 R = 0;
		if (!GridModel.GetCoordFromIndex(TileIndex, Q, R))
		{
			continue;
		}

		for (const EHexFeatureType FeatureType : Tile.Features)
		{
			if (FeatureType == EHexFeatureType::None)
			{
				continue;
			}

			const FHexFeatureDefinition* FeatureDefinition =
				TileData->FindFeatureDefinition(FeatureType);

			if (!FeatureDefinition || !FeatureDefinition->WorldMesh)
			{
				continue;
			}

			UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshComponent(
				Owner,
				AttachParent,
				FeatureDefinition->WorldMesh,
				FeatureDefinition->WorldMeshMaterialOverride,
				MeshToComponent,
				InOutFeatureMeshComponents
			);

			if (!MeshComponent)
			{
				continue;
			}

			AddFeatureInstancesForTile(
				MeshComponent,
				GridModel,
				*FeatureDefinition,
				Q,
				R,
				TileIndex,
				RandomSeed
			);
		}
	}

	for (TObjectPtr<UInstancedStaticMeshComponent>& MeshComponent : InOutFeatureMeshComponents)
	{
		if (MeshComponent)
		{
			MeshComponent->MarkRenderStateDirty();
		}
	}
}

void FHexFeatureMeshBuilder::ClearFeatureMeshes(
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutFeatureMeshComponents
)
{
	for (TObjectPtr<UInstancedStaticMeshComponent>& MeshComponent : InOutFeatureMeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->ClearInstances();
		MeshComponent->DestroyComponent();
	}

	InOutFeatureMeshComponents.Reset();
}

UInstancedStaticMeshComponent* FHexFeatureMeshBuilder::FindOrCreateMeshComponent(
	AActor* Owner,
	USceneComponent* AttachParent,
	UStaticMesh* Mesh,
	UMaterialInterface* MaterialOverride,
	TMap<FMeshComponentKey, UInstancedStaticMeshComponent*>& MeshToComponent,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutFeatureMeshComponents
)
{
	if (!Owner || !AttachParent || !Mesh)
	{
		return nullptr;
	}

	FMeshComponentKey Key;
	Key.Mesh = Mesh;
	Key.MaterialOverride = MaterialOverride;

	if (UInstancedStaticMeshComponent** ExistingComponent = MeshToComponent.Find(Key))
	{
		return *ExistingComponent;
	}

	const FString MaterialName = MaterialOverride
		? MaterialOverride->GetName()
		: TEXT("DefaultMat");

	const FName ComponentName = MakeUniqueObjectName(
		Owner,
		UInstancedStaticMeshComponent::StaticClass(),
		FName(*FString::Printf(
			TEXT("FeatureMesh_%s_%s"),
			*Mesh->GetName(),
			*MaterialName
		))
	);

	UInstancedStaticMeshComponent* NewComponent =
		NewObject<UInstancedStaticMeshComponent>(Owner, ComponentName);

	if (!NewComponent)
	{
		return nullptr;
	}

	NewComponent->SetStaticMesh(Mesh);

	if (MaterialOverride)
	{
		const int32 MaterialSlotCount = Mesh->GetStaticMaterials().Num();

		for (int32 MaterialIndex = 0; MaterialIndex < MaterialSlotCount; ++MaterialIndex)
		{
			NewComponent->SetMaterial(MaterialIndex, MaterialOverride);
		}
	}

	NewComponent->SetupAttachment(AttachParent);
	NewComponent->RegisterComponent();

	NewComponent->SetMobility(EComponentMobility::Static);
	NewComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	NewComponent->SetGenerateOverlapEvents(false);

	NewComponent->bCastDynamicShadow = true;
	NewComponent->bCastStaticShadow = true;
	NewComponent->CastShadow = true;

	Owner->AddInstanceComponent(NewComponent);

	MeshToComponent.Add(Key, NewComponent);
	InOutFeatureMeshComponents.Add(NewComponent);

	return NewComponent;
}

void FHexFeatureMeshBuilder::AddFeatureInstancesForTile(
	UInstancedStaticMeshComponent* MeshComponent,
	const FHexGridModel& GridModel,
	const FHexFeatureDefinition& FeatureDefinition,
	int32 Q,
	int32 R,
	int32 TileIndex,
	int32 RandomSeed
) const
{
	if (!MeshComponent || GridModel.GetHexRadius() <= 0.0f || !GridModel.IsValidTile(Q, R))
	{
		return;
	}

	const int32 MinCount = FMath::Max(1, FeatureDefinition.MinMeshInstancesPerTile);
	const int32 MaxCount = FMath::Max(MinCount, FeatureDefinition.MaxMeshInstancesPerTile);

	FRandomStream RandomStream(
		MakeFeatureVisualSeed(
			RandomSeed,
			TileIndex,
			FeatureDefinition.FeatureType
		)
	);

	const int32 SpawnedInstanceCount = RandomStream.RandRange(MinCount, MaxCount);
	const FVector FlatTileCenter = GridModel.GetHexCenter(Q, R);

	for (int32 InstanceIndex = 0; InstanceIndex < SpawnedInstanceCount; ++InstanceIndex)
	{
		const FVector FlatInstanceLocation = PickRandomPointInsideHex2D(
			FlatTileCenter,
			GridModel.GetHexRadius(),
			FeatureDefinition.ScatterRadiusRatio,
			RandomStream
		);

		const float SurfaceZ = GetTerrainSurfaceHeightAtTilePoint(
			GridModel,
			Q,
			R,
			FlatInstanceLocation
		);

		const FVector SurfaceLocation(
			FlatInstanceLocation.X,
			FlatInstanceLocation.Y,
			SurfaceZ
		);

		const FTransform InstanceTransform = BuildFeatureTransform(
			FeatureDefinition,
			SurfaceLocation,
			SpawnedInstanceCount,
			RandomStream
		);

		MeshComponent->AddInstance(InstanceTransform);
	}
}

FVector FHexFeatureMeshBuilder::PickRandomPointInsideHex2D(
	const FVector& FlatTileCenter,
	float HexRadius,
	float ScatterRadiusRatio,
	FRandomStream& RandomStream
) const
{
	const float ClampedScatterRatio = FMath::Clamp(ScatterRadiusRatio, 0.0f, 1.0f);

	if (ClampedScatterRatio <= KINDA_SMALL_NUMBER)
	{
		return FVector(FlatTileCenter.X, FlatTileCenter.Y, 0.0f);
	}

	const float SafeScatterRadius =
		HexRadius *
		FMath::Cos(FMath::DegreesToRadians(30.0f)) *
		ClampedScatterRatio;

	const float Angle = RandomStream.FRandRange(0.0f, TWO_PI);
	const float Radius = SafeScatterRadius * FMath::Sqrt(RandomStream.FRand());

	const float OffsetX = FMath::Cos(Angle) * Radius;
	const float OffsetY = FMath::Sin(Angle) * Radius;

	return FVector(
		FlatTileCenter.X + OffsetX,
		FlatTileCenter.Y + OffsetY,
		0.0f
	);
}

float FHexFeatureMeshBuilder::GetHeightForTerrainCorner(
	const FHexGridModel& GridModel,
	const FHexTileData& Tile,
	const FVector& FlatCorner
) const
{
	const float BaseCornerHeight = GridModel.UsesHeightOffsets()
		? GridModel.GetResolvedCornerHeight(FlatCorner)
		: Tile.Height;

	return BaseCornerHeight + GridModel.GetResolvedCornerHeightVarianceOffset(FlatCorner);
}

float FHexFeatureMeshBuilder::GetTerrainSurfaceHeightAtTilePoint(
	const FHexGridModel& GridModel,
	int32 Q,
	int32 R,
	const FVector& FlatPoint
) const
{
	const int32 TileIndex = GridModel.GetTileIndex(Q, R);
	const TArray<FHexTileData>& Tiles = GridModel.GetTiles();

	if (!Tiles.IsValidIndex(TileIndex))
	{
		return 0.0f;
	}

	const FHexTileData& Tile = Tiles[TileIndex];

	const FVector FlatCenter = GridModel.GetHexCenter(Q, R);
	const FVector2D LocalPoint(
		FlatPoint.X - FlatCenter.X,
		FlatPoint.Y - FlatCenter.Y
	);

	float AngleRadians = FMath::Atan2(LocalPoint.Y, LocalPoint.X);
	if (AngleRadians < 0.0f)
	{
		AngleRadians += TWO_PI;
	}

	const float FirstCornerAngleRadians = FMath::DegreesToRadians(30.0f);
	float AdjustedAngle = AngleRadians - FirstCornerAngleRadians;

	if (AdjustedAngle < 0.0f)
	{
		AdjustedAngle += TWO_PI;
	}

	const float SectorSizeRadians = TWO_PI / 6.0f;

	int32 CornerIndex = FMath::FloorToInt(AdjustedAngle / SectorSizeRadians);
	CornerIndex = FMath::Clamp(CornerIndex, 0, 5);

	const int32 NextCornerIndex = (CornerIndex + 1) % 6;

	const FVector FlatCornerA = FlatCenter + GridModel.GetHexCornerOffset(CornerIndex);
	const FVector FlatCornerB = FlatCenter + GridModel.GetHexCornerOffset(NextCornerIndex);

	const FVector A(FlatCenter.X, FlatCenter.Y, Tile.Height);
	const FVector B(
		FlatCornerA.X,
		FlatCornerA.Y,
		GetHeightForTerrainCorner(GridModel, Tile, FlatCornerA)
	);
	const FVector C(
		FlatCornerB.X,
		FlatCornerB.Y,
		GetHeightForTerrainCorner(GridModel, Tile, FlatCornerB)
	);

	const FVector2D A2D(A.X, A.Y);
	const FVector2D B2D(B.X, B.Y);
	const FVector2D C2D(C.X, C.Y);
	const FVector2D P2D(FlatPoint.X, FlatPoint.Y);

	const FVector2D V0 = B2D - A2D;
	const FVector2D V1 = C2D - A2D;
	const FVector2D V2 = P2D - A2D;

	const float D00 = FVector2D::DotProduct(V0, V0);
	const float D01 = FVector2D::DotProduct(V0, V1);
	const float D11 = FVector2D::DotProduct(V1, V1);
	const float D20 = FVector2D::DotProduct(V2, V0);
	const float D21 = FVector2D::DotProduct(V2, V1);

	const float Denom = D00 * D11 - D01 * D01;
	if (FMath::IsNearlyZero(Denom))
	{
		return Tile.Height;
	}

	const float V = (D11 * D20 - D01 * D21) / Denom;
	const float W = (D00 * D21 - D01 * D20) / Denom;
	const float U = 1.0f - V - W;

	return
		(U * A.Z) +
		(V * B.Z) +
		(W * C.Z);
}

FTransform FHexFeatureMeshBuilder::BuildFeatureTransform(
	const FHexFeatureDefinition& FeatureDefinition,
	const FVector& SurfaceLocation,
	int32 SpawnedInstanceCount,
	FRandomStream& RandomStream
) const
{
	const float YawVariation = FeatureDefinition.RandomYawVariationDegrees > 0.0f
		? RandomStream.FRandRange(
			-FeatureDefinition.RandomYawVariationDegrees,
			FeatureDefinition.RandomYawVariationDegrees
		)
		: 0.0f;

	const FRotator FinalRotation(
		FeatureDefinition.MeshRotation.Pitch,
		FeatureDefinition.MeshRotation.Yaw + YawVariation,
		FeatureDefinition.MeshRotation.Roll
	);

	float ScaleMultiplier = 1.0f;

	if (FeatureDefinition.bScaleDownByInstanceCount)
	{
		ScaleMultiplier = 1.0f / FMath::Sqrt(static_cast<float>(FMath::Max(1, SpawnedInstanceCount)));
	}

	const FVector FinalScale = FeatureDefinition.MeshScale * ScaleMultiplier;
	const FVector FinalLocation = SurfaceLocation + FeatureDefinition.MeshOffset;

	return FTransform(
		FinalRotation,
		FinalLocation,
		FinalScale
	);
}

int32 FHexFeatureMeshBuilder::MakeFeatureVisualSeed(
	int32 RandomSeed,
	int32 TileIndex,
	EHexFeatureType FeatureType
) const
{
	uint32 Hash = GetTypeHash(RandomSeed);
	Hash = HashCombine(Hash, GetTypeHash(TileIndex));
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(FeatureType)));

	return static_cast<int32>(Hash);
}