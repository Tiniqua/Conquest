#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "SimpleHexGridActor.generated.h"

class UMaterialInterface;
class USceneComponent;
class UProceduralMeshComponent;

UENUM(BlueprintType)
enum class ESimpleHexTileType : uint8
{
	Grassland UMETA(DisplayName = "Grassland"),
	Plains    UMETA(DisplayName = "Plains"),
	Desert    UMETA(DisplayName = "Desert"),
	Tundra    UMETA(DisplayName = "Tundra"),
	Snow      UMETA(DisplayName = "Snow"),
	Coast     UMETA(DisplayName = "Coast"),
	Ocean     UMETA(DisplayName = "Ocean"),
	Lake      UMETA(DisplayName = "Lake"),
	Mountain  UMETA(DisplayName = "Mountain")
};

USTRUCT(BlueprintType)
struct FSimpleHexTileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	ESimpleHexTileType TileType = ESimpleHexTileType::Grassland;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	float Height = 0.0f;
};

USTRUCT(BlueprintType)
struct FSimpleHexTileGenerationRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	ESimpleHexTileType TileType = ESimpleHexTileType::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "1"))
	int32 MinClumpSize = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "1"))
	int32 MaxClumpSize = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<ESimpleHexTileType> PreferredAdjacentTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<ESimpleHexTileType> AvoidAdjacentTypes;

	// If this is not empty, this tile can only be placed beside one of these assigned tile types.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<ESimpleHexTileType> RequiredAdjacentTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	bool bSoftCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	bool bRejectBadAdjacency = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	float MinPlacementScore = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	float HeightOffset = 0.0f;
};

struct FSimpleHexMeshSection
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
};

struct FSimpleHexVertexKey
{
	int32 X = 0;
	int32 Y = 0;

	bool operator==(const FSimpleHexVertexKey& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
};

FORCEINLINE uint32 GetTypeHash(const FSimpleHexVertexKey& Key)
{
	uint32 Hash = ::GetTypeHash(Key.X);
	Hash = HashCombine(Hash, ::GetTypeHash(Key.Y));
	return Hash;
}

UCLASS()
class CONQUEST_API ASimpleHexGridActor : public AActor
{
	GENERATED_BODY()

public:
	ASimpleHexGridActor();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hex Grid")
	void RebuildGrid();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Size", meta = (ClampMin = "1"))
	int32 GridWidth = 20;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Size", meta = (ClampMin = "1"))
	int32 GridHeight = 20;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Size", meta = (ClampMin = "1.0"))
	float HexRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Height")
	bool bUseHeightOffsets = true;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Height")
	float HeightScale = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Height", meta = (ClampMin = "1.0"))
	float VertexKeyPrecision = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation")
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CountRandomness = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float SameTypeAdjacencyBonus = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float PreferredAdjacencyBonus = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float AvoidAdjacencyPenalty = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float HardAvoidAdjacencyPenalty = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "1"))
	int32 SeedSelectionAttempts = 48;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation")
	TArray<FSimpleHexTileGenerationRule> GenerationRules;

	// Material order:
	// 0 Grassland
	// 1 Plains
	// 2 Desert
	// 3 Tundra
	// 4 Snow
	// 5 Coast
	// 6 Ocean / Lake
	// 7 Mountain
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Materials")
	TArray<UMaterialInterface*> TileMaterials;

	UPROPERTY()
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid")
	UProceduralMeshComponent* GridMesh = nullptr;

	UPROPERTY()
	TArray<FSimpleHexTileData> Tiles;

	TMap<FSimpleHexVertexKey, float> ResolvedVertexHeights;

	// HEX GRID
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Overlay")
	bool bShowHexGridOverlay = true;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Overlay", meta = (ClampMin = "0.1"))
	float GridLineWidth = 4.0f;

	// Small offset above the terrain surface to prevent z-fighting.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Overlay", meta = (ClampMin = "0.0"))
	float GridOverlaySurfaceOffset = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Overlay")
	UMaterialInterface* GridOverlayMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid|Overlay")
	UProceduralMeshComponent* HexGridOverlayMesh = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Hex Grid|Overlay")
	void SetHexGridOverlayVisible(bool bVisible);

	void GenerateGridOverlayMesh();

	void AddGridEdgeQuad(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents,
		const FVector& A,
		const FVector& B,
		float Width
	);

	bool ShouldDrawGridEdge(int32 Q, int32 R, int32 EdgeIndex) const;

private:
	void SetupDefaultGenerationRules();

	void GenerateTileData();
	void ResolveTileHeights();
	void ResolveSharedVertexHeights();
	void GenerateMesh();

	void BuildDesiredTileCounts(
		FRandomStream& RandomStream,
		TMap<ESimpleHexTileType, int32>& OutDesiredCounts
	) const;

	bool FindSeedTileForRule(
		const FSimpleHexTileGenerationRule& Rule,
		FRandomStream& RandomStream,
		const TArray<bool>& Assigned,
		int32& OutQ,
		int32& OutR
	) const;

	float ScoreSeedTileForRule(
		const FSimpleHexTileGenerationRule& Rule,
		int32 Q,
		int32 R,
		const TArray<bool>& Assigned
	) const;

	float ScoreTileForRuleAdjacency(
		const FSimpleHexTileGenerationRule& Rule,
		int32 Q,
		int32 R,
		const TArray<bool>& Assigned
	) const;

	bool DoesTileSatisfyRequiredAdjacency(
		const FSimpleHexTileGenerationRule& Rule,
		int32 Q,
		int32 R,
		const TArray<bool>& Assigned
	) const;

	int32 GrowClump(
		const FSimpleHexTileGenerationRule& Rule,
		int32 SeedQ,
		int32 SeedR,
		int32 TargetSize,
		FRandomStream& RandomStream,
		TArray<bool>& Assigned
	);

	int32 PickBestFrontierIndex(
		const FSimpleHexTileGenerationRule& Rule,
		const TArray<FIntPoint>& Frontier,
		FRandomStream& RandomStream,
		const TArray<bool>& Assigned
	) const;

	ESimpleHexTileType PickWeightedTileType(FRandomStream& RandomStream) const;
	ESimpleHexTileType PickWeightedLandTileType(FRandomStream& RandomStream) const;

	float GetHeightOffsetForTileType(ESimpleHexTileType TileType) const;
	float GetResolvedCornerHeight(const FVector& FlatCornerPosition) const;

	void AddHexTile(
		int32 Q,
		int32 R,
		TArray<FSimpleHexMeshSection>& MeshSections
	);

	void AddTriangle(
		FSimpleHexMeshSection& Section,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC
	);
	
	void AddVertex(
	FSimpleHexMeshSection& Section,
	const FVector& Position,
	const FVector2D& UV,
	const FVector& Normal
);
	
	FVector GetHexCenter(int32 Q, int32 R) const;
	FVector GetHexCornerOffset(int32 CornerIndex) const;
	FVector2D GetHexCornerUV(int32 CornerIndex) const;

	FSimpleHexVertexKey MakeVertexKey(const FVector& FlatPosition) const;

	bool GetNeighbourCoord(
		int32 Q,
		int32 R,
		int32 Direction,
		int32& OutQ,
		int32& OutR
	) const;

	int32 GetTileIndex(int32 Q, int32 R) const;
	bool IsValidCoord(int32 Q, int32 R) const;
	bool IsValidTile(int32 Q, int32 R) const;

	int32 GetSectionIndexForTileType(ESimpleHexTileType TileType) const;
	int32 GetSectionCount() const;
};