#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "WorldGenRuleTypes.h"
#include "WorldGenProceduralActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FWorldGenTileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Gen")
	EWorldGenTileType TileType = EWorldGenTileType::Grassland;
};

struct FWorldGenMeshSectionData
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
class CONQUEST_API AWorldGenProceduralActor : public AActor
{
	GENERATED_BODY()

public:
	AWorldGenProceduralActor();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "World Gen")
	void RebuildWorld();

	// debug
	UPROPERTY(EditAnywhere, Category = "World Gen|Debug")
	bool bLogGenerationDebug = true;

	UPROPERTY(EditAnywhere, Category = "World Gen|Debug")
	bool bLogEveryTile = false;

	UPROPERTY(EditAnywhere, Category = "World Gen|Debug")
	bool bLogEveryEdge = true;

	UPROPERTY(EditAnywhere, Category = "World Gen|Debug")
	bool bLogOnlyProblemEdges = false;

	UPROPERTY(EditAnywhere, Category = "World Gen|Debug")
	bool bValidateGeneratedMesh = true;

	UPROPERTY(EditAnywhere, Category = "World Gen|Debug", meta = (ClampMin = "0.0"))
	float DebugHeightTolerance = 0.1f;

	void LogGenerationDebug(const TArray<FWorldGenMeshSectionData>& MeshSections) const;

	void LogTileDebug(int32 Q,int32 R) const;

	void LogEdgeDebug(int32 Q,int32 R,int32 EdgeIndex,bool& bOutProblemFound) const;

	void ValidateMeshSections(const TArray<FWorldGenMeshSectionData>& MeshSections) const;

	FString TileTypeToString(EWorldGenTileType TileType) const;
	FString TransitionStyleToString(EWorldGenTransitionStyle TransitionStyle) const;
	FString EdgeIndexToString(int32 EdgeIndex) const;

	// debug end
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "World Gen")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = "World Gen|Grid", meta = (ClampMin = "1"))
	int32 GridWidth = 20;

	UPROPERTY(EditAnywhere, Category = "World Gen|Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 20;

	UPROPERTY(EditAnywhere, Category = "World Gen|Grid", meta = (ClampMin = "1.0"))
	float HexRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Generation")
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, Category = "World Gen|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WaterChance = 0.18f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HillChance = 0.22f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MountainChance = 0.04f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Heights")
	float FlatHeight = 0.0f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Heights")
	float HillHeight = 80.0f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Heights")
	float MountainHeight = 180.0f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Heights")
	float CoastHeight = -12.0f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Heights")
	float OceanHeight = -36.0f;

	// How far the transition owner pulls its plateau edge inward.
	UPROPERTY(EditAnywhere, Category = "World Gen|Shape", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float TransitionInsetRatio = 0.28f;

	UPROPERTY(EditAnywhere, Category = "World Gen|Lighting")
	bool bUseStylizedUpNormals = true;

	UPROPERTY(EditAnywhere, Category = "World Gen|Materials")
	TArray<UMaterialInterface*> TileMaterials;

private:
	UPROPERTY()
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "World Gen")
	UProceduralMeshComponent* WorldMesh = nullptr;

	UPROPERTY()
	TArray<FWorldGenTileData> Tiles;

private:
	void GenerateTileData();
	void GenerateMesh();

	void AddTileGeometry(
		int32 Q,
		int32 R,
		TArray<FWorldGenMeshSectionData>& MeshSections
	);

	void AddTriangle(
		FWorldGenMeshSectionData& Section,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC
	);

	void AddQuad(
		FWorldGenMeshSectionData& Section,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC,
		const FVector2D& UVD
	);

	void AddVertex(
		FWorldGenMeshSectionData& Section,
		const FVector& Position,
		const FVector2D& UV,
		const FVector& Normal
	);

	FVector CalculateNormal(
		const FVector& A,
		const FVector& B,
		const FVector& C
	) const;

	FVector GetHexCenter(int32 Q, int32 R) const;
	FVector GetHexCornerOffset(int32 CornerIndex, float Radius, float Z = 0.0f) const;
	FVector2D GetHexCornerUV(int32 CornerIndex, float Scale = 1.0f) const;

	bool GetNeighbourCoord(
		int32 Q,
		int32 R,
		int32 EdgeIndex,
		int32& OutQ,
		int32& OutR
	) const;

	bool IsValidTile(int32 Q, int32 R) const;
	int32 GetTileIndex(int32 Q, int32 R) const;

	const FWorldGenTileData* GetTile(int32 Q, int32 R) const;

	FWorldGenTileRule GetRule(EWorldGenTileType TileType) const;
	int32 GetSectionIndexForTile(EWorldGenTileType TileType) const;
};