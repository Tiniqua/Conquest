#pragma once

#include "CoreMinimal.h"

struct FHexTileData;
class FHexGridModel;
class AActor;
class USceneComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UHexResourceSetData;
class UStaticMesh;

struct FHexResourceDefinition;

class CONQUEST_API FHexTileResourceMeshBuilder
{
public:
	void BuildResourceMeshes(
		AActor* Owner,
		USceneComponent* AttachParent,
		const FHexGridModel& GridModel,
		const UHexResourceSetData* ResourceSetData,
		int32 RandomSeed,
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
	);

	void ClearResourceMeshes(
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
	);

private:
	struct FMeshComponentKey
	{
		TObjectPtr<UStaticMesh> Mesh = nullptr;
		TObjectPtr<UMaterialInterface> MaterialOverride = nullptr;

		bool operator==(const FMeshComponentKey& Other) const
		{
			return Mesh == Other.Mesh && MaterialOverride == Other.MaterialOverride;
		}
	};

	friend uint32 GetTypeHash(const FMeshComponentKey& Key)
	{
		uint32 Hash = GetTypeHash(Key.Mesh);
		Hash = HashCombine(Hash, GetTypeHash(Key.MaterialOverride));
		return Hash;
	}

	UInstancedStaticMeshComponent* FindOrCreateMeshComponent(
		AActor* Owner,
		USceneComponent* AttachParent,
		UStaticMesh* Mesh,
		UMaterialInterface* MaterialOverride,
		TMap<FMeshComponentKey, UInstancedStaticMeshComponent*>& MeshToComponent,
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutResourceMeshComponents
	);

	void AddResourceInstancesForTile(
	UInstancedStaticMeshComponent* MeshComponent,
	const FHexGridModel& GridModel,
	const FHexResourceDefinition& ResourceDefinition,
	int32 Q,
	int32 R,
	int32 TileIndex,
	int32 RandomSeed
) const;

	FVector PickRandomPointInsideHex2D(
		const FVector& FlatTileCenter,
		float HexRadius,
		float ScatterRadiusRatio,
		FRandomStream& RandomStream
	) const;

	float GetTerrainSurfaceHeightAtTilePoint(
		const FHexGridModel& GridModel,
		int32 Q,
		int32 R,
		const FVector& FlatPoint
	) const;

	float GetHeightForTerrainCorner(
		const FHexGridModel& GridModel,
		const FHexTileData& Tile,
		const FVector& FlatCorner
	) const;

	FTransform BuildResourceTransform(
		const FHexResourceDefinition& ResourceDefinition,
		const FVector& SurfaceLocation,
		int32 SpawnedInstanceCount,
		FRandomStream& RandomStream
	) const;
	int32 MakeResourceVisualSeed(int32 RandomSeed, int32 TileIndex, FName ResourceId) const;
};