#pragma once

#include "CoreMinimal.h"
#include "ConquestGameSetupTypes.h"
#include "GameFramework/Actor.h"
#include "HexGridModel.h"
#include "HexMapGenerator.h"
#include "HexResourceGenerator.h"
#include "HexMeshBuilder.h"
#include "HexRiverGenerator.h"
#include "HexTileResourceMeshBuilder.h"
#include "ModularHexGridActor.generated.h"

class USceneComponent;
class UProceduralMeshComponent;
class UHexTileResourceData;
class UHexResourceSetData;
class UHexImprovementSetData;

UCLASS()
class CONQUEST_API AModularHexGridActor : public AActor
{
	GENERATED_BODY()

public:
	AModularHexGridActor();
	void ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hex Grid")
	void RebuildGrid();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Map Type")
	void SetMapTypePreset(EHexMapTypePreset MapTypePreset);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Overlay")
	void SetHexGridOverlayVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Water")
	void SetWaterLayerVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Rivers")
	void SetRiverLayerVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	const TArray<FHexTileData>& GetTiles() const { return GridModel.GetTiles(); }

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Improvements")
	void GetPossibleImprovementIdsForTile(int32 Q, int32 R, TArray<FName>& OutImprovementIds) const;

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Improvements")
	bool SetTileImprovement(int32 Q, int32 R, FName ImprovementId);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Hover")
	bool GetTileAtWorldLocation(
		const FVector& WorldLocation,
		int32& OutQ,
		int32& OutR,
		FHexTileData& OutTileData
		) const;

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Fog Of War")
	void SetFogOfWarVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Hex Grid|Overlay")
	bool IsHexGridOverlayVisible() const { return OverlaySettings.bShowHexGridOverlay; }

	UFUNCTION(BlueprintPure, Category = "Hex Grid|Fog Of War")
	bool IsFogOfWarVisible() const { return bGenerateFogOfWar; }
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	bool bGenerateOnBeginPlay = true;

	// Terrain, features, base yields, height offsets, and terrain materials.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Data")
	TObjectPtr<UHexTileResourceData> TileResourceData = nullptr;

	// Bonus/luxury/strategic resources. This is where Wheat, Silk, Iron, etc. are authored.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Data")
	TObjectPtr<UHexResourceSetData> ResourceSetData = nullptr;

	// Farms, mines, plantations, pastures, camps, fishing boats, etc.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Data")
	TObjectPtr<UHexImprovementSetData> ImprovementSetData = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Size")
	FHexGridSizeSettings SizeSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Height")
	FHexHeightSettings HeightSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation")
	FHexGenerationSettings GenerationSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Resources")
	FHexResourceGenerationSettings ResourceGenerationSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Rivers")
	FHexRiverGenerationSettings RiverGenerationSettings;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Rivers")
	TObjectPtr<UProceduralMeshComponent> RiverMesh = nullptr;
	
	FHexRiverGenerator RiverGenerator;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Water")
	FHexWaterSettings WaterSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Overlay")
	FHexOverlaySettings OverlaySettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Fog Of War")
	bool bGenerateFogOfWar = true;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Fog Of War")
	FHexFogOfWarSettings FogOfWarSettings;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Fog Of War")
	TObjectPtr<UProceduralMeshComponent> FogOfWarMesh = nullptr;

	

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid")
	TObjectPtr<UProceduralMeshComponent> GridMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Water")
	TObjectPtr<UProceduralMeshComponent> WaterMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Overlay")
	TObjectPtr<UProceduralMeshComponent> HexGridOverlayMesh = nullptr;

	FHexGridModel GridModel;
	FHexMapGenerator Generator;
	FHexResourceGenerator ResourceGenerator;
	FHexMeshBuilder MeshBuilder;
	
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> ResourceMeshComponents;

	FHexTileResourceMeshBuilder ResourceMeshBuilder;

	void EnsureDefaultGenerationRules();
	void ConfigureMeshComponents();
};
