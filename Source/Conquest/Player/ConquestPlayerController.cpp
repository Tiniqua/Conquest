#include "ConquestPlayerController.h"

AConquestPlayerController::AConquestPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	DefaultMouseCursor = EMouseCursor::Default;
}

void AConquestPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());
}