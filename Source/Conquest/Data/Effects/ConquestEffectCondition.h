#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ConquestEffectCondition.generated.h"

UENUM(BlueprintType)
enum class EConquestConditionType : uint8
{
	Always                         UMETA(DisplayName = "Always"),
	OwnerHasTag                    UMETA(DisplayName = "Owner Has Tag"),
	TargetHasTag                   UMETA(DisplayName = "Target Has Tag"),
	SourceHasTag                   UMETA(DisplayName = "Source Has Tag"),
	CityIsCapital                  UMETA(DisplayName = "City Is Capital"),
	CityIsCoastal                  UMETA(DisplayName = "City Is Coastal"),
	CityHasBuildingClass           UMETA(DisplayName = "City Has Building Class"),
	TileHasTerrainTag              UMETA(DisplayName = "Tile Has Terrain Tag"),
	TileHasFeatureTag              UMETA(DisplayName = "Tile Has Feature Tag"),
	TileHasResourceTag             UMETA(DisplayName = "Tile Has Resource Tag"),
	TileHasImprovementTag          UMETA(DisplayName = "Tile Has Improvement Tag"),
	TileIsAdjacentToRiver          UMETA(DisplayName = "Tile Is Adjacent To River"),
	UnitIsAttacking                UMETA(DisplayName = "Unit Is Attacking"),
	UnitIsDefending                UMETA(DisplayName = "Unit Is Defending"),
	UnitIsWounded                  UMETA(DisplayName = "Unit Is Wounded"),
	BuildingClassExistsInCapital   UMETA(DisplayName = "Building Class Exists In Capital")
};

UCLASS(BlueprintType, Abstract)
class CONQUEST_API UConquestEffectCondition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	FName ConditionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	EConquestConditionType ConditionType = EConquestConditionType::Always;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	bool bInvertResult = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition|Tags")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition|Tags")
	FGameplayTagContainer BlockedTags;
};
