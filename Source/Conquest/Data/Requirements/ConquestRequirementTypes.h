#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ConquestRequirementTypes.generated.h"

class UConquestBuildingClassDefinition;

UENUM(BlueprintType)
enum class EConquestRequirementType : uint8
{
	None                  UMETA(DisplayName = "None"),
	PlayerHasTechnology   UMETA(DisplayName = "Player Has Technology"),
	PlayerHasPolicy       UMETA(DisplayName = "Player Has Policy"),
	PlayerHasResource     UMETA(DisplayName = "Player Has Resource"),
	CityHasBuildingClass  UMETA(DisplayName = "City Has Building Class"),
	CityIsCoastal         UMETA(DisplayName = "City Is Coastal"),
	CityIsCapital         UMETA(DisplayName = "City Is Capital"),
	CityAdjacentToRiver   UMETA(DisplayName = "City Adjacent To River"),
	TileHasResource       UMETA(DisplayName = "Tile Has Resource"),
	TileHasImprovement    UMETA(DisplayName = "Tile Has Improvement"),
	TileHasFeature        UMETA(DisplayName = "Tile Has Feature")
};

USTRUCT(BlueprintType)
struct CONQUEST_API FConquestRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	EConquestRequirementType RequirementType = EConquestRequirementType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	bool bInvert = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	TObjectPtr<UConquestBuildingClassDefinition> RequiredBuildingClass = nullptr;
};
