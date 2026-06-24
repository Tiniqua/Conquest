#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestUnitWorldIconWidget.generated.h"

class UImage;
class UMaterialInterface;
class UProgressBar;
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
		UMaterialInterface* UnitIconMaterial,
		int32 CurrentHealth,
		int32 MaxHealth,
		int32 AttackValue
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Unit")
	void OnUnitIconChanged(
		const FText& UnitName,
		const FText& CivilisationName,
		FLinearColor UnitDisplayColor,
		UMaterialInterface* UnitIconMaterial,
		int32 CurrentHealth,
		int32 MaxHealth,
		int32 AttackValue
	);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnitNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CivilisationNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> UnitIconImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttackText = nullptr;
};
