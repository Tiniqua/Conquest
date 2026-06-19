#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConquestCivilisationTypes.generated.h"

class UMaterialInterface;
class UStaticMesh;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Theme")
	TObjectPtr<UMaterialInterface> BorderMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Theme")
	TObjectPtr<UMaterialInterface> BorderFillMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Theme")
	TObjectPtr<UMaterialInterface> CityLabelMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Theme")
	FLinearColor ThemeColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Cities")
	TArray<FName> CityNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Cities")
	TObjectPtr<UStaticMesh> CityMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Cities")
	TObjectPtr<UMaterialInterface> CityMeshMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Cities")
	bool bOverrideCityMeshScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Civilisation|Cities", meta=(EditCondition="bOverrideCityMeshScale"))
	FVector CityMeshScaleOverride = FVector(0.5f, 0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FConquestContentOverride> BuildingOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FConquestContentOverride> UnitOverrides;
};
