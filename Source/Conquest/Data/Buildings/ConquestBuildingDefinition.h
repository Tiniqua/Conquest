#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Conquest/Data/ConquestDataTypes.h"
#include "Conquest/Data/Requirements/ConquestRequirementTypes.h"
#include "ConquestBuildingDefinition.generated.h"

class UTexture2D;
class UStaticMesh;
class UConquestBuildingClassDefinition;
class UConquestEffectSet;

UCLASS(BlueprintType)
class CONQUEST_API UConquestBuildingDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FName BuildingId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UConquestBuildingClassDefinition> BuildingClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	bool bIsUniqueBuilding = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	EConquestEra Era = EConquestEra::Ancient;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FGameplayTagContainer BuildingTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costs")
	int32 ProductionCost = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costs")
	int32 GoldCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Costs")
	int32 MaintenanceCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yields")
	TArray<FConquestYieldAmount> BaseYields;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
	TArray<TObjectPtr<UConquestBuildingClassDefinition>> RequiredBuildingClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
	TArray<FConquestRequirement> Requirements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UConquestEffectSet> BuildingEffectSet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UStaticMesh> CityMesh = nullptr;
};
