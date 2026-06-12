#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Conquest/Data/ConquestDataTypes.h"
#include "ConquestEffectTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestEffectTargetType : uint8
{
	Player      UMETA(DisplayName = "Player"),
	City        UMETA(DisplayName = "City"),
	Unit        UMETA(DisplayName = "Unit"),
	Building    UMETA(DisplayName = "Building"),
	Tile        UMETA(DisplayName = "Tile"),
	Resource    UMETA(DisplayName = "Resource"),
	Production  UMETA(DisplayName = "Production"),
	Combat      UMETA(DisplayName = "Combat"),
	Movement    UMETA(DisplayName = "Movement"),
	Yield       UMETA(DisplayName = "Yield")
};

UENUM(BlueprintType)
enum class EConquestEffectScope : uint8
{
	Self             UMETA(DisplayName = "Self"),
	Owner            UMETA(DisplayName = "Owner"),
	Player           UMETA(DisplayName = "Player"),
	Capital          UMETA(DisplayName = "Capital"),
	LocalCity        UMETA(DisplayName = "Local City"),
	AllOwnedCities   UMETA(DisplayName = "All Owned Cities"),
	AllOwnedUnits    UMETA(DisplayName = "All Owned Units"),
	WorkedTiles      UMETA(DisplayName = "Worked Tiles"),
	OwnedTiles       UMETA(DisplayName = "Owned Tiles"),
	CombatAttacker   UMETA(DisplayName = "Combat Attacker"),
	CombatDefender   UMETA(DisplayName = "Combat Defender")
};

UENUM(BlueprintType)
enum class EConquestEffectTrigger : uint8
{
	Passive                 UMETA(DisplayName = "Passive"),
	OnTurnStarted           UMETA(DisplayName = "On Turn Started"),
	OnTurnEnded             UMETA(DisplayName = "On Turn Ended"),
	OnCityFounded           UMETA(DisplayName = "On City Founded"),
	OnCityCaptured          UMETA(DisplayName = "On City Captured"),
	OnBuildingConstructed   UMETA(DisplayName = "On Building Constructed"),
	OnUnitTrained           UMETA(DisplayName = "On Unit Trained"),
	OnUnitKilled            UMETA(DisplayName = "On Unit Killed"),
	OnCombatStarted         UMETA(DisplayName = "On Combat Started"),
	OnCombatResolved        UMETA(DisplayName = "On Combat Resolved"),
	OnTileImproved          UMETA(DisplayName = "On Tile Improved"),
	OnTechnologyResearched  UMETA(DisplayName = "On Technology Researched")
};

UENUM(BlueprintType)
enum class EConquestModifierTarget : uint8
{
	None                UMETA(DisplayName = "None"),
	TileYield           UMETA(DisplayName = "Tile Yield"),
	CityYield           UMETA(DisplayName = "City Yield"),
	PlayerYield         UMETA(DisplayName = "Player Yield"),
	BuildingYield       UMETA(DisplayName = "Building Yield"),
	UnitCombatStrength  UMETA(DisplayName = "Unit Combat Strength"),
	UnitRangedStrength  UMETA(DisplayName = "Unit Ranged Strength"),
	UnitMovement        UMETA(DisplayName = "Unit Movement"),
	UnitSight           UMETA(DisplayName = "Unit Sight"),
	ProductionCost      UMETA(DisplayName = "Production Cost"),
	ProductionOutput    UMETA(DisplayName = "Production Output"),
	BuildingProduction  UMETA(DisplayName = "Building Production"),
	UnitProduction      UMETA(DisplayName = "Unit Production"),
	MovementCost        UMETA(DisplayName = "Movement Cost"),
	MaintenanceCost     UMETA(DisplayName = "Maintenance Cost")
};

USTRUCT(BlueprintType)
struct CONQUEST_API FConquestEffectModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	EConquestModifierTarget Target = EConquestModifierTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	EConquestModifierOperation Operation = EConquestModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	EConquestStackingRule StackingRule = EConquestStackingRule::StackNormally;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	EConquestYieldType YieldType = EConquestYieldType::Food;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Tags")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Tags")
	FGameplayTagContainer BlockedTargetTags;
};
