#pragma once

#include "CoreMinimal.h"
#include "Conquest/World/Generation/HexYieldTypes.h"
#include "ConquestPlayerEmpireState.generated.h"

USTRUCT(BlueprintType)
struct FConquestPlayerEmpireState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> CityIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexYield StoredYields;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHexYield CachedYieldPerTurn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> ResearchedTechIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CurrentResearchId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentResearchProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AdoptedPhilosophyTenets = 0;

	bool HasResearched(FName TechId) const
	{
		return !TechId.IsNone() && ResearchedTechIds.Contains(TechId);
	}
};
