#include "ConquestGameMode.h"

#include "Conquest/Player/ConquestPawn.h"
#include "Conquest/Player/ConquestPlayerController.h"


AConquestGameMode::AConquestGameMode()
{
	DefaultPawnClass = AConquestPawn::StaticClass();
	PlayerControllerClass = AConquestPlayerController::StaticClass();
}
