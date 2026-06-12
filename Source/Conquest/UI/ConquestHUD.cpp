#include "ConquestHUD.h"

#include "Blueprint/UserWidget.h"
#include "ConquestHUDWidget.h"
#include "ConquestGameSetupWidget.h"
#include "ConquestGameWidget.h"
#include "ConquestMainMenuWidget.h"
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

	NewGridActor->SetMapTypePreset(MapPreset);

	UGameplayStatics::FinishSpawningActor(NewGridActor, HexGridSpawnTransform);

	SpawnedHexGridActor = NewGridActor;

	ShowGame();
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