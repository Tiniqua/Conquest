#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Conquest/Data/ConquestDataTypes.h"
#include "ConquestBuildingClassDefinition.generated.h"

class UConquestBuildingDefinition;

UCLASS(BlueprintType)
class CONQUEST_API UConquestBuildingClassDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Class")
	FName BuildingClassId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Class")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Class")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Class")
	EConquestEra Era = EConquestEra::Ancient;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Class")
	FGameplayTagContainer BuildingClassTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Class")
	TObjectPtr<UConquestBuildingDefinition> DefaultBuilding = nullptr;
};
