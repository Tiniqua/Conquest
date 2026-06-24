#pragma once

#include "CoreMinimal.h"

struct FHexImprovementDefinition;
class AActor;
class FHexGridModel;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;

class CONQUEST_API FHexImprovementMeshBuilder
{
public:
	void BuildImprovementMeshes(
		AActor* Owner,
		USceneComponent* AttachParent,
		const FHexGridModel& GridModel,
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutImprovementMeshComponents
	);

	void ClearImprovementMeshes(
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutImprovementMeshComponents
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
		TArray<TObjectPtr<UInstancedStaticMeshComponent>>& InOutImprovementMeshComponents
	);

	FTransform BuildImprovementTransform(
		const FHexGridModel& GridModel,
		const FHexImprovementDefinition& ImprovementDefinition,
		int32 Q,
		int32 R
	) const;
};
