#include "HexImprovementMeshBuilder.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

#include "HexGridModel.h"
#include "HexImprovementTypes.h"
#include "HexTileTypes.h"

void FHexImprovementMeshBuilder::BuildImprovementMeshes(
	AActor* Owner,
	USceneComponent* AttachParent,
	const FHexGridModel& GridModel,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutImprovementMeshComponents
)
{
	ClearImprovementMeshes(InOutImprovementMeshComponents);

	if (!Owner || !AttachParent)
	{
		return;
	}

	TMap<FMeshComponentKey, UInstancedStaticMeshComponent*> MeshToComponent;
	const TArray<FHexTileData>& Tiles = GridModel.GetTiles();

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		const FHexTileData& Tile = Tiles[TileIndex];
		if (Tile.ImprovementId.IsNone())
		{
			continue;
		}

		const FHexImprovementDefinition* ImprovementDefinition =
			GridModel.FindImprovementDefinition(Tile.ImprovementId);

		if (!ImprovementDefinition || !ImprovementDefinition->WorldMesh)
		{
			continue;
		}

		int32 Q = 0;
		int32 R = 0;
		if (!GridModel.GetCoordFromIndex(TileIndex, Q, R))
		{
			continue;
		}

		UMaterialInterface* EffectiveMaterialOverride = ImprovementDefinition->WorldMeshMaterialOverride
			? ImprovementDefinition->WorldMeshMaterialOverride.Get()
			: ImprovementDefinition->IconMaterial.Get();

		UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshComponent(
			Owner,
			AttachParent,
			ImprovementDefinition->WorldMesh,
			EffectiveMaterialOverride,
			MeshToComponent,
			InOutImprovementMeshComponents
		);

		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->AddInstance(BuildImprovementTransform(
			GridModel,
			*ImprovementDefinition,
			Q,
			R
		));
	}

	for (TObjectPtr<UInstancedStaticMeshComponent>& MeshComponent : InOutImprovementMeshComponents)
	{
		if (MeshComponent)
		{
			MeshComponent->MarkRenderStateDirty();
		}
	}
}

void FHexImprovementMeshBuilder::ClearImprovementMeshes(
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutImprovementMeshComponents
)
{
	for (TObjectPtr<UInstancedStaticMeshComponent>& MeshComponent : InOutImprovementMeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->ClearInstances();
		MeshComponent->DestroyComponent();
	}

	InOutImprovementMeshComponents.Reset();
}

UInstancedStaticMeshComponent* FHexImprovementMeshBuilder::FindOrCreateMeshComponent(
	AActor* Owner,
	USceneComponent* AttachParent,
	UStaticMesh* Mesh,
	UMaterialInterface* MaterialOverride,
	TMap<FMeshComponentKey, UInstancedStaticMeshComponent*>& MeshToComponent,
	TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutImprovementMeshComponents
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
			TEXT("ImprovementMesh_%s_%s"),
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
		const int32 MaterialSlotCount = FMath::Max(1, Mesh->GetStaticMaterials().Num());

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
	InOutImprovementMeshComponents.Add(NewComponent);

	return NewComponent;
}

FTransform FHexImprovementMeshBuilder::BuildImprovementTransform(
	const FHexGridModel& GridModel,
	const FHexImprovementDefinition& ImprovementDefinition,
	int32 Q,
	int32 R
) const
{
	const FVector FlatCenter = GridModel.GetHexCenter(Q, R);
	float SurfaceZ = 0.0f;

	if (const FHexTileData* Tile = GridModel.GetTile(Q, R))
	{
		SurfaceZ = Tile->Height;
	}

	const FVector SurfaceLocation(FlatCenter.X, FlatCenter.Y, SurfaceZ);

	return FTransform(
		ImprovementDefinition.MeshRotation,
		SurfaceLocation + ImprovementDefinition.MeshOffset,
		ImprovementDefinition.MeshScale
	);
}
