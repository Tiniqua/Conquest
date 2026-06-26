#include "Conquest/Managers/ConquestPhilosophyManager.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestModifierManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Philosophies/ConquestPhilosophyTypes.h"

void UConquestPhilosophyManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

TArray<FName> UConquestPhilosophyManager::GetAvailablePhilosophyIds(int32 PlayerId) const
{
	TArray<FName> Result;
	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return Result;
	}

	TArray<const FConquestPhilosophyRow*> AllPhilosophies;
	GameStateRef->ContentManager->GetAllPhilosophies(AllPhilosophies);

	for (const FConquestPhilosophyRow* PhilosophyRow : AllPhilosophies)
	{
		if (PhilosophyRow && CanAdoptPhilosophy(PlayerId, PhilosophyRow->PhilosophyId))
		{
			Result.Add(PhilosophyRow->PhilosophyId);
		}
	}

	return Result;
}

bool UConquestPhilosophyManager::CanAdoptPhilosophy(int32 PlayerId, FName PhilosophyId) const
{
	if (!GameStateRef || !GameStateRef->ContentManager || PhilosophyId.IsNone())
	{
		return false;
	}

	const FConquestPhilosophyRow* PhilosophyRow = GameStateRef->ContentManager->FindPhilosophy(PhilosophyId);
	if (!PhilosophyRow)
	{
		return false;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	if (Player.PlayerId != PlayerId || Player.HasAdoptedPhilosophy(PhilosophyId))
	{
		return false;
	}

	for (const FName PrereqId : PhilosophyRow->PrerequisitePhilosophyIds)
	{
		if (!PrereqId.IsNone() && !Player.HasAdoptedPhilosophy(PrereqId))
		{
			return false;
		}
	}

	return true;
}

int32 UConquestPhilosophyManager::GetNextPhilosophyCultureCost(int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return 0;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	const int32 AdoptedCount = FMath::Max(Player.AdoptedPhilosophyIds.Num(), Player.AdoptedPhilosophyTenets);
	const float CostMultiplier = FMath::Pow(
		1.0f + FMath::Max(0.0f, GameStateRef->PhilosophyCostIncreasePercentPerAdoption),
		static_cast<float>(AdoptedCount)
	);
	const float BaseCost = FMath::Max(
		0.0f,
		static_cast<float>(GameStateRef->BasePhilosophyCultureCost) * CostMultiplier
	);

	return GameStateRef->ModifierManager
		? GameStateRef->ModifierManager->CalculatePhilosophyCost(PlayerId, BaseCost)
		: FMath::RoundToInt(BaseCost);
}

bool UConquestPhilosophyManager::AdoptPhilosophyById(int32 PlayerId, FName PhilosophyId)
{
	if (!GameStateRef || !GameStateRef->ContentManager || !CanAdoptPhilosophy(PlayerId, PhilosophyId))
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	if (Player.PlayerId != PlayerId)
	{
		return false;
	}

	const int32 CultureCost = GetNextPhilosophyCultureCost(PlayerId);
	if (Player.StoredYields.Culture < CultureCost)
	{
		return false;
	}

	Player.StoredYields.Culture -= CultureCost;
	Player.AdoptedPhilosophyIds.AddUnique(PhilosophyId);
	Player.AdoptedPhilosophyTenets = Player.AdoptedPhilosophyIds.Num();

	if (GameStateRef->CityManager)
	{
		GameStateRef->CityManager->RecalculateEmpireYields(PlayerId);
	}
	else if (GameStateRef->YieldManager)
	{
		GameStateRef->YieldManager->RecalculateEmpireYieldPerTurn(PlayerId);
	}

	OnPhilosophiesChanged.Broadcast();
	GameStateRef->BroadcastStateChanged();
	return true;
}
