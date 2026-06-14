#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ConquestUnitTypes.generated.h"

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
	int32 CombatStrength = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Movement = 2;
};