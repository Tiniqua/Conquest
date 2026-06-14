#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ConquestPhilosophyTypes.generated.h"

USTRUCT(BlueprintType)
struct FConquestPhilosophyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PhilosophyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> PrerequisitePhilosophyIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BranchId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CultureCost = 100;
};