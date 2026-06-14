#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "ConquestResearchPanelWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UConquestChoiceButtonWidget;

UCLASS()
class CONQUEST_API UConquestResearchPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void RefreshResearchOptions();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentResearchText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> TechButtonBox;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestChoiceButtonWidget> ChoiceButtonWidgetClass;

private:
	UFUNCTION()
	void HandleResearchChoiceClicked(const FConquestChoiceButtonData& ChoiceData);

	void HandleTechClicked(FName TechId);
};