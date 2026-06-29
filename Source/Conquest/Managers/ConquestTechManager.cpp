#include "Conquest/Managers/ConquestTechManager.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestYieldManager.h"
#include "Conquest/Tech/ConquestTechTypes.h"

void UConquestTechManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

bool UConquestTechManager::SetCurrentResearchById(int32 PlayerId, FName TechId)
{
	if (!GameStateRef || !GameStateRef->ContentManager || TechId.IsNone())
	{
		return false;
	}

	if (!CanResearchTech(PlayerId, TechId))
	{
		return false;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);
	CacheCurrentResearchProgress(Player);

	Player.CurrentResearchId = TechId;
	Player.CurrentResearchProgress = GetCachedResearchProgress(Player, TechId);

	OnResearchChanged.Broadcast();
	GameStateRef->BroadcastStateChangedWithVisuals(EConquestStateVisualDirtyFlags::None);

	return true;
}

void UConquestTechManager::ProcessResearchAtStartOfTurn(int32 PlayerId)
{
	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return;
	}

	FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpireMutable(PlayerId);

	if (Player.CurrentResearchId.IsNone())
	{
		return;
	}

	const FConquestTechRow* CurrentResearch =
		GameStateRef->ContentManager->FindTech(Player.CurrentResearchId);

	if (!CurrentResearch)
	{
		return;
	}

	const int32 SciencePerTurn = CalculateEmpireSciencePerTurn(PlayerId);
	if (SciencePerTurn <= 0)
	{
		return;
	}

	Player.CurrentResearchProgress += SciencePerTurn;

	if (Player.CurrentResearchProgress >= CurrentResearch->ScienceCost)
	{
		Player.ResearchedTechIds.AddUnique(CurrentResearch->TechId);
		ClearCachedResearchProgress(Player, CurrentResearch->TechId);
		Player.CurrentResearchId = NAME_None;
		Player.CurrentResearchProgress = 0.0f;

		if (GameStateRef->CityManager)
		{
			GameStateRef->CityManager->RecalculateEmpireYields(PlayerId);
		}
	}

	OnResearchChanged.Broadcast();
	GameStateRef->BroadcastStateChangedWithVisuals(EConquestStateVisualDirtyFlags::None);
}

TArray<FName> UConquestTechManager::GetAvailableResearchIds(int32 PlayerId) const
{
	TArray<FName> Result;

	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return Result;
	}

	TArray<const FConquestTechRow*> AllTechs;
	GameStateRef->ContentManager->GetAllTechs(AllTechs);

	for (const FConquestTechRow* TechRow : AllTechs)
	{
		if (!TechRow)
		{
			continue;
		}

		if (CanResearchTech(PlayerId, TechRow->TechId))
		{
			Result.Add(TechRow->TechId);
		}
	}

	return Result;
}

int32 UConquestTechManager::EstimateTurnsToResearchById(int32 PlayerId, FName TechId) const
{
	if (!GameStateRef || !GameStateRef->ContentManager || TechId.IsNone())
	{
		return INDEX_NONE;
	}

	const FConquestTechRow* TechRow =
		GameStateRef->ContentManager->FindTech(TechId);

	if (!TechRow)
	{
		return INDEX_NONE;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	const int32 SciencePerTurn = CalculateEmpireSciencePerTurn(PlayerId);

	if (SciencePerTurn <= 0)
	{
		return INDEX_NONE;
	}

	const float CurrentProgress =
		Player.CurrentResearchId == TechId
			? Player.CurrentResearchProgress
			: GetCachedResearchProgress(Player, TechId);

	const float Remaining = FMath::Max(
		0.0f,
		static_cast<float>(TechRow->ScienceCost) - CurrentProgress
	);

	return FMath::Max(1, FMath::CeilToInt(Remaining / SciencePerTurn));
}

const FConquestTechRow* UConquestTechManager::GetCurrentResearchRow(int32 PlayerId) const
{
	if (!GameStateRef || !GameStateRef->ContentManager)
	{
		return nullptr;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);

	if (Player.CurrentResearchId.IsNone())
	{
		return nullptr;
	}

	return GameStateRef->ContentManager->FindTech(Player.CurrentResearchId);
}

int32 UConquestTechManager::CalculateEmpireSciencePerTurn(int32 PlayerId) const
{
	if (!GameStateRef)
	{
		return 0;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);
	if (Player.PlayerId != PlayerId)
	{
		return 0;
	}

	if (GameStateRef->YieldManager)
	{
		GameStateRef->YieldManager->RecalculateEmpireYieldPerTurn(PlayerId);
	}

	return GameStateRef->GetPlayerEmpire(PlayerId).CachedYieldPerTurn.Science;
}

bool UConquestTechManager::CanResearchTech(int32 PlayerId, FName TechId) const
{
	if (!GameStateRef || !GameStateRef->ContentManager || TechId.IsNone())
	{
		return false;
	}

	const FConquestTechRow* TechRow =
		GameStateRef->ContentManager->FindTech(TechId);

	if (!TechRow)
	{
		return false;
	}

	const FConquestPlayerEmpireState& Player = GameStateRef->GetPlayerEmpire(PlayerId);

	if (Player.HasResearched(TechId))
	{
		return false;
	}

	if (Player.CurrentResearchId == TechId)
	{
		return false;
	}

	for (const FName PrereqId : TechRow->PrerequisiteTechIds)
	{
		if (!PrereqId.IsNone() && !Player.HasResearched(PrereqId))
		{
			return false;
		}
	}

	return true;
}

void UConquestTechManager::CacheCurrentResearchProgress(FConquestPlayerEmpireState& Player) const
{
	if (Player.CurrentResearchId.IsNone() || Player.CurrentResearchProgress <= 0.0f)
	{
		return;
	}

	FConquestResearchProgressCacheEntry* CacheEntry =
		Player.ResearchProgressCache.FindByPredicate([&Player](const FConquestResearchProgressCacheEntry& Entry)
		{
			return Entry.TechId == Player.CurrentResearchId;
		});

	if (!CacheEntry)
	{
		CacheEntry = &Player.ResearchProgressCache.AddDefaulted_GetRef();
		CacheEntry->TechId = Player.CurrentResearchId;
	}

	CacheEntry->Progress = FMath::Max(0.0f, Player.CurrentResearchProgress);
}

float UConquestTechManager::GetCachedResearchProgress(
	const FConquestPlayerEmpireState& Player,
	FName TechId
) const
{
	if (TechId.IsNone())
	{
		return 0.0f;
	}

	const FConquestResearchProgressCacheEntry* CacheEntry =
		Player.ResearchProgressCache.FindByPredicate([TechId](const FConquestResearchProgressCacheEntry& Entry)
		{
			return Entry.TechId == TechId;
		});

	return CacheEntry ? FMath::Max(0.0f, CacheEntry->Progress) : 0.0f;
}

void UConquestTechManager::ClearCachedResearchProgress(FConquestPlayerEmpireState& Player, FName TechId) const
{
	if (TechId.IsNone())
	{
		return;
	}

	Player.ResearchProgressCache.RemoveAll([TechId](const FConquestResearchProgressCacheEntry& Entry)
	{
		return Entry.TechId == TechId;
	});
}
