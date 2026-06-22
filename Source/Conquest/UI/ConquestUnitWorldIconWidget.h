#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestUnitWorldIconWidget.generated.h"

class UImage;
class UMaterialInterface;
class UTextBlock;

UCLASS()
class CONQUEST_API UConquestUnitWorldIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void SetUnitIcon(
		const FText& UnitName,
		const FText& CivilisationName,
		FLinearColor UnitDisplayColor,
		UMaterialInterface* UnitIconMaterial
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Unit")
	void OnUnitIconChanged(
		const FText& UnitName,
		const FText& CivilisationName,
		FLinearColor UnitDisplayColor,
		UMaterialInterface* UnitIconMaterial
	);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnitNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivilisationNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> UnitIconImage = nullptr;
};
