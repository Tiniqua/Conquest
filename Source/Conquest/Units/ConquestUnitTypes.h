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
	Movement,
	AttackMultiplier,
	DefenseMultiplier
};

UENUM(BlueprintType)
enum class EConquestUnitCombatModifierType : uint8
{
	Attack,
	Defense
};

UENUM(BlueprintType)
enum class EConquestCombatPreviewRating : uint8
{
	Invalid,
	Safe,
	Neutral,
	Costly
};

USTRUCT(BlueprintType)
struct FConquestUnitCombatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ModifierId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConquestUnitCombatModifierType ModifierType = EConquestUnitCombatModifierType::Attack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Multiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FlatBonus = 0;
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
	float MultiplierBonus = 0.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TArray<FName> AllowedAugmentIds;

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

USTRUCT(BlueprintType)
struct FConquestCombatPreviewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 AttackerUnitInstanceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 DefenderUnitInstanceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	FText AttackerName;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	FText DefenderName;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 AttackerAttackValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 DefenderAttackValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 DamageDealt = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 DamageTaken = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 AttackerCurrentHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 AttackerProjectedHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 AttackerMaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 DefenderCurrentHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 DefenderProjectedHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 DefenderMaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	int32 AttackDistance = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	bool bCanAttack = false;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	bool bHasCounterAttack = false;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	bool bAttackerKilled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	bool bDefenderKilled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	EConquestCombatPreviewRating Rating = EConquestCombatPreviewRating::Invalid;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	FText RatingText;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	FText InvalidReason;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	TArray<FText> ModifierTexts;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Combat Preview")
	bool bIsValid = false;
};
