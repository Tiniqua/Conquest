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

		const FVector FlatCenter = GridModel.GetHexCenter(Q, R);

		// Use the generated tile height, not world Z = 0.
		// This keeps sea resources low and mountain/mining resources high.
		const FVector TileCenter(
			FlatCenter.X,
			FlatCenter.Y,
			Tile.Height
		);

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
			*ResourceDefinition,
			TileCenter,
			GridModel.GetHexRadius(),
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
	const FHexResourceDefinition& ResourceDefinition,
	const FVector& TileCenter,
	float HexRadius,
	int32 TileIndex,
	int32 RandomSeed
) const
{
	if (!MeshComponent || HexRadius <= 0.0f)
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

	for (int32 InstanceIndex = 0; InstanceIndex < SpawnedInstanceCount; ++InstanceIndex)
	{
		const FVector InstanceLocation = PickRandomPointInsideHex(
			TileCenter,
			HexRadius,
			ResourceDefinition.ScatterRadiusRatio,
			RandomStream
		);

		const FTransform InstanceTransform = BuildResourceTransform(
			ResourceDefinition,
			InstanceLocation,
			SpawnedInstanceCount,
			RandomStream
		);

		MeshComponent->AddInstance(InstanceTransform);
	}
}

FVector FHexTileResourceMeshBuilder::PickRandomPointInsideHex(
	const FVector& TileCenter,
	float HexRadius,
	float ScatterRadiusRatio,
	FRandomStream& RandomStream
) const
{
	const float ClampedScatterRatio = FMath::Clamp(ScatterRadiusRatio, 0.0f, 1.0f);

	if (ClampedScatterRatio <= KINDA_SMALL_NUMBER)
	{
		return TileCenter;
	}

	// For a flat-top hex with corner radius HexRadius, the apothem is:
	// cos(30) * HexRadius.
	// Keeping scatter within the apothem is a safe way to remain inside the hex.
	const float SafeScatterRadius =
		HexRadius *
		FMath::Cos(FMath::DegreesToRadians(30.0f)) *
		ClampedScatterRatio;

	// sqrt random radius gives an even distribution over the disc.
	const float Angle = RandomStream.FRandRange(0.0f, TWO_PI);
	const float Radius = SafeScatterRadius * FMath::Sqrt(RandomStream.FRand());

	const float OffsetX = FMath::Cos(Angle) * Radius;
	const float OffsetY = FMath::Sin(Angle) * Radius;

	return FVector(
		TileCenter.X + OffsetX,
		TileCenter.Y + OffsetY,
		TileCenter.Z
	);
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