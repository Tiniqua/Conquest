#pragma once

#include "CoreMinimal.h"

enum class EHexFeatureType : uint8;
struct FHexTileData;
class AActor;
class USceneComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UHexTileResourceData;
class FHexGridModel;

struct FHexFeatureDefinition;

class FHexFeatureMeshBuilder
{
public:
	void BuildFeatureMeshes(
		AActor* Owner,
		USceneComponent* AttachParent,
		const FHexGridModel& GridModel,
		const UHexTileResourceData* TileData,
		int32 RandomSeed,
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutFeatureMeshComponents
	);

	void ClearFeatureMeshes(
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutFeatureMeshComponents
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
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutFeatureMeshComponents
	);

	void AddFeatureInstancesForTile(
		UInstancedStaticMeshComponent* MeshComponent,
		const FHexGridModel& GridModel,
		const FHexFeatureDefinition& FeatureDefinition,
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

	FTransform BuildFeatureTransform(
		const FHexFeatureDefinition& FeatureDefinition,
		const FVector& SurfaceLocation,
		int32 SpawnedInstanceCount,
		FRandomStream& RandomStream
	) const;

	int32 MakeFeatureVisualSeed(
		int32 RandomSeed,
		int32 TileIndex,
		EHexFeatureType FeatureType
	) const;
};