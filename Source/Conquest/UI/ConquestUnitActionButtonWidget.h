#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestUnitActionButtonWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConquestUnitActionClicked, FName, ActionId);

UCLASS()
class CONQUEST_API UConquestUnitActionButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable)
	FOnConquestUnitActionClicked OnUnitActionClicked;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void SetupUnitActionButton(FName InActionId, const FText& InTitle);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void SetActionEnabled(bool bEnabled);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText = nullptr;

private:
	UPROPERTY()
	FName ActionId = NAME_None;

	UFUNCTION()
	void HandleClicked();
};
