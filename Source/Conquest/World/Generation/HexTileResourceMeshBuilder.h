#pragma once

#include "CoreMinimal.h"

class FHexGridModel;
class AActor;
class USceneComponent;
class UInstancedStaticMeshComponent;
class UHexResourceSetData;
class UStaticMesh;



class CONQUEST_API FHexTileResourceMeshBuilder
{
public:
	void BuildResourceMeshes(
		AActor* Owner,
		USceneComponent* AttachParent,
		const FHexGridModel& GridModel,
		const UHexResourceSetData* ResourceSetData,
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
	);

	void ClearResourceMeshes(
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
	);

private:
	UInstancedStaticMeshComponent* FindOrCreateMeshComponent(
		AActor* Owner,
		USceneComponent* AttachParent,
		UStaticMesh* Mesh,
		TMap<UStaticMesh*, UInstancedStaticMeshComponent*>& MeshToComponent,
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
	);

	FTransform BuildResourceTransform(
		const FVector& TileCenter,
		const FRotator& MeshRotation,
		const FVector& MeshOffset,
		const FVector& MeshScale
	) const;
};