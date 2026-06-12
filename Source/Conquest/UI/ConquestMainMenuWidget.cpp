#include "ConquestMainMenuWidget.h"

#include "Components/Button.h"
#include "ConquestHUDWidget.h"

void UConquestMainMenuWidget::InitializeMainMenuWidget(UConquestHUDWidget* InParentHUDWidget)
{
	ParentHUDWidget = InParentHUDWidget;
}

void UConquestMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartGameButton)
	{
		StartGameButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleStartGameButtonClicked);
		StartGameButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleStartGameButtonClicked);
	}
}

void UConquestMainMenuWidget::HandleStartGameButtonClicked()
{
	if (ParentHUDWidget)
	{
		ParentHUDWidget->ShowGameSetup();
	}
}