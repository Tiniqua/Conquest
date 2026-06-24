#include "ConquestPlayerController.h"

#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Net/UnrealNetwork.h"

namespace
{
	AConquestGameMode* GetConquestGameMode(const AConquestPlayerController* PlayerController)
	{
		return PlayerController && PlayerController->GetWorld()
			? PlayerController->GetWorld()->GetAuthGameMode<AConquestGameMode>()
			: nullptr;
	}
}

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

void AConquestPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AConquestPlayerController, AssignedPlayerId);
}

void AConquestPlayerController::SetAssignedPlayerId(int32 NewPlayerId)
{
	if (HasAuthority())
	{
		AssignedPlayerId = NewPlayerId;
	}
}

void AConquestPlayerController::RequestEndTurn()
{
	if (HasAuthority())
	{
		ServerRequestEndTurn_Implementation();
	}
	else
	{
		ServerRequestEndTurn();
	}
}

void AConquestPlayerController::RequestReturnToMainMenu()
{
	if (HasAuthority())
	{
		ServerRequestReturnToMainMenu_Implementation();
	}
	else
	{
		ServerRequestReturnToMainMenu();
	}
}

void AConquestPlayerController::RequestRegenerateFirstTurnMap()
{
	if (HasAuthority())
	{
		ServerRequestRegenerateFirstTurnMap_Implementation();
	}
	else
	{
		ServerRequestRegenerateFirstTurnMap();
	}
}

void AConquestPlayerController::RequestSetCurrentResearch(FName TechId)
{
	if (HasAuthority())
	{
		ServerRequestSetCurrentResearch_Implementation(TechId);
	}
	else
	{
		ServerRequestSetCurrentResearch(TechId);
	}
}

void AConquestPlayerController::RequestSetCityProduction(int32 CityId, ECityProductionType ProductionType, FName ProductionId)
{
	if (HasAuthority())
	{
		ServerRequestSetCityProduction_Implementation(CityId, ProductionType, ProductionId);
	}
	else
	{
		ServerRequestSetCityProduction(CityId, ProductionType, ProductionId);
	}
}

void AConquestPlayerController::RequestClaimExpansionTile(int32 CityId, FIntPoint Coord)
{
	if (HasAuthority())
	{
		ServerRequestClaimExpansionTile_Implementation(CityId, Coord);
	}
	else
	{
		ServerRequestClaimExpansionTile(CityId, Coord);
	}
}

void AConquestPlayerController::RequestPurchaseTileImprovement(FIntPoint Coord, FName ImprovementId)
{
	if (HasAuthority())
	{
		ServerRequestPurchaseTileImprovement_Implementation(Coord, ImprovementId);
	}
	else
	{
		ServerRequestPurchaseTileImprovement(Coord, ImprovementId);
	}
}

void AConquestPlayerController::RequestFoundStartingCity(FIntPoint Coord, FName CityName)
{
	if (HasAuthority())
	{
		ServerRequestFoundStartingCity_Implementation(Coord, CityName);
	}
	else
	{
		ServerRequestFoundStartingCity(Coord, CityName);
	}
}

void AConquestPlayerController::RequestMoveUnit(int32 UnitInstanceId, FIntPoint TargetCoord)
{
	if (HasAuthority())
	{
		ServerRequestMoveUnit_Implementation(UnitInstanceId, TargetCoord);
	}
	else
	{
		ServerRequestMoveUnit(UnitInstanceId, TargetCoord);
	}
}

void AConquestPlayerController::RequestUnitAction(int32 UnitInstanceId, FName ActionId)
{
	if (HasAuthority())
	{
		ServerRequestUnitAction_Implementation(UnitInstanceId, ActionId);
	}
	else
	{
		ServerRequestUnitAction(UnitInstanceId, ActionId);
	}
}

void AConquestPlayerController::RequestApplyUnitAugment(int32 UnitInstanceId, FName AugmentId)
{
	if (HasAuthority())
	{
		ServerRequestApplyUnitAugment_Implementation(UnitInstanceId, AugmentId);
	}
	else
	{
		ServerRequestApplyUnitAugment(UnitInstanceId, AugmentId);
	}
}

void AConquestPlayerController::RequestAttackUnit(int32 AttackerUnitInstanceId, int32 DefenderUnitInstanceId)
{
	if (HasAuthority())
	{
		ServerRequestAttackUnit_Implementation(AttackerUnitInstanceId, DefenderUnitInstanceId);
	}
	else
	{
		ServerRequestAttackUnit(AttackerUnitInstanceId, DefenderUnitInstanceId);
	}
}

void AConquestPlayerController::RequestAttackCity(int32 AttackerUnitInstanceId, int32 DefenderCityId)
{
	if (HasAuthority())
	{
		ServerRequestAttackCity_Implementation(AttackerUnitInstanceId, DefenderCityId);
	}
	else
	{
		ServerRequestAttackCity(AttackerUnitInstanceId, DefenderCityId);
	}
}

void AConquestPlayerController::RequestSetLobbySlotCivilisation(
	int32 SlotIndex,
	UConquestCivilisationData* Civilisation
)
{
	if (HasAuthority())
	{
		ServerRequestSetLobbySlotCivilisation_Implementation(SlotIndex, Civilisation);
	}
	else
	{
		ServerRequestSetLobbySlotCivilisation(SlotIndex, Civilisation);
	}
}

void AConquestPlayerController::RequestSetLobbyReady(bool bReady)
{
	if (HasAuthority())
	{
		ServerRequestSetLobbyReady_Implementation(bReady);
	}
	else
	{
		ServerRequestSetLobbyReady(bReady);
	}
}

void AConquestPlayerController::ServerRequestEndTurn_Implementation()
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->EndTurnForPlayer(AssignedPlayerId);
	}
}

void AConquestPlayerController::ServerRequestReturnToMainMenu_Implementation()
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ResetGameToMainMenu();
	}
}

void AConquestPlayerController::ServerRequestRegenerateFirstTurnMap_Implementation()
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->RegenerateFirstTurnMapForPlayer(AssignedPlayerId);
	}
}

void AConquestPlayerController::ServerRequestSetCurrentResearch_Implementation(FName TechId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetCurrentResearchForPlayer(AssignedPlayerId, TechId);
	}
}

void AConquestPlayerController::ServerRequestSetCityProduction_Implementation(
	int32 CityId,
	ECityProductionType ProductionType,
	FName ProductionId
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetCityProductionForPlayer(AssignedPlayerId, CityId, ProductionType, ProductionId);
	}
}

void AConquestPlayerController::ServerRequestClaimExpansionTile_Implementation(int32 CityId, FIntPoint Coord)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ClaimExpansionTileForPlayer(AssignedPlayerId, CityId, Coord);
	}
}

void AConquestPlayerController::ServerRequestPurchaseTileImprovement_Implementation(FIntPoint Coord, FName ImprovementId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->PurchaseTileImprovementForPlayer(AssignedPlayerId, Coord, ImprovementId);
	}
}

void AConquestPlayerController::ServerRequestFoundStartingCity_Implementation(FIntPoint Coord, FName CityName)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->FoundStartingCityForPlayer(AssignedPlayerId, Coord, CityName);
	}
}

void AConquestPlayerController::ServerRequestMoveUnit_Implementation(int32 UnitInstanceId, FIntPoint TargetCoord)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->MoveUnitForPlayer(AssignedPlayerId, UnitInstanceId, TargetCoord);
	}
}

void AConquestPlayerController::ServerRequestUnitAction_Implementation(int32 UnitInstanceId, FName ActionId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ApplyUnitActionForPlayer(AssignedPlayerId, UnitInstanceId, ActionId);
	}
}

void AConquestPlayerController::ServerRequestApplyUnitAugment_Implementation(int32 UnitInstanceId, FName AugmentId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ApplyUnitAugmentForPlayer(AssignedPlayerId, UnitInstanceId, AugmentId);
	}
}

void AConquestPlayerController::ServerRequestAttackUnit_Implementation(
	int32 AttackerUnitInstanceId,
	int32 DefenderUnitInstanceId
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->AttackUnitForPlayer(AssignedPlayerId, AttackerUnitInstanceId, DefenderUnitInstanceId);
	}
}

void AConquestPlayerController::ServerRequestAttackCity_Implementation(
	int32 AttackerUnitInstanceId,
	int32 DefenderCityId
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->AttackCityForPlayer(AssignedPlayerId, AttackerUnitInstanceId, DefenderCityId);
	}
}

void AConquestPlayerController::ServerRequestSetLobbySlotCivilisation_Implementation(
	int32 SlotIndex,
	UConquestCivilisationData* Civilisation
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetLobbySlotCivilisationForPlayer(AssignedPlayerId, SlotIndex, Civilisation);
	}
}

void AConquestPlayerController::ServerRequestSetLobbyReady_Implementation(bool bReady)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetLobbyReadyForPlayer(AssignedPlayerId, bReady);
	}
}
