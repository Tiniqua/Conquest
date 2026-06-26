#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Conquest/Core/ConquestContentTypes.h"
#include "Conquest/Core/ConquestModifierTypes.h"
#include "Conquest/World/Generation/HexYieldTypes.h"
#include "ConquestBuildingTypes.generated.h"

USTRUCT(BlueprintType)
struct FConquestStrategicResourceCapBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ResourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 CapBonus = 0;
};

USTRUCT(BlueprintType)
struct FConquestBuildingRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BuildingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ReplacesBuildingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EConquestProductionCategory ProductionCategory = EConquestProductionCategory::Building;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ProductionCost = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RequiredTechId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FHexYield FlatCityYieldBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 HappinessBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FConquestStrategicResourceCapBonus> StrategicResourceCapBonuses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> UnlockedTileImprovementIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifiers")
	TArray<FConquestModifierDefinition> Modifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bGrantedOnCityFounding = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHideFromProductionChoices = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsWorldWonder = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsNationalWonder = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsProject = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bPubliciseProgress = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bTriggersAscensionVictory = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequiredPhilosophyTenets = 0;

	FName GetResolvedId() const
	{
		return !BuildingId.IsNone() ? BuildingId : NAME_None;
	}
};
