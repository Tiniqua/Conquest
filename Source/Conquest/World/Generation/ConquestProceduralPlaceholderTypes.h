#pragma once

#include "CoreMinimal.h"
#include "ConquestProceduralPlaceholderTypes.generated.h"

class UMaterialInterface;

UENUM(BlueprintType)
enum class EConquestPlaceholderShape : uint8
{
	Box        UMETA(DisplayName = "Box"),
	Pyramid    UMETA(DisplayName = "Pyramid"),
	Cylinder   UMETA(DisplayName = "Cylinder"),
	HexPrism   UMETA(DisplayName = "Hex Prism")
};

UENUM(BlueprintType)
enum class EConquestPlaceholderPlacementBias : uint8
{
	Center      UMETA(DisplayName = "Center"),
	Edges       UMETA(DisplayName = "Edges"),
	Random      UMETA(DisplayName = "Random"),
	CityBiased  UMETA(DisplayName = "City Biased")
};

UENUM(BlueprintType)
enum class EConquestPlaceholderVisualPreset : uint8
{
	Custom       UMETA(DisplayName = "Custom"),
	Farm         UMETA(DisplayName = "Farm"),
	Mine         UMETA(DisplayName = "Mine"),
	Quarry       UMETA(DisplayName = "Quarry"),
	LumberMill   UMETA(DisplayName = "Lumber Mill"),
	Plantation   UMETA(DisplayName = "Plantation"),
	Pasture      UMETA(DisplayName = "Pasture"),
	Camp         UMETA(DisplayName = "Camp"),
	FishingBoat  UMETA(DisplayName = "Fishing Boat"),
	Fort         UMETA(DisplayName = "Fort"),
	WatchTower   UMETA(DisplayName = "Watch Tower")
};

USTRUCT(BlueprintType)
struct FConquestProceduralPlaceholderVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	EConquestPlaceholderVisualPreset Preset = EConquestPlaceholderVisualPreset::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	bool bCoversFullTile = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	EConquestPlaceholderShape Shape = EConquestPlaceholderShape::Box;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual", meta = (ClampMin = "1", ClampMax = "32"))
	int32 ShapeInstances = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual", meta = (ClampMin = "1.0"))
	float MeshHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual", meta = (ClampMin = "0.01"))
	FVector ShapeScale = FVector(0.35f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ShapeScaleRate = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	EConquestPlaceholderPlacementBias PlacementBias = EConquestPlaceholderPlacementBias::Center;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	bool bAlignToTerrain = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	FVector MeshOffset = FVector(0.0f, 0.0f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Visual")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	bool ShouldRender() const
	{
		return bEnabled || Preset != EConquestPlaceholderVisualPreset::Custom;
	}
};
