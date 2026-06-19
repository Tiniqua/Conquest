#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ConquestUnitTypes.generated.h"

class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class EConquestUnitAugmentStat : uint8
{
	Strength,
	AttackRange,
	MaxHealth,
	HealthRegen,
	Movement
};

USTRUCT(BlueprintType)
struct FConquestStrategicResourceCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ResourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	int32 Amount = 0;
};

USTRUCT(BlueprintType)
struct FConquestUnitAugmentRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AugmentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EConquestUnitAugmentStat ModifiedStat = EConquestUnitAugmentStat::Strength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FlatBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FConquestStrategicResourceCost> ResourceCosts;
};

USTRUCT(BlueprintType)
struct FConquestUnitRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName UnitId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ReplacesUnitId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ProductionCost = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RequiredTechId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName UpgradeUnlockedByTechId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName UpgradesToUnitId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 UpgradeGoldCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 Strength = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 AttackRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 HealthRegenPerTurn = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 MovementPoints = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 GoldMaintenancePerTurn = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	bool bCanFoundCity = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> UnitMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> UnitMeshMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta=(ClampMin="1"))
	int32 UnitMeshCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta=(ClampMin="0.0"))
	float UnitMeshSpacing = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector UnitMeshScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector UnitMeshOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator UnitMeshRotation = FRotator::ZeroRotator;
};
