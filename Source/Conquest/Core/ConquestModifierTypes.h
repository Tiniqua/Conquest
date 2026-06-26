#pragma once

#include "CoreMinimal.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "ConquestModifierTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestModifierSourceType : uint8
{
	Unknown,
	Building,
	Tech,
	Philosophy,
	Civilisation,
	Unit,
	UnitAugment,
	Improvement,
	Resource,
	Happiness
};

UENUM(BlueprintType)
enum class EConquestModifierTargetScope : uint8
{
	Any,
	Empire,
	City,
	Tile,
	Unit,
	Combat,
	Production,
	Movement,
	Vision,
	Philosophy,
	StrategicResource,
	OwnedTileCombat
};

UENUM(BlueprintType)
enum class EConquestModifierAttribute : uint8
{
	None,

	Food,
	Production,
	Gold,
	Science,
	Culture,
	Happiness,

	UnitStrength,
	UnitAttackRange,
	UnitMaxHealth,
	UnitHealthRegen,
	UnitMovementPoints,
	UnitGoldMaintenance,

	CombatAttack,
	CombatDefense,

	CityStrength,
	CityMaxHealth,
	CityHealPerTurn,

	OwnedTileCombatStrength,
	OwnedTileMaxHealth,
	OwnedTileHealRate,
	OwnedTileDefenderModifier,

	ProductionCost,
	MovementCost,
	VisionRange,
	PhilosophyCost,
	StrategicResourceCap
};

UENUM(BlueprintType)
enum class EConquestModifierOperation : uint8
{
	AddFlat,
	AddPercent,
	Multiply,
	Override,
	Min,
	Max
};

UENUM(BlueprintType)
enum class EConquestModifierRoundingMode : uint8
{
	None,
	Nearest,
	Floor,
	Ceil
};

UENUM(BlueprintType)
enum class EConquestModifierConditionType : uint8
{
	Always,
	PlayerHasTech,
	PlayerHasPhilosophy,
	PlayerHasPhilosophyInBranch,
	CityHasBuilding,
	CityPopulationAtLeast,
	CityIsCapital,
	UnitIs,
	ProductionIs,
	ProductionTypeIs,
	TileIsOwnedByPlayer,
	TileIsWorked,
	TileTypeIs,
	TileHasFeature,
	TileHasResource,
	TileHasImprovement,
	ResourceIs,
	SourceIs,
	SourceTypeIs,
	TargetPlayerIsSelf,
	TargetPlayerIsOther
};

UENUM(BlueprintType)
enum class EConquestModifierCountSource : uint8
{
	None,
	Cities,
	Population,
	Buildings,
	Units,
	WorkedTiles,
	OwnedTiles,
	Improvements,
	Resources,
	LuxuryResources,
	StrategicResources,
	ResearchedTechs,
	AdoptedPhilosophies,
	AdoptedPhilosophiesInBranch
};

UENUM(BlueprintType)
enum class EConquestModifierCountScope : uint8
{
	PlayerEmpire,
	PlayerWorkedTiles,
	PlayerOwnedTiles,
	ContextCity,
	ContextCityWorkedTiles,
	ContextCityOwnedTiles
};

USTRUCT(BlueprintType)
struct FConquestModifierCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EConquestModifierConditionType Type = EConquestModifierConditionType::Always;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	TArray<FName> Ids;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EHexTileType TileType = EHexTileType::Grassland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EHexFeatureType FeatureType = EHexFeatureType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	ECityProductionType ProductionType = ECityProductionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EConquestModifierSourceType SourceType = EConquestModifierSourceType::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 MinValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	bool bInvert = false;
};

USTRUCT(BlueprintType)
struct FConquestModifierDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName ModifierId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EConquestModifierTargetScope TargetScope = EConquestModifierTargetScope::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EConquestModifierAttribute Attribute = EConquestModifierAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EConquestModifierOperation Operation = EConquestModifierOperation::AddFlat;

	// AddPercent uses decimal percent values: 0.10 = +10%, -0.10 = -10%.
	// Multiply uses direct multipliers: 1.10 = 110%, 0.90 = 90%.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 ApplicationOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Count")
	EConquestModifierCountSource CountSource = EConquestModifierCountSource::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Count")
	EConquestModifierCountScope CountScope = EConquestModifierCountScope::PlayerEmpire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Count")
	FName CountId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Count")
	TArray<FName> CountIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Count", meta = (ClampMin = "1"))
	int32 CountDivisor = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Count", meta = (ClampMin = "0"))
	int32 MaxApplications = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Count")
	bool bRoundCountUp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Conditions")
	TArray<FConquestModifierCondition> Conditions;
};

USTRUCT(BlueprintType)
struct FConquestModifierRoundingRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rounding")
	EConquestModifierAttribute Attribute = EConquestModifierAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rounding")
	EConquestModifierRoundingMode Mode = EConquestModifierRoundingMode::Nearest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier|Rounding", meta = (ClampMin = "0.0001"))
	float Increment = 1.0f;
};

USTRUCT(BlueprintType)
struct FConquestModifierContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EConquestModifierTargetScope Scope = EConquestModifierTargetScope::Empire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 OtherPlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 CityId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 UnitInstanceId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName UnitId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FIntPoint TileCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	bool bHasTileCoord = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FIntPoint TargetTileCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	bool bHasTargetTileCoord = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	ECityProductionType ProductionType = ECityProductionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName ProductionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName BuildingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName ResourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName ImprovementId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName PhilosophyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName TechId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	bool bIgnoreHappinessPenalty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	bool bOnlyHappinessPenalty = false;
};
