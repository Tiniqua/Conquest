#include "ConquestHUD.h"

#include "ConquestCityPanelWidget.h"
#include "Blueprint/UserWidget.h"
#include "ConquestHUDWidget.h"
#include "ConquestGameSetupWidget.h"
#include "ConquestGameWidget.h"
#include "ConquestMainMenuWidget.h"
#include "ConquestResearchPanelWidget.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/World/Generation/ModularHexGridActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AConquestHUD::AConquestHUD()
{
	HUDWidgetClass = UConquestHUDWidget::StaticClass();
	MainMenuWidgetClass = UConquestMainMenuWidget::StaticClass();
	GameSetupWidgetClass = UConquestGameSetupWidget::StaticClass();
	GameWidgetClass = UConquestGameWidget::StaticClass();
	HexGridActorClass = AModularHexGridActor::StaticClass();
}

void AConquestHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (!HUDWidgetClass)
	{
		return;
	}

	HUDWidget = CreateWidget<UConquestHUDWidget>(PlayerController, HUDWidgetClass);
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->InitializeHUDWidget(
		this,
		MainMenuWidgetClass,
		GameSetupWidgetClass,
		GameWidgetClass
	);

	if (CityPanelWidgetClass)
	{
		CityPanelWidget = CreateWidget<UConquestCityPanelWidget>(PlayerController, CityPanelWidgetClass);
		if (CityPanelWidget)
		{
			CityPanelWidget->AddToViewport(5);
			CityPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (ResearchPanelWidgetClass)
	{
		ResearchPanelWidget = CreateWidget<UConquestResearchPanelWidget>(PlayerController, ResearchPanelWidgetClass);
		if (ResearchPanelWidget)
		{
			ResearchPanelWidget->AddToViewport(6);
			ResearchPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	ShowMainMenu();
}

void AConquestHUD::ShowMainMenu()
{
	ConfigureMenuInputMode();

	if (HUDWidget)
	{
		HUDWidget->ShowMainMenu();
	}
}

void AConquestHUD::ShowCityPanel(int32 CityId)
{
	if (!CityPanelWidget)
	{
		return;
	}

	CityPanelWidget->SetCity(CityId);
	CityPanelWidget->SetVisibility(ESlateVisibility::Visible);
}

void AConquestHUD::HideCityPanel()
{
	if (CityPanelWidget)
	{
		CityPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AConquestHUD::ShowResearchPanel()
{
	if (!ResearchPanelWidget)
	{
		return;
	}

	ResearchPanelWidget->RefreshResearchOptions();
	ResearchPanelWidget->SetVisibility(ESlateVisibility::Visible);
}

void AConquestHUD::HideResearchPanel()
{
	if (ResearchPanelWidget)
	{
		ResearchPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AConquestHUD::ShowGameSetup()
{
	ConfigureMenuInputMode();

	if (HUDWidget)
	{
		HUDWidget->ShowGameSetup();
	}
}

void AConquestHUD::ShowGame()
{
	ConfigureGameInputMode();

	if (HUDWidget)
	{
		HUDWidget->ShowGame();
	}
}

void AConquestHUD::RequestStartGame(const FConquestGameSetupSettings& SetupSettings)
{
	UWorld* World = GetWorld();
	if (!World || !HexGridActorClass)
	{
		return;
	}

	if (SpawnedHexGridActor)
	{
		SpawnedHexGridActor->Destroy();
		SpawnedHexGridActor = nullptr;
	}

	AModularHexGridActor* NewGridActor = World->SpawnActorDeferred<AModularHexGridActor>(
		HexGridActorClass,
		HexGridSpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!NewGridActor)
	{
		return;
	}

	NewGridActor->ApplyGameSetupSettings(SetupSettings);

	UGameplayStatics::FinishSpawningActor(NewGridActor, HexGridSpawnTransform);

	SpawnedHexGridActor = NewGridActor;
	if (AConquestGameState* ConquestGS = World->GetGameState<AConquestGameState>())
	{
		ConquestGS->ActiveGridActor = SpawnedHexGridActor;
	}

	if (AConquestGameMode* ConquestGM = World->GetAuthGameMode<AConquestGameMode>())
	{
		ConquestGM->StartSinglePlayerGame();
	}

	ShowGame();
}

UConquestGameWidget* AConquestHUD::GetGameWidget() const
{
	return HUDWidget ? HUDWidget->GetGameWidget() : nullptr;
}

UConquestGameWidget* AConquestHUD::GetActiveGameWidget() const
{
	if (!HUDWidget || !HUDWidget->IsGameWidgetActive())
	{
		return nullptr;
	}

	return HUDWidget->GetGameWidget();
}

void AConquestHUD::StartGameWithMapPreset(EHexMapTypePreset MapPreset)
{
	FConquestGameSetupSettings SetupSettings;
	SetupSettings.MapTypePreset = MapPreset;

	RequestStartGame(SetupSettings);
}

void AConquestHUD::ConfigureMenuInputMode()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}

void AConquestHUD::ConfigureGameInputMode()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	PlayerController->SetInputMode(InputMode);
}