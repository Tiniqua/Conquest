#include "ConquestHUDWidget.h"

#include "ConquestHUD.h"
#include "ConquestGameSetupWidget.h"
#include "ConquestGameWidget.h"
#include "ConquestMainMenuWidget.h"

void UConquestHUDWidget::InitializeHUDWidget(
	AConquestHUD* InOwningHUD,
	TSubclassOf<UConquestMainMenuWidget> InMainMenuWidgetClass,
	TSubclassOf<UConquestGameSetupWidget> InGameSetupWidgetClass,
	TSubclassOf<UConquestGameWidget> InGameWidgetClass
)
{
	OwningConquestHUD = InOwningHUD;
	MainMenuWidgetClass = InMainMenuWidgetClass;
	GameSetupWidgetClass = InGameSetupWidgetClass;
	GameWidgetClass = InGameWidgetClass;
}

void UConquestHUDWidget::ShowMainMenu()
{
	HideCurrentScreen();

	if (!MainMenuWidget && MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UConquestMainMenuWidget>(GetOwningPlayer(), MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->InitializeMainMenuWidget(this);
		}
	}

	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport(10);
		CurrentScreenWidget = MainMenuWidget;
	}
}

void UConquestHUDWidget::ShowGameSetup()
{
	HideCurrentScreen();

	if (!GameSetupWidget && GameSetupWidgetClass)
	{
		GameSetupWidget = CreateWidget<UConquestGameSetupWidget>(GetOwningPlayer(), GameSetupWidgetClass);
		if (GameSetupWidget)
		{
			GameSetupWidget->InitializeGameSetupWidget(this);
		}
	}

	if (GameSetupWidget)
	{
		GameSetupWidget->RefreshMapPresetOptions();
		GameSetupWidget->AddToViewport(10);
		CurrentScreenWidget = GameSetupWidget;
	}
}

void UConquestHUDWidget::ShowGame()
{
	HideCurrentScreen();

	if (!GameWidget && GameWidgetClass)
	{
		GameWidget = CreateWidget<UConquestGameWidget>(GetOwningPlayer(), GameWidgetClass);
	}

	if (GameWidget)
	{
		GameWidget->AddToViewport(5);
		CurrentScreenWidget = GameWidget;
	}
}

void UConquestHUDWidget::RequestStartGame(EHexMapTypePreset MapPreset)
{
	if (OwningConquestHUD)
	{
		OwningConquestHUD->StartGameWithMapPreset(MapPreset);
	}
}

bool UConquestHUDWidget::IsGameWidgetActive() const
{
	return CurrentScreenWidget == GameWidget && GameWidget != nullptr;
}

void UConquestHUDWidget::HideCurrentScreen()
{
	if (CurrentScreenWidget)
	{
		CurrentScreenWidget->RemoveFromParent();
		CurrentScreenWidget = nullptr;
	}
}
