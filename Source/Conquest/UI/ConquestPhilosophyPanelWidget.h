#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "ConquestPhilosophyPanelWidget.generated.h"

class UButton;
class UConquestChoiceButtonWidget;
class UTextBlock;
class UVerticalBox;

UCLASS()
class CONQUEST_API UConquestPhilosophyPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Philosophy")
	void RefreshPhilosophyOptions();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CultureStatusText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AdoptedPhilosophyText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> PhilosophyButtonBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Conquest|UI")
	TSubclassOf<UConquestChoiceButtonWidget> ChoiceButtonWidgetClass;

private:
	UFUNCTION()
	void HandlePhilosophyChoiceClicked(const FConquestChoiceButtonData& ChoiceData);

	UFUNCTION()
	void HandleCloseClicked();

	FText BuildLockedReasonText(const TArray<FName>& PrerequisiteIds) const;
};
