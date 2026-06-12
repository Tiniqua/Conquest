#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestMainMenuWidget.generated.h"

class UButton;
class UConquestHUDWidget;

UCLASS()
class CONQUEST_API UConquestMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Main Menu")
	void InitializeMainMenuWidget(UConquestHUDWidget* InParentHUDWidget);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartGameButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> ParentHUDWidget = nullptr;

	UFUNCTION()
	void HandleStartGameButtonClicked();
};