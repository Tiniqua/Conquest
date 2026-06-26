#pragma once

#include "CoreMinimal.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "Conquest/UI/ConquestUnitWorldIconWidget.h"
#include "ConquestGameSetupTypes.h"
#include "HexFeatureGenerator.h"
#include "HexFeatureMeshBuilder.h"
#include "HexImprovementMeshBuilder.h"
#include "GameFramework/Actor.h"
#include "HexGridModel.h"
#include "HexMapGenerator.h"
#include "HexResourceGenerator.h"
#include "HexMeshBuilder.h"
#include "HexSimpleRiverGenerator.h"
#include "HexTileResourceMeshBuilder.h"
#include "ModularHexGridActor.generated.h"

class USceneComponent;
class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UWidgetComponent;
class UConquestCityWorldLabelWidget;
class UConquestTileHealthBarWidget;
class UHexTileResourceData;
class UHexResourceSetData;
class UDataTable;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTileYieldLensTransitionFinished, EConquestYieldType, YieldType);

UCLASS()
class CONQUEST_API AModularHexGridActor : public AActor
{
	GENERATED_BODY()

public:
	AModularHexGridActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UInstancedStaticMeshComponent* EnsureCityPlaceholderMeshComponent(UStaticMesh* OverrideMesh = nullptr, UMaterialInterface* OverrideMaterial = nullptr);
	FTransform BuildCityPlaceholderTransform(const FIntPoint& Coord, bool bOverrideScale = false, const FVector& OverrideScale = FVector::OneVector) const;
	FTransform BuildCityWorldLabelTransform(const FIntPoint& Coord) const;
	FTransform BuildTileHealthBarTransform(const FIntPoint& Coord) const;
	void AddCityPlaceholder(
		int32 CityId,
		const FIntPoint& Coord,
		UStaticMesh* OverrideMesh = nullptr,
		UMaterialInterface* OverrideMaterial = nullptr,
		bool bOverrideScale = false,
		const FVector& OverrideScale = FVector::OneVector
	);
	void AddOrUpdateCityWorldLabel(
		int32 CityId,
		const FIntPoint& Coord,
		FName CityName,
		int32 Population,
		int32 CurrentHealth,
		int32 MaxHealth,
		int32 Strength,
		float GrowthPercent,
		UMaterialInterface* CivilisationThemeMaterial = nullptr,
		FLinearColor CivilisationThemeColor = FLinearColor::White
	);
	void UpdateCityWorldLabel(
		int32 CityId,
		FName CityName,
		int32 Population,
		int32 CurrentHealth,
		int32 MaxHealth,
		int32 Strength,
		float GrowthPercent,
		UMaterialInterface* CivilisationThemeMaterial = nullptr,
		FLinearColor CivilisationThemeColor = FLinearColor::White
	);
	void SetCityWorldLabelVisible(int32 CityId, bool bVisible);
	void AddOrUpdateTileHealthBar(const FIntPoint& Coord, int32 CurrentHealth, int32 MaxHealth, int32 CombatStrength);
	void RemoveTileHealthBar(const FIntPoint& Coord);
	void ClearTileHealthBars();
	void ClearCityPlaceholders();
	void ClearCivilisationBorders();
	void RebuildCivilisationBorders(int32 OwnerPlayerId, UMaterialInterface* BorderMaterial, UMaterialInterface* BorderFillMaterial = nullptr);
	void RebuildCivilisationBordersForTiles(const TArray<FIntPoint>& OwnedTiles, UMaterialInterface* BorderMaterial, UMaterialInterface* BorderFillMaterial = nullptr);
	void RebuildExpansionCandidateHighlights(const TArray<FIntPoint>& CandidateCoords, UMaterialInterface* HighlightMaterial = nullptr);
	void ClearExpansionCandidateHighlights();
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

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities")
	float CityPlaceholderGroundTraceHeight = 10000.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities|World Label")
	TSubclassOf<UConquestCityWorldLabelWidget> CityWorldLabelWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities|World Label")
	FVector CityWorldLabelOffset = FVector(0.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities|World Label")
	FRotator CityWorldLabelRotation = FRotator(60.0f, 0.0f, 90.0f);

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities|World Label")
	FVector2D CityWorldLabelDrawSize = FVector2D(240.0f, 80.0f);

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Cities|World Label")
	float CityWorldLabelScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Tiles|Health Bar")
	TSubclassOf<UConquestTileHealthBarWidget> TileHealthBarWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Tiles|Health Bar")
	FVector TileHealthBarOffset = FVector(0.0f, 0.0f, 72.0f);

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Tiles|Health Bar")
	FRotator TileHealthBarRotation = FRotator(60.0f, 0.0f, 90.0f);

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Tiles|Health Bar")
	FVector2D TileHealthBarDrawSize = FVector2D(180.0f, 36.0f);

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Tiles|Health Bar")
	float TileHealthBarScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Units|World Icon")
	TSubclassOf<UConquestUnitWorldIconWidget> UnitWorldIconWidgetClass;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> CityPlaceholderMeshComponent = nullptr;

	UPROPERTY()
	TMap<int32, int32> CityIdToPlaceholderInstanceIndex;

	UPROPERTY()
	TMap<int32, int32> CityIdToPlaceholderVisualKey;

	UPROPERTY()
	TMap<int32, TObjectPtr<UInstancedStaticMeshComponent>> CityPlaceholderMeshComponentsByVisualKey;

	UPROPERTY()
	TMap<int32, TObjectPtr<UWidgetComponent>> CityWorldLabelComponents;

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<UWidgetComponent>> TileHealthBarComponents;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hex Grid")
	void RebuildGrid();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void RegenerateGridWithNewRandomSeed();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Map Type")
	void SetMapTypePreset(EHexMapTypePreset MapTypePreset);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Overlay")
	void SetHexGridOverlayVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Yields")
	void SetTileYieldLens(EConquestYieldType YieldType);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Yields")
	void ClearTileYieldLens();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Yields")
	void SetTileYieldOverlayVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Yields")
	void ToggleTileYieldOverlay();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Yields")
	void ToggleSpecificTileYieldLens(EConquestYieldType YieldType);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Yields")
	void RebuildTileYieldOverlay();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Yields")
	void UpdateTileYieldOverlayForTile(const FIntPoint& Coord);

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

	void RefreshPlacedTileVisualMeshes();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Hover")
	bool GetTileAtWorldLocation(
		const FVector& WorldLocation,
		int32& OutQ,
		int32& OutR,
		FHexTileData& OutTileData
		) const;

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Fog Of War")
	void SetFogOfWarVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Fog Of War")
	void ResetLocalFogOfWar(bool bVisible = true);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Fog Of War")
	void RevealFogOfWarAroundTile(FIntPoint CenterCoord, int32 Radius);

	UFUNCTION(BlueprintPure, Category = "Hex Grid|Overlay")
	bool IsHexGridOverlayVisible() const { return OverlaySettings.bShowHexGridOverlay; }

	UFUNCTION(BlueprintPure, Category = "Hex Grid|Yields")
	bool IsTileYieldOverlayVisible() const { return bShowTileYieldOverlay; }

	UFUNCTION(BlueprintPure, Category = "Hex Grid|Yields")
	EConquestYieldType GetActiveTileYieldLens() const { return ActiveTileYieldLens; }

	UFUNCTION(BlueprintPure, Category = "Hex Grid|Yields")
	bool HasActiveTileYieldLens() const { return bHasActiveTileYieldLens; }

	UFUNCTION(BlueprintPure, Category = "Hex Grid|Yields")
	bool IsTileYieldLensTransitioning() const { return bIsTileYieldLensTransitioning; }

	UPROPERTY(BlueprintAssignable, Category = "Hex Grid|Yields")
	FOnTileYieldLensTransitionFinished OnTileYieldLensTransitionFinished;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Borders")
	TObjectPtr<UProceduralMeshComponent> CivilisationBorderMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Borders")
	TObjectPtr<UProceduralMeshComponent> CivilisationBorderFillMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	TObjectPtr<UMaterialInterface> DefaultCivilisationBorderMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	TObjectPtr<UMaterialInterface> DefaultCivilisationBorderFillMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	float CivilisationBorderEdgeWidth = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	float CivilisationBorderSurfaceOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	float CivilisationBorderFillSurfaceOffset = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders", meta=(ClampMin="0.01", ClampMax="1.0"))
	float CivilisationBorderFillHexScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	float CivilisationBorderVertexRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	int32 CivilisationBorderTranslucencySortPriority = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Borders")
	int32 CivilisationBorderFillTranslucencySortPriority = 4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Expansion")
	TObjectPtr<UProceduralMeshComponent> ExpansionCandidateMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Expansion")
	TObjectPtr<UMaterialInterface> ExpansionCandidateMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Expansion")
	float ExpansionCandidateEdgeWidth = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Expansion")
	float ExpansionCandidateSurfaceOffset = 14.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Yields")
	TArray<TObjectPtr<UProceduralMeshComponent>> TileYieldBorderMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Yields")
	TArray<TObjectPtr<UProceduralMeshComponent>> TileYieldFillMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Yields")
	TArray<TObjectPtr<UProceduralMeshComponent>> TileYieldTextMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	TObjectPtr<UMaterialInterface> TileYieldBorderMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	TObjectPtr<UMaterialInterface> TileYieldFillMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	bool bUseTileYieldFillMaterialForBorder = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	FName TileYieldMaterialColorParameterName = TEXT("Colour");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	bool bShowTileYieldOverlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	EConquestYieldType ActiveTileYieldLens = EConquestYieldType::Food;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	EConquestYieldType LastSelectedTileYieldLens = EConquestYieldType::Food;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	bool bHasActiveTileYieldLens = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid|Yields")
	bool bIsTileYieldLensTransitioning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float TileYieldHexScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields", meta = (ClampMin = "0.0"))
	float TileYieldBorderWidth = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	float TileYieldSurfaceOffset = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	bool bUseFixedTileYieldHeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	float FixedTileYieldHeight = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	int32 TileYieldBorderTranslucencySortPriority = 31;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	int32 TileYieldFillTranslucencySortPriority = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	bool bShowTileYieldText = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	FLinearColor TileYieldTextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	TObjectPtr<UMaterialInterface> TileYieldTextMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	int32 TileYieldTextTranslucencySortPriority = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	float TileYieldTextWorldSize = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	float TileYieldTextHeightOffset = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields")
	FRotator TileYieldTextRotation = FRotator(90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields", meta = (ClampMin = "0"))
	int32 TileYieldHoverRevealRadius = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields|Colors")
	FLinearColor FoodYieldColor = FLinearColor(0.1f, 0.8f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields|Colors")
	FLinearColor ProductionYieldColor = FLinearColor(0.75f, 0.45f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields|Colors")
	FLinearColor GoldYieldColor = FLinearColor(1.0f, 0.78f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields|Colors")
	FLinearColor ScienceYieldColor = FLinearColor(0.1f, 0.55f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid|Yields|Colors")
	FLinearColor CultureYieldColor = FLinearColor(0.75f, 0.2f, 0.95f, 1.0f);
	
protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_ReplicatedSetupSettings();

	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedSetupSettings)
	FConquestGameSetupSettings ReplicatedSetupSettings;

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
	TObjectPtr<UDataTable> ImprovementTable = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Size")
	FHexGridSizeSettings SizeSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Height")
	FHexHeightSettings HeightSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation")
	FHexGenerationSettings GenerationSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Resources")
	FHexResourceGenerationSettings ResourceGenerationSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Rivers")
	FHexSimpleRiverSettings RiverSettings;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Features")
	FHexFeatureGenerationSettings FeatureGenerationSettings;
	
	FHexFeatureGenerator FeatureGenerator;
	FHexFeatureMeshBuilder FeatureMeshBuilder;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> FeatureMeshComponents;

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

	UPROPERTY(Transient)
	TSet<FIntPoint> LocallyRevealedFogTiles;

	int32 HoveredQ = INDEX_NONE;
	int32 HoveredR = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid")
	TObjectPtr<UProceduralMeshComponent> GridMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Water")
	TObjectPtr<UProceduralMeshComponent> WaterMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Rivers")
	TObjectPtr<UProceduralMeshComponent> RiverMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Overlay")
	TObjectPtr<UProceduralMeshComponent> HexGridOverlayMesh = nullptr;

	FHexGridModel GridModel;
	FHexMapGenerator Generator;
	FHexSimpleRiverGenerator RiverGenerator;
	FHexResourceGenerator ResourceGenerator;
	FHexMeshBuilder MeshBuilder;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Rivers")
	TArray<FHexSimpleRiverPath> GeneratedRivers;
	
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> ResourceMeshComponents;

	FHexTileResourceMeshBuilder ResourceMeshBuilder;
	FHexImprovementMeshBuilder ImprovementMeshBuilder;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> ImprovementMeshComponents;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TileYieldBorderMaterialInstances;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TileYieldFillMaterialInstances;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TileYieldTextMaterialInstances;

	TMap<int32, TMap<FIntPoint, int32>> TileYieldSectionIndicesByLayer;

	void EnsureDefaultGenerationRules();
	void ConfigureMeshComponents();
	void RebuildPlacedTileVisualMeshes();
	void RebuildFogOfWarMesh();
	void ClearTileYieldOverlay();
	void BuildAllTileYieldOverlays();
	void UpdateTileYieldOverlayVisibility();
	int32 GetTileYieldValue(const FHexYield& Yield, EConquestYieldType YieldType) const;
	FLinearColor GetTileYieldColor(EConquestYieldType YieldType) const;
	EConquestYieldType GetTileYieldTypeForLayer(int32 LayerIndex) const;
	int32 GetTileYieldLayerIndex(EConquestYieldType YieldType) const;
	float GetTileYieldOverlayZ(const FHexTileData& Tile) const;
	bool IsTileDiscoveredForYieldOverlay(const FIntPoint& Coord) const;
	bool IsTileInYieldHoverRange(const FIntPoint& Coord) const;


};
