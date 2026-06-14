#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConquestCivilisationTypes.generated.h"

USTRUCT(BlueprintType)
struct FConquestContentOverride
{
	GENERATED_BODY()

	// The base thing being replaced.
	// Example: Granary
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ReplacesId = NAME_None;

	// The unique thing shown instead.
	// Example: Roman_Granary
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ReplacementId = NAME_None;
};

UCLASS(BlueprintType)
class CONQUEST_API UConquestCivilisationData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CivilisationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText CivilisationName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText LeaderName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FConquestContentOverride> BuildingOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FConquestContentOverride> UnitOverrides;
};
