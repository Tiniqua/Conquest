#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Conquest/Data/ConquestDataTypes.h"
#include "ConquestUnitClassDefinition.generated.h"

class UConquestUnitDefinition;

UCLASS(BlueprintType)
class CONQUEST_API UConquestUnitClassDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	FName UnitClassId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	EConquestDomain Domain = EConquestDomain::Land;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	EConquestUnitRole Role = EConquestUnitRole::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	EConquestEra Era = EConquestEra::Ancient;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	FGameplayTagContainer UnitClassTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Class")
	TObjectPtr<UConquestUnitDefinition> DefaultUnit = nullptr;
};
