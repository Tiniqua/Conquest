#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestCityWorldLabelWidget.generated.h"

class UTextBlock;
class UImage;
class UMaterialInterface;

UCLASS()
class CONQUEST_API UConquestCityWorldLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|City")
	void SetCityLabel(
		const FName& CityName,
		int32 Population,
		UMaterialInterface* CivilisationThemeMaterial,
		FLinearColor CivilisationThemeColor
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|City")
	void OnCivilisationThemeChanged(UMaterialInterface* CivilisationThemeMaterial, FLinearColor CivilisationThemeColor);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CityNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PopulationText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ThemeMaterialImage = nullptr;
};
