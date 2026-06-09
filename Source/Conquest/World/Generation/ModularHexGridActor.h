#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexGridModel.h"
#include "HexMapGenerator.h"
#include "HexMeshBuilder.h"
#include "ModularHexGridActor.generated.h"

class USceneComponent;
class UProceduralMeshComponent;
class UHexTileResourceData;

UCLASS()
class CONQUEST_API AModularHexGridActor : public AActor
{
	GENERATED_BODY()

public:
	AModularHexGridActor();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hex Grid")
	void RebuildGrid();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Overlay")
	void SetHexGridOverlayVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Water")
	void SetWaterLayerVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	const TArray<FHexTileData>& GetTiles() const { return GridModel.GetTiles(); }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Data")
	UHexTileResourceData* TileResourceData = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Size")
	FHexGridSizeSettings SizeSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Height")
	FHexHeightSettings HeightSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation")
	FHexGenerationSettings GenerationSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Water")
	FHexWaterSettings WaterSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Overlay")
	FHexOverlaySettings OverlaySettings;

	UPROPERTY()
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid")
	UProceduralMeshComponent* GridMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Water")
	UProceduralMeshComponent* WaterMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Overlay")
	UProceduralMeshComponent* HexGridOverlayMesh = nullptr;

	FHexGridModel GridModel;
	FHexMapGenerator Generator;
	FHexMeshBuilder MeshBuilder;

	void EnsureDefaultGenerationRules();
	void ConfigureMeshComponents();
};
