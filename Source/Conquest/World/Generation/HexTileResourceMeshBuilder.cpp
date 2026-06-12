#include "HexTileResourceMeshBuilder.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

#include "HexGridModel.h"
#include "HexResourceSetData.h"
#include "HexResourceTypes.h"
#include "HexTileTypes.h"

void FHexTileResourceMeshBuilder::BuildResourceMeshes(
	AActor* Owner,
	USceneComponent* AttachParent,
	const FHexGridModel& GridModel,
	const UHexResourceSetData* ResourceSetData,
	int32 RandomSeed,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
)
{
	ClearResourceMeshes(InOutResourceMeshComponents);

	if (!Owner || !AttachParent || !ResourceSetData)
	{
		return;
	}

	TMap<FMeshComponentKey, UInstancedStaticMeshComponent*> MeshToComponent;

	const TArray<FHexTileData>& Tiles = GridModel.GetTiles();

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		const FHexTileData& Tile = Tiles[TileIndex];

		if (!Tile.Resource.HasResource())
		{
			continue;
		}

		const FHexResourceDefinition* ResourceDefinition =
			ResourceSetData->FindResource(Tile.Resource.ResourceId);

		if (!ResourceDefinition)
		{
			continue;
		}

		UStaticMesh* ResourceMesh = ResourceDefinition->WorldMesh;
		if (!ResourceMesh)
		{
			continue;
		}

		int32 Q = 0;
		int32 R = 0;
		if (!GridModel.GetCoordFromIndex(TileIndex, Q, R))
		{
			continue;
		}

		UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshComponent(
			Owner,
			AttachParent,
			ResourceMesh,
			ResourceDefinition->WorldMeshMaterialOverride,
			MeshToComponent,
			InOutResourceMeshComponents
		);

		if (!MeshComponent)
		{
			continue;
		}

		AddResourceInstancesForTile(
			MeshComponent,
			GridModel,
			*ResourceDefinition,
			Q,
			R,
			TileIndex,
			RandomSeed
		);
	}

	for (TObjectPtr<UInstancedStaticMeshComponent>& MeshComponent : InOutResourceMeshComponents)
	{
		if (MeshComponent)
		{
			MeshComponent->MarkRenderStateDirty();
		}
	}
}

void FHexTileResourceMeshBuilder::ClearResourceMeshes(
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
)
{
	for (TObjectPtr<UInstancedStaticMeshComponent>& MeshComponent : InOutResourceMeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->ClearInstances();
		MeshComponent->DestroyComponent();
	}

	InOutResourceMeshComponents.Reset();
}

UInstancedStaticMeshComponent* FHexTileResourceMeshBuilder::FindOrCreateMeshComponent(
	AActor* Owner,
	USceneComponent* AttachParent,
	UStaticMesh* Mesh,
	UMaterialInterface* MaterialOverride,
	TMap<FMeshComponentKey, UInstancedStaticMeshComponent*>& MeshToComponent,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
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
			TEXT("ResourceMesh_%s_%s"),
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
	InOutResourceMeshComponents.Add(NewComponent);

	return NewComponent;
}

void FHexTileResourceMeshBuilder::AddResourceInstancesForTile(
	UInstancedStaticMeshComponent* MeshComponent,
	const FHexGridModel& GridModel,
	const FHexResourceDefinition& ResourceDefinition,
	int32 Q,
	int32 R,
	int32 TileIndex,
	int32 RandomSeed
) const
{
	if (!MeshComponent || GridModel.GetHexRadius() <= 0.0f)
	{
		return;
	}

	if (!GridModel.IsValidTile(Q, R))
	{
		return;
	}

	const int32 MinCount = FMath::Max(1, ResourceDefinition.MinMeshInstancesPerTile);
	const int32 MaxCount = FMath::Max(MinCount, ResourceDefinition.MaxMeshInstancesPerTile);

	FRandomStream RandomStream(
		MakeResourceVisualSeed(
			RandomSeed,
			TileIndex,
			ResourceDefinition.ResourceId
		)
	);

	const int32 SpawnedInstanceCount = RandomStream.RandRange(MinCount, MaxCount);
	const FVector FlatTileCenter = GridModel.GetHexCenter(Q, R);

	for (int32 InstanceIndex = 0; InstanceIndex < SpawnedInstanceCount; ++InstanceIndex)
	{
		const FVector FlatInstanceLocation = PickRandomPointInsideHex2D(
			FlatTileCenter,
			GridModel.GetHexRadius(),
			ResourceDefinition.ScatterRadiusRatio,
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

		const FTransform InstanceTransform = BuildResourceTransform(
			ResourceDefinition,
			SurfaceLocation,
			SpawnedInstanceCount,
			RandomStream
		);

		MeshComponent->AddInstance(InstanceTransform);
	}
}

FVector FHexTileResourceMeshBuilder::PickRandomPointInsideHex2D(
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

float FHexTileResourceMeshBuilder::GetHeightForTerrainCorner(
	const FHexGridModel& GridModel,
	const FHexTileData& Tile,
	const FVector& FlatCorner
) const
{
	const float BaseCornerHeight = GridModel.UsesHeightOffsets()
		? GridModel.GetResolvedCornerHeight(FlatCorner)
		: Tile.Height;

	// If you added the shared vertex variance fix, use it here too.
	// This keeps resources aligned to the actual rendered terrain surface.
	const float SharedVarianceOffset =
		GridModel.GetResolvedCornerHeightVarianceOffset(FlatCorner);

	return BaseCornerHeight + SharedVarianceOffset;
}

float FHexTileResourceMeshBuilder::GetTerrainSurfaceHeightAtTilePoint(
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

	const float SectorSizeRadians = TWO_PI / 6.0f;

	// This assumes GetHexCornerOffset(0) starts at angle 0 and corners progress around the hex.
	int32 CornerIndex = FMath::FloorToInt(AngleRadians / SectorSizeRadians);
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

	const FVector P(FlatPoint.X, FlatPoint.Y, 0.0f);

	const FVector2D A2D(A.X, A.Y);
	const FVector2D B2D(B.X, B.Y);
	const FVector2D C2D(C.X, C.Y);
	const FVector2D P2D(P.X, P.Y);

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

FTransform FHexTileResourceMeshBuilder::BuildResourceTransform(
	const FHexResourceDefinition& ResourceDefinition,
	const FVector& InstanceLocation,
	int32 SpawnedInstanceCount,
	FRandomStream& RandomStream
) const
{
	const float YawVariation = ResourceDefinition.RandomYawVariationDegrees > 0.0f
		? RandomStream.FRandRange(
			-ResourceDefinition.RandomYawVariationDegrees,
			ResourceDefinition.RandomYawVariationDegrees
		)
		: 0.0f;

	// Only yaw changes. Pitch/Roll remain exactly as defined in the data asset.
	const FRotator FinalRotation(
		ResourceDefinition.MeshRotation.Pitch,
		ResourceDefinition.MeshRotation.Yaw + YawVariation,
		ResourceDefinition.MeshRotation.Roll
	);

	float ScaleMultiplier = 1.0f;

	if (ResourceDefinition.bScaleDownByInstanceCount)
	{
		ScaleMultiplier = 1.0f / FMath::Sqrt(static_cast<float>(FMath::Max(1, SpawnedInstanceCount)));
	}

	const FVector FinalScale = ResourceDefinition.MeshScale * ScaleMultiplier;
	const FVector FinalLocation = InstanceLocation + ResourceDefinition.MeshOffset;

	return FTransform(
		FinalRotation,
		FinalLocation,
		FinalScale
	);
}

int32 FHexTileResourceMeshBuilder::MakeResourceVisualSeed(
	int32 RandomSeed,
	int32 TileIndex,
	FName ResourceId
) const
{
	uint32 Hash = GetTypeHash(RandomSeed);
	Hash = HashCombine(Hash, GetTypeHash(TileIndex));
	Hash = HashCombine(Hash, GetTypeHash(ResourceId));

	return static_cast<int32>(Hash);
}