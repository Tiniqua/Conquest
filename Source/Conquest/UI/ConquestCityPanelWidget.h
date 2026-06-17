#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "ConquestCityPanelWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class UConquestChoiceButtonWidget;

UCLASS()
class CONQUEST_API UConquestCityPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void SetCity(int32 InCityId);

	UFUNCTION(BlueprintCallable)
	void Refresh();

	UFUNCTION(BlueprintCallable)
	void ClosePanel();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CityNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PopulationText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> YieldText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentProductionText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> CloseButton = nullptr;

	// This can later be renamed to ProductionChoiceBox.
	// Keep this name for now if your Widget Blueprint already uses BuildingButtonBox.
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> BuildingButtonBox;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestChoiceButtonWidget> ChoiceButtonWidgetClass;

private:
	UPROPERTY()
	int32 CityId = INDEX_NONE;

	void BuildBuildingButtons();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleProductionChoiceClicked(const FConquestChoiceButtonData& ChoiceData);
};
