#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "ConquestCityPanelWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class UImage;
class UMaterialInterface;
class UConquestChoiceButtonWidget;
class AConquestGameState;
struct FCityState;

UENUM(BlueprintType)
enum class EConquestCityProductionPanelTab : uint8
{
	Units,
	Buildings
};

UCLASS()
class CONQUEST_API UConquestCityPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	void SetCity(int32 InCityId);

	UFUNCTION(BlueprintCallable)
	void Refresh();

	UFUNCTION(BlueprintCallable)
	void ClosePanel();

	UFUNCTION(BlueprintCallable, Category = "Conquest|City")
	void SetProductionPanelTab(EConquestCityProductionPanelTab NewTab);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|City")
	void OnCivilisationThemeChanged(UMaterialInterface* CivilisationThemeMaterial, FLinearColor CivilisationThemeColor);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CityNameText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> CityNameThemeMaterialImage = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> ThemeMaterialImage = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PopulationText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> YieldText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentProductionText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> CloseButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> UnitsButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> BuildingsButton = nullptr;

	// This can later be renamed to ProductionChoiceBox.
	// Keep this name for now if your Widget Blueprint already uses BuildingButtonBox.
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> BuildingButtonBox;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestChoiceButtonWidget> ChoiceButtonWidgetClass;

private:
	UPROPERTY()
	int32 CityId = INDEX_NONE;

	UPROPERTY()
	EConquestCityProductionPanelTab ActiveProductionPanelTab = EConquestCityProductionPanelTab::Units;

	void BuildBuildingButtons();
	void BuildBuildingChoices(AConquestGameState& GS);
	void BuildUnitChoices(AConquestGameState& GS);
	void ApplyCivilisationTheme(const FCityState& City, const AConquestGameState& GS);
	void BindToGameState();
	void UnbindFromGameState();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleUnitsButtonClicked();

	UFUNCTION()
	void HandleBuildingsButtonClicked();

	UFUNCTION()
	void HandleProductionChoiceClicked(const FConquestChoiceButtonData& ChoiceData);

	UFUNCTION()
	void HandleCityChanged(int32 ChangedCityId);

	UFUNCTION()
	void HandleConquestStateChanged();
};
