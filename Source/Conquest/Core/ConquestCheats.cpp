#include "Conquest/Core/ConquestCheats.h"

#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "Conquest/World/Generation/ModularHexGridActor.h"

void UConquestCheats::revealFOW()
{
	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOuter());
	AConquestGameState* ConquestGS = ConquestPC && ConquestPC->GetWorld()
		? ConquestPC->GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	AModularHexGridActor* GridActor = ConquestGS ? ConquestGS->ActiveGridActor.Get() : nullptr;
	if (!GridActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("revealFOW failed: no active grid actor."));
		return;
	}

	const FHexGridModel& GridModel = GridActor->GetGridModel();
	for (int32 R = 0; R < GridModel.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < GridModel.GetGridWidth(); ++Q)
		{
			if (GridModel.IsValidTile(Q, R))
			{
				GridActor->RevealFogOfWarAroundTile(FIntPoint(Q, R), 0);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("revealFOW complete."));
}

void UConquestCheats::ImproveAllResources()
{
	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOuter());
	if (!ConquestPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ImproveAllResources failed: no Conquest player controller."));
		return;
	}

	ConquestPC->RequestCheatImproveAllResources();
}

void UConquestCheats::granttech(const FString& TechId)
{
	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOuter());
	if (!ConquestPC || TechId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("granttech failed: expected granttech <TechId>."));
		return;
	}

	ConquestPC->RequestCheatGrantTech(FName(*TechId));
}

void UConquestCheats::grantphil(const FString& PhilosophyId)
{
	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOuter());
	if (!ConquestPC || PhilosophyId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("grantphil failed: expected grantphil <PhilosophyId>."));
		return;
	}

	ConquestPC->RequestCheatGrantPhilosophy(FName(*PhilosophyId));
}

void UConquestCheats::spawnunit(const FString& UnitId)
{
	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOuter());
	if (!ConquestPC || UnitId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("spawnunit failed: expected spawnunit <UnitId>."));
		return;
	}

	FIntPoint HoveredTile = FIntPoint::ZeroValue;
	if (!GetHoveredTile(HoveredTile))
	{
		UE_LOG(LogTemp, Warning, TEXT("spawnunit failed: no hovered tile under cursor."));
		return;
	}

	ConquestPC->RequestCheatSpawnUnit(FName(*UnitId), HoveredTile);
}

bool UConquestCheats::GetHoveredTile(FIntPoint& OutCoord) const
{
	AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOuter());
	if (!ConquestPC)
	{
		return false;
	}

	FHitResult HitResult;
	if (!ConquestPC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		return false;
	}

	AModularHexGridActor* GridActor = Cast<AModularHexGridActor>(HitResult.GetActor());
	if (!GridActor && HitResult.GetComponent())
	{
		GridActor = Cast<AModularHexGridActor>(HitResult.GetComponent()->GetOwner());
	}

	if (!GridActor)
	{
		return false;
	}

	FHexTileData TileData;
	return GridActor->GetTileAtWorldLocation(HitResult.ImpactPoint, OutCoord.X, OutCoord.Y, TileData);
}
