#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Conquest/Data/ConquestDataTypes.h"
#include "Conquest/Data/Requirements/ConquestRequirementTypes.h"
#include "ConquestUnitDefinition.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;
class UConquestUnitClassDefinition;
class UConquestEffectSet;

UCLASS(BlueprintType)
class CONQUEST_API UConquestUnitDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FName UnitId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	TObjectPtr<UConquestUnitClassDefinition> UnitClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	bool bIsUniqueUnit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EConquestDomain Domain = EConquestDomain::Land;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EConquestUnitRole Role = EConquestUnitRole::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EConquestEra Era = EConquestEra::Ancient;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FGameplayTagContainer UnitTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costs")
	int32 ProductionCost = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costs")
	int32 GoldCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costs")
	int32 MaintenanceCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 CombatStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 RangedStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Range = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Movement = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Sight = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	TObjectPtr<UConquestUnitClassDefinition> UpgradesToUnitClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
	TArray<FConquestRequirement> Requirements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UConquestEffectSet> UnitEffectSet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;
};
