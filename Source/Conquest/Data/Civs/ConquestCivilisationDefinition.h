#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ConquestCivilisationDefinition.generated.h"

class UTexture2D;
class UConquestLeaderDefinition;
class UConquestUnitClassDefinition;
class UConquestUnitDefinition;
class UConquestBuildingClassDefinition;
class UConquestBuildingDefinition;
class UConquestEffectSet;

USTRUCT(BlueprintType)
struct CONQUEST_API FConquestUnitReplacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Replacement")
	TObjectPtr<UConquestUnitClassDefinition> ReplacedUnitClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Replacement")
	TObjectPtr<UConquestUnitDefinition> ReplacementUnit = nullptr;
};

USTRUCT(BlueprintType)
struct CONQUEST_API FConquestBuildingReplacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Replacement")
	TObjectPtr<UConquestBuildingClassDefinition> ReplacedBuildingClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building Replacement")
	TObjectPtr<UConquestBuildingDefinition> ReplacementBuilding = nullptr;
};

UCLASS(BlueprintType)
class CONQUEST_API UConquestCivilisationDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FName CivilisationId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FText CivilisationName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FText ShortName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FText Adjective;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FLinearColor PrimaryColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FLinearColor SecondaryColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation")
	FGameplayTagContainer CivilisationTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation|Leaders")
	TArray<TObjectPtr<UConquestLeaderDefinition>> AvailableLeaders;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation|Cities")
	TArray<FText> CityNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation|Unique Units")
	TArray<FConquestUnitReplacement> UniqueUnitReplacements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation|Unique Buildings")
	TArray<FConquestBuildingReplacement> UniqueBuildingReplacements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Civilisation|Effects")
	TObjectPtr<UConquestEffectSet> CivilisationEffectSet = nullptr;
};
