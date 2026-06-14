#pragma once

#include "CoreMinimal.h"
#include "ConquestGameSetupTypes.h"
#include "HexFeatureGenerator.h"
#include "HexFeatureMeshBuilder.h"
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
	void EnsureCityPlaceholderMeshComponent();
	FTransform BuildCityPlaceholderTransform(const FIntPoint& Coord) const;
	void AddCityPlaceholder(int32 CityId, const FIntPoint& Coord);
	void ClearCityPlaceholders();
	void ApplyGameSetupSettings(const FConquestGameSetupSettings& SetupSettings);

	// -----------------------------------------------------------------------------
	// Cities - temporary placeholder visuals
	// -----------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities")
	TObjectPtr<UStaticMesh> CityPlaceholderMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities")
	TObjectPtr<UMaterialInterface> CityPlaceholderMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities")
	FVector CityPlaceholderOffset = FVector(0.0f, 0.0f, 35.0f);

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities")
	FRotator CityPlaceholderRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities")
	FVector CityPlaceholderScale = FVector(0.5f, 0.5f, 0.5f);

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> CityPlaceholderMeshComponent = nullptr;

	UPROPERTY()
	TMap<int32, int32> CityIdToPlaceholderInstanceIndex;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hex Grid")
	void RebuildGrid();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void RegenerateGridWithNewRandomSeed();

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

	const FHexGridModel& GetGridModel() const
	{
		return GridModel;
	}

	FHexGridModel& GetMutableGridModel()
	{
		return GridModel;
	}

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

	void SetHoveredTile(int32 Q, int32 R);
	void ClearHoveredTile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> HoverHighlightMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Hover")
	TObjectPtr<UMaterialInterface> HoverHighlightMaterial = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Hover")
	float HoverEdgeWidth = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Hover")
	float HoverVertexRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Hover")
	float HoverSurfaceOffset = 8.0f;
	
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

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Features")
	FHexFeatureGenerationSettings FeatureGenerationSettings;
	
	FHexFeatureGenerator FeatureGenerator;
	FHexFeatureMeshBuilder FeatureMeshBuilder;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> FeatureMeshComponents;

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

	int32 HoveredQ = INDEX_NONE;
	int32 HoveredR = INDEX_NONE;

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
