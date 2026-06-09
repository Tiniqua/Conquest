#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "HexGridProceduralActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EHexTileType : uint8
{
	FlatGround UMETA(DisplayName = "Flat Ground"),
	Hill       UMETA(DisplayName = "Hill"),
	Ocean      UMETA(DisplayName = "Ocean")
};

USTRUCT(BlueprintType)
struct FHexTileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	EHexTileType TileType = EHexTileType::FlatGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	float Height = 0.0f;
};

struct FHexMeshSectionData
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	void Reset()
	{
		Vertices.Reset();
		Triangles.Reset();
		Normals.Reset();
		UVs.Reset();
		VertexColors.Reset();
		Tangents.Reset();
	}
};

UCLASS()
class CONQUEST_API AHexGridProceduralActor : public AActor
{
	GENERATED_BODY()

public:
	AHexGridProceduralActor();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hex Grid")
	void RebuildGrid();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = "Hex Grid", meta = (ClampMin = "1"))
	int32 GridWidth = 20;

	UPROPERTY(EditAnywhere, Category = "Hex Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 20;

	UPROPERTY(EditAnywhere, Category = "Hex Grid", meta = (ClampMin = "1.0"))
	float HexRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OceanChance = 0.20f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HillChance = 0.20f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Heights")
	float FlatHeight = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Heights")
	float HillHeight = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Heights")
	float OceanHeight = -20.0f;

	// 1.0 = no visible transition band.
	// 0.72 means the middle 72% is a plateau and the outer 28% is the taper/edge band.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Shape", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float InnerPlateauScale = 0.72f;

	// Default true for Civ/board-game style lighting.
	// False gives real geometric slope normals, which can look harsher.
	UPROPERTY(EditAnywhere, Category = "Hex Grid|Lighting")
	bool bUseStylizedUpNormals = true;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Materials")
	UMaterialInterface* FlatGroundMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Materials")
	UMaterialInterface* HillMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Materials")
	UMaterialInterface* OceanMaterial = nullptr;

private:
	UPROPERTY()
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Hex Grid")
	UProceduralMeshComponent* GridMesh = nullptr;

	UPROPERTY()
	TArray<FHexTileData> Tiles;

	// Global resolved corner heights.
	// This prevents neighbouring tiles from creating different Z values at the same hex corner.
	TMap<FIntPoint, float> SharedCornerHeights;

private:
	void GenerateTileData();
	void BuildSharedCornerHeights();
	void GenerateMesh();

	void AddTileGeometry(
		int32 Q,
		int32 R,
		FHexMeshSectionData& FlatSection,
		FHexMeshSectionData& HillSection,
		FHexMeshSectionData& OceanSection
	);

	void AddPlateauTriangle(
		FHexMeshSectionData& Section,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC
	);

	void AddEdgeBandQuad(
		FHexMeshSectionData& Section,
		const FVector& InnerA,
		const FVector& InnerB,
		const FVector& OuterA,
		const FVector& OuterB,
		const FVector2D& UVInnerA,
		const FVector2D& UVInnerB,
		const FVector2D& UVOuterA,
		const FVector2D& UVOuterB
	);

	void AddTriangleVisibleFromAbove(
		FHexMeshSectionData& Section,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC
	);

	void AddVertex(
		FHexMeshSectionData& Section,
		const FVector& Position,
		const FVector2D& UV,
		const FVector& Normal
	);

	FVector CalculateTriangleNormal(
		const FVector& A,
		const FVector& B,
		const FVector& C
	) const;

	FHexMeshSectionData& GetSectionForTileType(
		EHexTileType TileType,
		FHexMeshSectionData& FlatSection,
		FHexMeshSectionData& HillSection,
		FHexMeshSectionData& OceanSection
	) const;

	FVector GetHexCenter(int32 Q, int32 R) const;
	FVector GetHexCornerOffset(int32 CornerIndex, float Radius, float Z = 0.0f) const;
	FVector2D GetHexCornerWorldXY(int32 Q, int32 R, int32 CornerIndex) const;
	FVector2D GetHexCornerUV(int32 CornerIndex, float Scale) const;

	FIntPoint MakeCornerKey(const FVector2D& WorldXY) const;

	float GetTileHeight(EHexTileType TileType) const;
	float GetSharedCornerHeight(int32 Q, int32 R, int32 CornerIndex) const;
	float GetDesiredEdgeHeightForTile(int32 Q, int32 R, int32 EdgeIndex) const;
	float ResolveCornerHeight(const TArray<float>& Samples) const;

	bool IsTransitionOwner(
		EHexTileType ThisType,
		EHexTileType OtherType
	) const;

	bool GetNeighbourCoord(
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		int32& OutQ,
		int32& OutR
	) const;

	bool IsValidTile(int32 Q, int32 R) const;
	int32 GetTileIndex(int32 Q, int32 R) const;

	const FHexTileData* GetTile(int32 Q, int32 R) const;
};