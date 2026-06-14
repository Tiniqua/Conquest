#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ConquestTechTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestEra : uint8
{
	Ancient,
	Classical,
	Medieval,
	Renaissance,
	Industrial,
	Modern,
	Future
};

USTRUCT(BlueprintType)
struct FConquestTechRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName TechId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ScienceCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EConquestEra Era = EConquestEra::Ancient;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> PrerequisiteTechIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> UnlockedBuildingIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> UnlockedUnitIds;
};