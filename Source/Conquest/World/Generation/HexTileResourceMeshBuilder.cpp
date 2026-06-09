#include "HexTileResourceMeshBuilder.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

#include "HexGridModel.h"
#include "HexResourceSetData.h"
#include "HexResourceTypes.h"
#include "HexTileTypes.h"

void FHexTileResourceMeshBuilder::BuildResourceMeshes(
	AActor* Owner,
	USceneComponent* AttachParent,
	const FHexGridModel& GridModel,
	const UHexResourceSetData* ResourceSetData,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
)
{
	ClearResourceMeshes(InOutResourceMeshComponents);

	if (!Owner || !AttachParent || !ResourceSetData)
	{
		return;
	}

	TMap<UStaticMesh*, UInstancedStaticMeshComponent*> MeshToComponent;

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

		// Important:
		// Use Tile.Height, not Z = 0 and not WaterSurfaceZ.
		// This means fish can sit underwater and mountain/mining resources sit high.
		const FVector TileCenter(
			FlatCenter.X,
			FlatCenter.Y,
			Tile.Height
		);

		UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshComponent(
			Owner,
			AttachParent,
			ResourceMesh,
			MeshToComponent,
			InOutResourceMeshComponents
		);

		if (!MeshComponent)
		{
			continue;
		}

		const FTransform InstanceTransform = BuildResourceTransform(
			TileCenter,
			ResourceDefinition->MeshRotation,
			ResourceDefinition->MeshOffset,
			ResourceDefinition->MeshScale
		);

		MeshComponent->AddInstance(InstanceTransform);
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
	TMap<UStaticMesh*, UInstancedStaticMeshComponent*>& MeshToComponent,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
)
{
	if (!Owner || !AttachParent || !Mesh)
	{
		return nullptr;
	}

	if (UInstancedStaticMeshComponent** ExistingComponent = MeshToComponent.Find(Mesh))
	{
		return *ExistingComponent;
	}

	const FName ComponentName = MakeUniqueObjectName(
		Owner,
		UInstancedStaticMeshComponent::StaticClass(),
		FName(*FString::Printf(TEXT("ResourceMesh_%s"), *Mesh->GetName()))
	);

	UInstancedStaticMeshComponent* NewComponent =
		NewObject<UInstancedStaticMeshComponent>(Owner, ComponentName);

	if (!NewComponent)
	{
		return nullptr;
	}

	NewComponent->SetStaticMesh(Mesh);
	NewComponent->SetupAttachment(AttachParent);
	NewComponent->RegisterComponent();

	NewComponent->SetMobility(EComponentMobility::Static);
	NewComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	NewComponent->SetGenerateOverlapEvents(false);

	NewComponent->bCastDynamicShadow = true;
	NewComponent->bCastStaticShadow = true;
	NewComponent->CastShadow = true;

	Owner->AddInstanceComponent(NewComponent);

	MeshToComponent.Add(Mesh, NewComponent);
	InOutResourceMeshComponents.Add(NewComponent);

	return NewComponent;
}

FTransform FHexTileResourceMeshBuilder::BuildResourceTransform(
	const FVector& TileCenter,
	const FRotator& MeshRotation,
	const FVector& MeshOffset,
	const FVector& MeshScale
) const
{
	const FVector FinalLocation = TileCenter + MeshOffset;

	return FTransform(
		MeshRotation,
		FinalLocation,
		MeshScale
	);
}