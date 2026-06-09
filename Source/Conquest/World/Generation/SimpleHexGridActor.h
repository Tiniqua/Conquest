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
	Mountain  UMETA(DisplayName = "Mountain")
};

USTRUCT(BlueprintType)
struct FSimpleHexTileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	ESimpleHexTileType TileType = ESimpleHexTileType::Grassland;
};

USTRUCT(BlueprintType)
struct FSimpleHexTileGenerationRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	ESimpleHexTileType TileType = ESimpleHexTileType::Grassland;

	// Relative share of the final map.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "1"))
	int32 MinClumpSize = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation", meta = (ClampMin = "1"))
	int32 MaxClumpSize = 12;

	// Positive bias.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<ESimpleHexTileType> PreferredAdjacentTypes;

	// Negative bias.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	TArray<ESimpleHexTileType> AvoidAdjacentTypes;

	// If true, this type is allowed to place fewer tiles than its desired count
	// when the available positions are poor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	bool bSoftCount = false;

	// If true, clump growth will reject very bad candidate tiles instead of forcing placement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	bool bRejectBadAdjacency = false;

	// Minimum candidate score required when bRejectBadAdjacency is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Generation")
	float MinPlacementScore = 0.5f;
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

	// Distance from hex center to corner.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Size", meta = (ClampMin = "1.0"))
	float HexRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation")
	int32 RandomSeed = 1337;

	// Adds variation to the final tile counts.
	// Example: 0.15 means each rule weight can shift by roughly +/-15%.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CountRandomness = 0.15f;

	// Higher values make clumps more strongly prefer growing into positions beside similar/preferred terrain.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float SameTypeAdjacencyBonus = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float PreferredAdjacencyBonus = 2.0f;

	// More attempts gives better seed selection, but costs slightly more generation time.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "1"))
	int32 SeedSelectionAttempts = 48;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation")
	TArray<FSimpleHexTileGenerationRule> GenerationRules;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float AvoidAdjacencyPenalty = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0"))
	float HardAvoidAdjacencyPenalty = 12.0f;

	// Material order:
	// 0 Grassland
	// 1 Plains
	// 2 Desert
	// 3 Tundra
	// 4 Snow
	// 5 Coast
	// 6 Ocean
	// 7 Mountain
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Materials")
	TArray<UMaterialInterface*> TileMaterials;

	UPROPERTY()
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid")
	UProceduralMeshComponent* GridMesh = nullptr;

	UPROPERTY()
	TArray<FSimpleHexTileData> Tiles;

private:
	void SetupDefaultGenerationRules();

	void GenerateTileData();
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
		const FVector2D& UV
	);

	FVector GetHexCenter(int32 Q, int32 R) const;
	FVector GetHexCornerOffset(int32 CornerIndex) const;
	FVector2D GetHexCornerUV(int32 CornerIndex) const;

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