#include "ConquestGameMode.h"

#include "Conquest/Player/ConquestPawn.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/UI/ConquestHUD.h"


AConquestGameMode::AConquestGameMode()
{
	DefaultPawnClass = AConquestPawn::StaticClass();
	PlayerControllerClass = AConquestPlayerController::StaticClass();
	HUDClass = AConquestHUD::StaticClass();
}
