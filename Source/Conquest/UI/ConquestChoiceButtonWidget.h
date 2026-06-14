#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "ConquestChoiceButtonWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnConquestChoiceClicked,
	const FConquestChoiceButtonData&,
	ChoiceData
);

UCLASS()
class CONQUEST_API UConquestChoiceButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable)
	FOnConquestChoiceClicked OnChoiceClicked;

	UFUNCTION(BlueprintCallable)
	void SetupChoiceButton(const FConquestChoiceButtonData& InChoiceData);

	UFUNCTION(BlueprintPure)
	const FConquestChoiceButtonData& GetChoiceData() const
	{
		return ChoiceData;
	}

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button = nullptr;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailText = nullptr;

private:
	UPROPERTY()
	FConquestChoiceButtonData ChoiceData;

	UFUNCTION()
	void HandleClicked();

	void RefreshVisuals();
};