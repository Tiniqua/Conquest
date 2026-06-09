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

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WaterChance = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Hex Grid|Generation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MountainChance = 0.04f;

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
	void GenerateTileData();
	void GenerateMesh();

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

	int32 GetTileIndex(int32 Q, int32 R) const;
	bool IsValidTile(int32 Q, int32 R) const;

	int32 GetSectionIndexForTileType(ESimpleHexTileType TileType) const;
	int32 GetSectionCount() const;
};