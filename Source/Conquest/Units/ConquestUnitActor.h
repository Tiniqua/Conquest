#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Units/ConquestUnitTypes.h"
#include "ConquestUnitActor.generated.h"

class AModularHexGridActor;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

UCLASS()
class CONQUEST_API AConquestUnitActor : public AActor
{
	GENERATED_BODY()

public:
	AConquestUnitActor();

	void InitializeUnit(
		const FConquestUnitState& InUnitState,
		const FConquestUnitRow& InUnitRow,
		AModularHexGridActor* InGridActor
	);

	void RefreshUnitVisuals(const FConquestUnitState& InUnitState, const FConquestUnitRow& InUnitRow);
	void SetSelected(bool bSelected, UMaterialInterface* SelectionMaterial = nullptr);
	void MoveToTile(const FIntPoint& NewCoord);

	UFUNCTION(BlueprintPure, Category = "Conquest|Unit")
	int32 GetUnitInstanceId() const { return UnitInstanceId; }

	UFUNCTION(BlueprintPure, Category = "Conquest|Unit")
	int32 GetOwnerPlayerId() const { return OwnerPlayerId; }

	UFUNCTION(BlueprintPure, Category = "Conquest|Unit")
	FIntPoint GetTileCoord() const { return TileCoord; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> UnitMeshInstances = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> SelectionMesh = nullptr;

private:
	UPROPERTY()
	TObjectPtr<AModularHexGridActor> GridActor = nullptr;

	int32 UnitInstanceId = INDEX_NONE;
	int32 OwnerPlayerId = INDEX_NONE;
	FIntPoint TileCoord = FIntPoint::ZeroValue;
	int32 CurrentHealth = 0;
	int32 MaxHealth = 1;
	int32 UnitMeshCount = 1;
	float UnitMeshSpacing = 60.0f;
	FVector UnitMeshScale = FVector::OneVector;
	FVector UnitMeshOffset = FVector(0.0f, 0.0f, 8.0f);
	FRotator UnitMeshRotation = FRotator::ZeroRotator;

	void RebuildMeshInstances();
	void RebuildSelectionMesh(UMaterialInterface* SelectionMaterial);
	void ClearSelectionMesh();

	void UpdateActorTransformForTile();
	FVector GetTileLocalCenter() const;
	FVector ProjectLocalPointToTerrain(const FVector& LocalPoint) const;
	FVector GridLocalToActorLocal(const FVector& GridLocalPoint) const;
	TArray<FVector> BuildFormationOffsets(int32 VisibleMeshCount) const;
};
