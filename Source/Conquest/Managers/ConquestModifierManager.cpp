#include "Conquest/Managers/ConquestModifierManager.h"

#include "Conquest/Buildings/ConquestBuildingTypes.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Philosophies/ConquestPhilosophyTypes.h"
#include "Conquest/Tech/ConquestTechTypes.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexImprovementTypes.h"
#include "Conquest/World/Generation/HexResourceSetData.h"
#include "Conquest/World/Generation/HexResourceTypes.h"
#include "Conquest/World/Generation/HexTileTypes.h"

namespace
{
	const FName ConquestUnhappyAttackModifierId(TEXT("Happiness_Unhappy_Attack"));
	const FName ConquestUnhappyDefenseModifierId(TEXT("Happiness_Unhappy_Defense"));
	const FName ConquestTileDefenseModifierId(TEXT("Tile_Defense_Modifier"));

	struct FConquestResolvedModifier
	{
		FConquestModifierDefinition Definition;
		EConquestModifierSourceType SourceType = EConquestModifierSourceType::Unknown;
		FName SourceId = NAME_None;
		int32 SourceCityId = INDEX_NONE;
		int32 SourceUnitInstanceId = INDEX_NONE;
		FIntPoint SourceTileCoord = FIntPoint::ZeroValue;
		bool bHasSourceTileCoord = false;
	};

	bool MatchesNameList(FName Value, FName SingleId, const TArray<FName>& Ids)
	{
		if (Value.IsNone())
		{
			return false;
		}

		if (Ids.Num() > 0)
		{
			return Ids.Contains(Value);
		}

		return !SingleId.IsNone() && Value == SingleId;
	}

	EConquestModifierAttribute YieldAttributeFromIndex(int32 Index)
	{
		switch (Index)
		{
		case 0:
			return EConquestModifierAttribute::Food;
		case 1:
			return EConquestModifierAttribute::Production;
		case 2:
			return EConquestModifierAttribute::Gold;
		case 3:
			return EConquestModifierAttribute::Science;
		case 4:
			return EConquestModifierAttribute::Culture;
		default:
			return EConquestModifierAttribute::None;
		}
	}

	int32 GetYieldByIndex(const FHexYield& Yield, int32 Index)
	{
		switch (Index)
		{
		case 0:
			return Yield.Food;
		case 1:
			return Yield.Production;
		case 2:
			return Yield.Gold;
		case 3:
			return Yield.Science;
		case 4:
			return Yield.Culture;
		default:
			return 0;
		}
	}

	void SetYieldByIndex(FHexYield& Yield, int32 Index, int32 Value)
	{
		switch (Index)
		{
		case 0:
			Yield.Food = Value;
			break;
		case 1:
			Yield.Production = Value;
			break;
		case 2:
			Yield.Gold = Value;
			break;
		case 3:
			Yield.Science = Value;
			break;
		case 4:
			Yield.Culture = Value;
			break;
		default:
			break;
		}
	}

	void AddResolvedModifier(
		TArray<FConquestResolvedModifier>& OutModifiers,
		const FConquestModifierDefinition& Definition,
		EConquestModifierSourceType SourceType,
		FName SourceId,
		int32 SourceCityId = INDEX_NONE,
		int32 SourceUnitInstanceId = INDEX_NONE,
		const FIntPoint* SourceTileCoord = nullptr
	)
	{
		if (Definition.Attribute == EConquestModifierAttribute::None)
		{
			return;
		}

		FConquestResolvedModifier& Modifier = OutModifiers.AddDefaulted_GetRef();
		Modifier.Definition = Definition;
		Modifier.SourceType = SourceType;
		Modifier.SourceId = SourceId;
		Modifier.SourceCityId = SourceCityId;
		Modifier.SourceUnitInstanceId = SourceUnitInstanceId;
		if (SourceTileCoord)
		{
			Modifier.SourceTileCoord = *SourceTileCoord;
			Modifier.bHasSourceTileCoord = true;
		}
	}

	FConquestModifierDefinition MakeModifier(
		FName ModifierId,
		EConquestModifierTargetScope TargetScope,
		EConquestModifierAttribute Attribute,
		EConquestModifierOperation Operation,
		float Value
	)
	{
		FConquestModifierDefinition Definition;
		Definition.ModifierId = ModifierId;
		Definition.TargetScope = TargetScope;
		Definition.Attribute = Attribute;
		Definition.Operation = Operation;
		Definition.Value = Value;
		return Definition;
	}

	void AddYieldModifiers(
		TArray<FConquestResolvedModifier>& OutModifiers,
		const FHexYield& Yield,
		EConquestModifierTargetScope TargetScope,
		FName SourceId,
		EConquestModifierSourceType SourceType,
		int32 SourceCityId = INDEX_NONE,
		const FIntPoint* SourceTileCoord = nullptr
	)
	{
		for (int32 YieldIndex = 0; YieldIndex < 5; ++YieldIndex)
		{
			const int32 YieldValue = GetYieldByIndex(Yield, YieldIndex);
			if (YieldValue == 0)
			{
				continue;
			}

			const EConquestModifierAttribute Attribute = YieldAttributeFromIndex(YieldIndex);
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, TargetScope, Attribute, EConquestModifierOperation::AddFlat, YieldValue),
				SourceType,
				SourceId,
				SourceCityId,
				INDEX_NONE,
				SourceTileCoord
			);
		}
	}

	bool TargetScopeMatches(EConquestModifierTargetScope ModifierScope, EConquestModifierTargetScope ContextScope)
	{
		return ModifierScope == EConquestModifierTargetScope::Any || ModifierScope == ContextScope;
	}

	const FCityState* FindContextCity(const AConquestGameState* GameState, const FConquestModifierContext& Context)
	{
		return GameState && GameState->CityManager && Context.CityId != INDEX_NONE
			? GameState->CityManager->GetCity(Context.CityId)
			: nullptr;
	}

	const FConquestUnitState* FindUnitInPlayer(const FConquestPlayerEmpireState& Player, int32 UnitInstanceId)
	{
		return Player.Units.FindByPredicate([UnitInstanceId](const FConquestUnitState& Unit)
		{
			return Unit.UnitInstanceId == UnitInstanceId;
		});
	}

	const FConquestUnitState* FindContextUnit(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context
	)
	{
		if (!GameState || Context.UnitInstanceId == INDEX_NONE)
		{
			return nullptr;
		}

		if (Context.PlayerId != INDEX_NONE)
		{
			const FConquestPlayerEmpireState& Player = GameState->GetPlayerEmpire(Context.PlayerId);
			if (const FConquestUnitState* Unit = FindUnitInPlayer(Player, Context.UnitInstanceId))
			{
				return Unit;
			}
		}

		for (const FConquestPlayerEmpireState& Player : GameState->PlayerEmpires)
		{
			if (const FConquestUnitState* Unit = FindUnitInPlayer(Player, Context.UnitInstanceId))
			{
				return Unit;
			}
		}

		return nullptr;
	}

	const FHexTileData* FindContextTile(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context
	)
	{
		const FHexGridModel* GridModel = GameState ? GameState->GetHexGridModel() : nullptr;
		if (!GridModel)
		{
			return nullptr;
		}

		if (Context.bHasTargetTileCoord)
		{
			return GridModel->GetTile(Context.TargetTileCoord);
		}

		return Context.bHasTileCoord ? GridModel->GetTile(Context.TileCoord) : nullptr;
	}

	FName GetContextTileResourceId(const AConquestGameState* GameState, const FConquestModifierContext& Context)
	{
		if (!Context.ResourceId.IsNone())
		{
			return Context.ResourceId;
		}

		const FHexTileData* Tile = FindContextTile(GameState, Context);
		return Tile ? Tile->Resource.ResourceId : NAME_None;
	}

	void AddCityTileCoords(
		const FCityState& City,
		bool bWorkedTiles,
		TArray<FIntPoint>& OutCoords
	)
	{
		const TArray<FIntPoint>& SourceTiles = bWorkedTiles ? City.WorkedTiles : City.OwnedTiles;
		for (const FIntPoint& Coord : SourceTiles)
		{
			OutCoords.AddUnique(Coord);
		}
	}

	void GetTileCoordsForCountScope(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		EConquestModifierCountScope CountScope,
		TArray<FIntPoint>& OutCoords
	)
	{
		OutCoords.Reset();
		if (!GameState || !GameState->CityManager)
		{
			return;
		}

		const bool bContextOnly =
			CountScope == EConquestModifierCountScope::ContextCity ||
			CountScope == EConquestModifierCountScope::ContextCityWorkedTiles ||
			CountScope == EConquestModifierCountScope::ContextCityOwnedTiles;
		const bool bWorkedTiles =
			CountScope == EConquestModifierCountScope::PlayerWorkedTiles ||
			CountScope == EConquestModifierCountScope::ContextCityWorkedTiles;

		if (bContextOnly)
		{
			if (const FCityState* City = FindContextCity(GameState, Context))
			{
				AddCityTileCoords(*City, bWorkedTiles, OutCoords);
			}
			return;
		}

		for (const FCityState& City : GameState->CityManager->Cities)
		{
			if (City.OwnerPlayerId == Context.PlayerId)
			{
				AddCityTileCoords(City, bWorkedTiles, OutCoords);
			}
		}
	}

	bool CountIdMatches(FName Value, const FConquestModifierDefinition& Definition)
	{
		return
			Definition.CountIds.Num() <= 0 && Definition.CountId.IsNone()
				? true
				: MatchesNameList(Value, Definition.CountId, Definition.CountIds);
	}
}

void UConquestModifierManager::Initialize(AConquestGameState* InGameState)
{
	GameStateRef = InGameState;
}

namespace
{
	void AddRowModifiers(
		TArray<FConquestResolvedModifier>& OutModifiers,
		const TArray<FConquestModifierDefinition>& Modifiers,
		EConquestModifierSourceType SourceType,
		FName SourceId,
		int32 SourceCityId = INDEX_NONE,
		int32 SourceUnitInstanceId = INDEX_NONE,
		const FIntPoint* SourceTileCoord = nullptr
	)
	{
		for (const FConquestModifierDefinition& Modifier : Modifiers)
		{
			AddResolvedModifier(
				OutModifiers,
				Modifier,
				SourceType,
				SourceId,
				SourceCityId,
				SourceUnitInstanceId,
				SourceTileCoord
			);
		}
	}

	void AddBuildingModifiers(
		TArray<FConquestResolvedModifier>& OutModifiers,
		const FConquestBuildingRow& BuildingRow,
		int32 SourceCityId
	)
	{
		const FName SourceId = !BuildingRow.BuildingId.IsNone() ? BuildingRow.BuildingId : NAME_None;
		AddRowModifiers(OutModifiers, BuildingRow.Modifiers, EConquestModifierSourceType::Building, SourceId, SourceCityId);
		AddYieldModifiers(
			OutModifiers,
			BuildingRow.FlatCityYieldBonus,
			EConquestModifierTargetScope::City,
			SourceId,
			EConquestModifierSourceType::Building,
			SourceCityId
		);

		if (BuildingRow.HappinessBonus != 0)
		{
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(
					SourceId,
					EConquestModifierTargetScope::Empire,
					EConquestModifierAttribute::Happiness,
					EConquestModifierOperation::AddFlat,
					BuildingRow.HappinessBonus
				),
				EConquestModifierSourceType::Building,
				SourceId,
				SourceCityId
			);
		}

		for (const FConquestStrategicResourceCapBonus& CapBonus : BuildingRow.StrategicResourceCapBonuses)
		{
			if (CapBonus.ResourceId.IsNone() || CapBonus.CapBonus == 0)
			{
				continue;
			}

			FConquestModifierDefinition Definition = MakeModifier(
				SourceId,
				EConquestModifierTargetScope::StrategicResource,
				EConquestModifierAttribute::StrategicResourceCap,
				EConquestModifierOperation::AddFlat,
				CapBonus.CapBonus
			);
			FConquestModifierCondition ResourceCondition;
			ResourceCondition.Type = EConquestModifierConditionType::ResourceIs;
			ResourceCondition.Id = CapBonus.ResourceId;
			Definition.Conditions.Add(ResourceCondition);

			AddResolvedModifier(
				OutModifiers,
				Definition,
				EConquestModifierSourceType::Building,
				SourceId,
				SourceCityId
			);
		}
	}

	void AddUnitAugmentModifiers(
		TArray<FConquestResolvedModifier>& OutModifiers,
		const FConquestUnitAugmentRow& AugmentRow,
		const FConquestUnitAugmentState& AugmentState,
		int32 SourceUnitInstanceId
	)
	{
		const FName SourceId = !AugmentRow.AugmentId.IsNone() ? AugmentRow.AugmentId : AugmentState.AugmentId;
		AddRowModifiers(
			OutModifiers,
			AugmentRow.Modifiers,
			EConquestModifierSourceType::UnitAugment,
			SourceId,
			INDEX_NONE,
			SourceUnitInstanceId
		);

		switch (AugmentState.ModifiedStat)
		{
		case EConquestUnitAugmentStat::Strength:
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Unit, EConquestModifierAttribute::UnitStrength, EConquestModifierOperation::AddFlat, AugmentState.FlatBonus),
				EConquestModifierSourceType::UnitAugment,
				SourceId,
				INDEX_NONE,
				SourceUnitInstanceId
			);
			break;
		case EConquestUnitAugmentStat::AttackRange:
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Unit, EConquestModifierAttribute::UnitAttackRange, EConquestModifierOperation::AddFlat, AugmentState.FlatBonus),
				EConquestModifierSourceType::UnitAugment,
				SourceId,
				INDEX_NONE,
				SourceUnitInstanceId
			);
			break;
		case EConquestUnitAugmentStat::MaxHealth:
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Unit, EConquestModifierAttribute::UnitMaxHealth, EConquestModifierOperation::AddFlat, AugmentState.FlatBonus),
				EConquestModifierSourceType::UnitAugment,
				SourceId,
				INDEX_NONE,
				SourceUnitInstanceId
			);
			break;
		case EConquestUnitAugmentStat::HealthRegen:
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Unit, EConquestModifierAttribute::UnitHealthRegen, EConquestModifierOperation::AddFlat, AugmentState.FlatBonus),
				EConquestModifierSourceType::UnitAugment,
				SourceId,
				INDEX_NONE,
				SourceUnitInstanceId
			);
			break;
		case EConquestUnitAugmentStat::Movement:
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Unit, EConquestModifierAttribute::UnitMovementPoints, EConquestModifierOperation::AddFlat, AugmentState.FlatBonus),
				EConquestModifierSourceType::UnitAugment,
				SourceId,
				INDEX_NONE,
				SourceUnitInstanceId
			);
			break;
		case EConquestUnitAugmentStat::AttackMultiplier:
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Combat, EConquestModifierAttribute::CombatAttack, EConquestModifierOperation::AddPercent, AugmentState.MultiplierBonus),
				EConquestModifierSourceType::UnitAugment,
				SourceId,
				INDEX_NONE,
				SourceUnitInstanceId
			);
			break;
		case EConquestUnitAugmentStat::DefenseMultiplier:
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Combat, EConquestModifierAttribute::CombatDefense, EConquestModifierOperation::AddPercent, AugmentState.MultiplierBonus),
				EConquestModifierSourceType::UnitAugment,
				SourceId,
				INDEX_NONE,
				SourceUnitInstanceId
			);
			break;
		default:
			break;
		}
	}

	void AddImprovementModifiers(
		TArray<FConquestResolvedModifier>& OutModifiers,
		const FHexImprovementDefinition& Improvement,
		const FIntPoint& SourceTileCoord
	)
	{
		const FName SourceId = Improvement.ImprovementId;
		AddRowModifiers(
			OutModifiers,
			Improvement.Modifiers,
			EConquestModifierSourceType::Improvement,
			SourceId,
			INDEX_NONE,
			INDEX_NONE,
			&SourceTileCoord
		);

		if (Improvement.TileCombatStrengthBonus != 0)
		{
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::OwnedTileCombat, EConquestModifierAttribute::OwnedTileCombatStrength, EConquestModifierOperation::AddFlat, Improvement.TileCombatStrengthBonus),
				EConquestModifierSourceType::Improvement,
				SourceId,
				INDEX_NONE,
				INDEX_NONE,
				&SourceTileCoord
			);
		}

		if (Improvement.TileHealRateBonus != 0)
		{
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::OwnedTileCombat, EConquestModifierAttribute::OwnedTileHealRate, EConquestModifierOperation::AddFlat, Improvement.TileHealRateBonus),
				EConquestModifierSourceType::Improvement,
				SourceId,
				INDEX_NONE,
				INDEX_NONE,
				&SourceTileCoord
			);
		}

		if (!FMath::IsNearlyZero(Improvement.UnitDefenderModifierBonus))
		{
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::OwnedTileCombat, EConquestModifierAttribute::OwnedTileDefenderModifier, EConquestModifierOperation::AddFlat, Improvement.UnitDefenderModifierBonus),
				EConquestModifierSourceType::Improvement,
				SourceId,
				INDEX_NONE,
				INDEX_NONE,
				&SourceTileCoord
			);
		}
	}

	void AddResourceModifiers(
		TArray<FConquestResolvedModifier>& OutModifiers,
		const FHexResourceDefinition& Resource,
		const FIntPoint& SourceTileCoord
	)
	{
		const FName SourceId = Resource.ResourceId;
		AddRowModifiers(
			OutModifiers,
			Resource.Modifiers,
			EConquestModifierSourceType::Resource,
			SourceId,
			INDEX_NONE,
			INDEX_NONE,
			&SourceTileCoord
		);

		if (Resource.Category == EHexResourceCategory::Luxury && Resource.Happiness != 0)
		{
			AddResolvedModifier(
				OutModifiers,
				MakeModifier(SourceId, EConquestModifierTargetScope::Empire, EConquestModifierAttribute::Happiness, EConquestModifierOperation::AddFlat, Resource.Happiness),
				EConquestModifierSourceType::Resource,
				SourceId,
				INDEX_NONE,
				INDEX_NONE,
				&SourceTileCoord
			);
		}
	}

	void GatherActiveModifiers(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		TArray<FConquestResolvedModifier>& OutModifiers
	)
	{
		OutModifiers.Reset();
		if (!GameState || Context.PlayerId == INDEX_NONE)
		{
			return;
		}

		const UConquestContentManager* ContentManager = GameState->ContentManager;
		const FConquestPlayerEmpireState& Player = GameState->GetPlayerEmpire(Context.PlayerId);
		if (Player.PlayerId != Context.PlayerId)
		{
			return;
		}
		const bool bIncludeTargetOwnedContextSources =
			Context.OtherPlayerId == INDEX_NONE ||
			Context.OtherPlayerId == Context.PlayerId;

		if (!Context.bOnlyHappinessPenalty)
		{
			if (const UConquestCivilisationData* Civilisation = GameState->GetCivilisationForPlayer(Context.PlayerId))
			{
				AddRowModifiers(
					OutModifiers,
					Civilisation->Modifiers,
					EConquestModifierSourceType::Civilisation,
					Civilisation->CivilisationId
				);
			}

			if (ContentManager)
			{
				for (const FName TechId : Player.ResearchedTechIds)
				{
					if (const FConquestTechRow* TechRow = ContentManager->FindTech(TechId))
					{
						AddRowModifiers(OutModifiers, TechRow->Modifiers, EConquestModifierSourceType::Tech, TechRow->TechId);
					}
				}

				for (const FName PhilosophyId : Player.AdoptedPhilosophyIds)
				{
					if (const FConquestPhilosophyRow* PhilosophyRow = ContentManager->FindPhilosophy(PhilosophyId))
					{
						AddRowModifiers(
							OutModifiers,
							PhilosophyRow->Modifiers,
							EConquestModifierSourceType::Philosophy,
							PhilosophyRow->PhilosophyId
						);
					}
				}
			}

			if (ContentManager && GameState->CityManager)
			{
				auto AddCityBuildingSources = [&OutModifiers, ContentManager](const FCityState& City)
				{
					for (const FName BuildingId : City.ConstructedBuildingIds)
					{
						if (const FConquestBuildingRow* BuildingRow = ContentManager->FindBuilding(BuildingId))
						{
							AddBuildingModifiers(OutModifiers, *BuildingRow, City.CityId);
						}
					}
				};

				if (
					Context.Scope == EConquestModifierTargetScope::City ||
					Context.Scope == EConquestModifierTargetScope::Production
				)
				{
					if (const FCityState* City = FindContextCity(GameState, Context))
					{
						AddCityBuildingSources(*City);
					}
				}
				else if (
					Context.Scope == EConquestModifierTargetScope::Empire ||
					Context.Scope == EConquestModifierTargetScope::StrategicResource ||
					Context.Scope == EConquestModifierTargetScope::Unit ||
					Context.Scope == EConquestModifierTargetScope::Combat ||
					Context.Scope == EConquestModifierTargetScope::Movement ||
					Context.Scope == EConquestModifierTargetScope::Vision ||
					Context.Scope == EConquestModifierTargetScope::Philosophy
				)
				{
					for (const FCityState& City : GameState->CityManager->Cities)
					{
						if (City.OwnerPlayerId == Context.PlayerId)
						{
							AddCityBuildingSources(City);
						}
					}
				}
				else if (Context.CityId != INDEX_NONE)
				{
					if (const FCityState* City = FindContextCity(GameState, Context))
					{
						AddCityBuildingSources(*City);
					}
				}
			}

			const FConquestUnitState* ContextUnit = FindContextUnit(GameState, Context);
			if (bIncludeTargetOwnedContextSources && ContentManager && ContextUnit)
			{
				if (const FConquestUnitRow* UnitRow = ContentManager->FindUnit(ContextUnit->UnitId))
				{
					AddRowModifiers(
						OutModifiers,
						UnitRow->Modifiers,
						EConquestModifierSourceType::Unit,
						UnitRow->UnitId,
						INDEX_NONE,
						ContextUnit->UnitInstanceId
					);
				}

				for (const FConquestUnitAugmentState& Augment : ContextUnit->Augments)
				{
					if (const FConquestUnitAugmentRow* AugmentRow = ContentManager->FindUnitAugment(Augment.AugmentId))
					{
						AddUnitAugmentModifiers(OutModifiers, *AugmentRow, Augment, ContextUnit->UnitInstanceId);
					}
				}
			}

			const FHexGridModel* GridModel = GameState->GetHexGridModel();
			const FHexTileData* ContextTile = FindContextTile(GameState, Context);
			const FIntPoint RelevantTileCoord = Context.bHasTargetTileCoord
				? Context.TargetTileCoord
				: Context.TileCoord;
			if (bIncludeTargetOwnedContextSources && ContentManager && GridModel && ContextTile)
			{
				if (const FHexImprovementDefinition* Improvement = GridModel->FindImprovementDefinition(ContextTile->ImprovementId))
				{
					AddImprovementModifiers(OutModifiers, *Improvement, RelevantTileCoord);
				}

				if (const FHexResourceDefinition* Resource = GridModel->FindResourceDefinition(ContextTile->Resource.ResourceId))
				{
					AddResourceModifiers(OutModifiers, *Resource, RelevantTileCoord);
				}
			}

			if (
				GridModel &&
				GameState->CityManager &&
				Context.Scope == EConquestModifierTargetScope::Empire
			)
			{
				TSet<FName> AddedResourceIds;
				for (const FCityState& City : GameState->CityManager->Cities)
				{
					if (City.OwnerPlayerId != Context.PlayerId)
					{
						continue;
					}

					for (const FIntPoint& Coord : City.WorkedTiles)
					{
						const FHexTileData* Tile = GridModel->GetTile(Coord);
						if (!Tile || Tile->Resource.ResourceId.IsNone() || AddedResourceIds.Contains(Tile->Resource.ResourceId))
						{
							continue;
						}

						if (const FHexResourceDefinition* Resource = GridModel->FindResourceDefinition(Tile->Resource.ResourceId))
						{
							AddResourceModifiers(OutModifiers, *Resource, Coord);
							AddedResourceIds.Add(Tile->Resource.ResourceId);
						}
					}
				}
			}
		}

		if (!Context.bIgnoreHappinessPenalty)
		{
			const float HappinessMultiplier = ConquestHappiness::GetPenaltyMultiplier(Player.CachedHappiness);
			if (HappinessMultiplier < 1.0f)
			{
				const float PenaltyPercent = HappinessMultiplier - 1.0f;
				if (Context.Scope == EConquestModifierTargetScope::City)
				{
					for (const EConquestModifierAttribute Attribute :
						{
							EConquestModifierAttribute::Food,
							EConquestModifierAttribute::Production,
							EConquestModifierAttribute::Gold,
							EConquestModifierAttribute::Science,
							EConquestModifierAttribute::Culture
						})
					{
						AddResolvedModifier(
							OutModifiers,
							MakeModifier(
								FName(TEXT("Happiness_Unhappy_Yield")),
								EConquestModifierTargetScope::City,
								Attribute,
								EConquestModifierOperation::AddPercent,
								PenaltyPercent
							),
							EConquestModifierSourceType::Happiness,
							FName(TEXT("Happiness"))
						);
					}
				}

				AddResolvedModifier(
					OutModifiers,
					MakeModifier(
						ConquestUnhappyAttackModifierId,
						EConquestModifierTargetScope::Combat,
						EConquestModifierAttribute::CombatAttack,
						EConquestModifierOperation::AddPercent,
						PenaltyPercent
					),
					EConquestModifierSourceType::Happiness,
					FName(TEXT("Happiness"))
				);
				AddResolvedModifier(
					OutModifiers,
					MakeModifier(
						ConquestUnhappyDefenseModifierId,
						EConquestModifierTargetScope::Combat,
						EConquestModifierAttribute::CombatDefense,
						EConquestModifierOperation::AddPercent,
						PenaltyPercent
					),
					EConquestModifierSourceType::Happiness,
					FName(TEXT("Happiness"))
				);
			}
		}
	}

	bool ModifierConditionMatches(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		const FConquestResolvedModifier& Modifier,
		const FConquestModifierCondition& Condition
	)
	{
		if (!GameState)
		{
			return false;
		}

		const FConquestPlayerEmpireState& Player = GameState->GetPlayerEmpire(Context.PlayerId);
		const FCityState* City = FindContextCity(GameState, Context);
		const FConquestUnitState* Unit = FindContextUnit(GameState, Context);
		const FHexTileData* Tile = FindContextTile(GameState, Context);

		bool bMatches = true;
		switch (Condition.Type)
		{
		case EConquestModifierConditionType::Always:
			bMatches = true;
			break;
		case EConquestModifierConditionType::PlayerHasTech:
			bMatches = Player.HasResearched(Condition.Id);
			break;
		case EConquestModifierConditionType::PlayerHasPhilosophy:
			bMatches = !Condition.Id.IsNone() && Player.AdoptedPhilosophyIds.Contains(Condition.Id);
			break;
		case EConquestModifierConditionType::PlayerHasPhilosophyInBranch:
			bMatches = false;
			if (GameState->ContentManager)
			{
				for (const FName PhilosophyId : Player.AdoptedPhilosophyIds)
				{
					const FConquestPhilosophyRow* PhilosophyRow = GameState->ContentManager->FindPhilosophy(PhilosophyId);
					if (PhilosophyRow && MatchesNameList(PhilosophyRow->BranchId, Condition.Id, Condition.Ids))
					{
						bMatches = true;
						break;
					}
				}
			}
			break;
		case EConquestModifierConditionType::CityHasBuilding:
			bMatches = false;
			if (City)
			{
				if (Condition.Ids.Num() > 0)
				{
					for (const FName BuildingId : Condition.Ids)
					{
						if (City->ConstructedBuildingIds.Contains(BuildingId))
						{
							bMatches = true;
							break;
						}
					}
				}
				else if (!Condition.Id.IsNone())
				{
					bMatches = City->ConstructedBuildingIds.Contains(Condition.Id);
				}
			}
			break;
		case EConquestModifierConditionType::CityPopulationAtLeast:
			bMatches = City && City->Population >= Condition.MinValue;
			break;
		case EConquestModifierConditionType::CityIsCapital:
			bMatches =
				City &&
				Player.CityIds.Num() > 0 &&
				Player.CityIds[0] == City->CityId;
			break;
		case EConquestModifierConditionType::UnitIs:
			bMatches = Unit && MatchesNameList(Unit->UnitId, Condition.Id, Condition.Ids);
			break;
		case EConquestModifierConditionType::ProductionIs:
			bMatches = MatchesNameList(Context.ProductionId, Condition.Id, Condition.Ids);
			break;
		case EConquestModifierConditionType::ProductionTypeIs:
			bMatches = Context.ProductionType == Condition.ProductionType;
			break;
		case EConquestModifierConditionType::TileIsOwnedByPlayer:
			bMatches = Tile && Tile->Gameplay.OwnerPlayerId == Context.PlayerId;
			break;
		case EConquestModifierConditionType::TileIsWorked:
			bMatches = Tile && Tile->Gameplay.bIsWorked;
			break;
		case EConquestModifierConditionType::TileTypeIs:
			bMatches = Tile && Tile->TileType == Condition.TileType;
			break;
		case EConquestModifierConditionType::TileHasFeature:
			bMatches = Tile && Tile->Features.Contains(Condition.FeatureType);
			break;
		case EConquestModifierConditionType::TileHasResource:
			bMatches = Tile && MatchesNameList(Tile->Resource.ResourceId, Condition.Id, Condition.Ids);
			break;
		case EConquestModifierConditionType::TileHasImprovement:
			bMatches = Tile && MatchesNameList(Tile->ImprovementId, Condition.Id, Condition.Ids);
			break;
		case EConquestModifierConditionType::ResourceIs:
			bMatches = MatchesNameList(GetContextTileResourceId(GameState, Context), Condition.Id, Condition.Ids);
			break;
		case EConquestModifierConditionType::SourceIs:
			bMatches = MatchesNameList(Modifier.SourceId, Condition.Id, Condition.Ids);
			break;
		case EConquestModifierConditionType::SourceTypeIs:
			bMatches = Modifier.SourceType == Condition.SourceType;
			break;
		case EConquestModifierConditionType::TargetPlayerIsSelf:
			bMatches =
				Context.OtherPlayerId == INDEX_NONE ||
				Context.OtherPlayerId == Context.PlayerId;
			break;
		case EConquestModifierConditionType::TargetPlayerIsOther:
			bMatches =
				Context.OtherPlayerId != INDEX_NONE &&
				Context.OtherPlayerId != Context.PlayerId;
			break;
		default:
			bMatches = false;
			break;
		}

		return Condition.bInvert ? !bMatches : bMatches;
	}

	bool ModifierMatches(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		EConquestModifierAttribute Attribute,
		const FConquestResolvedModifier& Modifier
	)
	{
		if (
			Modifier.Definition.Attribute != Attribute ||
			!TargetScopeMatches(Modifier.Definition.TargetScope, Context.Scope)
		)
		{
			return false;
		}

		for (const FConquestModifierCondition& Condition : Modifier.Definition.Conditions)
		{
			if (!ModifierConditionMatches(GameState, Context, Modifier, Condition))
			{
				return false;
			}
		}

		return true;
	}

	int32 CountBuildings(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		const FConquestModifierDefinition& Definition
	)
	{
		if (!GameState || !GameState->CityManager)
		{
			return 0;
		}

		int32 Count = 0;
		auto CountCityBuildings = [&Definition, &Count](const FCityState& City)
		{
			for (const FName BuildingId : City.ConstructedBuildingIds)
			{
				if (CountIdMatches(BuildingId, Definition))
				{
					++Count;
				}
			}
		};

		if (
			Definition.CountScope == EConquestModifierCountScope::ContextCity ||
			Definition.CountScope == EConquestModifierCountScope::ContextCityWorkedTiles ||
			Definition.CountScope == EConquestModifierCountScope::ContextCityOwnedTiles
		)
		{
			if (const FCityState* City = FindContextCity(GameState, Context))
			{
				CountCityBuildings(*City);
			}
			return Count;
		}

		for (const FCityState& City : GameState->CityManager->Cities)
		{
			if (City.OwnerPlayerId == Context.PlayerId)
			{
				CountCityBuildings(City);
			}
		}

		return Count;
	}

	int32 CountPopulation(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		EConquestModifierCountScope CountScope
	)
	{
		if (!GameState || !GameState->CityManager)
		{
			return 0;
		}

		if (
			CountScope == EConquestModifierCountScope::ContextCity ||
			CountScope == EConquestModifierCountScope::ContextCityWorkedTiles ||
			CountScope == EConquestModifierCountScope::ContextCityOwnedTiles
		)
		{
			const FCityState* City = FindContextCity(GameState, Context);
			return City ? FMath::Max(0, City->Population) : 0;
		}

		int32 Population = 0;
		for (const FCityState& City : GameState->CityManager->Cities)
		{
			if (City.OwnerPlayerId == Context.PlayerId)
			{
				Population += FMath::Max(0, City.Population);
			}
		}
		return Population;
	}

	int32 CountCities(const AConquestGameState* GameState, const FConquestModifierContext& Context)
	{
		if (!GameState || !GameState->CityManager)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FCityState& City : GameState->CityManager->Cities)
		{
			if (City.OwnerPlayerId == Context.PlayerId)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountUnits(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		const FConquestModifierDefinition& Definition
	)
	{
		if (!GameState)
		{
			return 0;
		}

		const FConquestPlayerEmpireState& Player = GameState->GetPlayerEmpire(Context.PlayerId);
		if (Player.PlayerId != Context.PlayerId)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FConquestUnitState& Unit : Player.Units)
		{
			if (CountIdMatches(Unit.UnitId, Definition))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountTileThings(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		const FConquestModifierDefinition& Definition
	)
	{
		const FHexGridModel* GridModel = GameState ? GameState->GetHexGridModel() : nullptr;
		if (!GridModel)
		{
			return 0;
		}

		TArray<FIntPoint> Coords;
		GetTileCoordsForCountScope(GameState, Context, Definition.CountScope, Coords);

		int32 Count = 0;
		for (const FIntPoint& Coord : Coords)
		{
			const FHexTileData* Tile = GridModel->GetTile(Coord);
			if (!Tile)
			{
				continue;
			}

			switch (Definition.CountSource)
			{
			case EConquestModifierCountSource::WorkedTiles:
			case EConquestModifierCountSource::OwnedTiles:
				++Count;
				break;
			case EConquestModifierCountSource::Improvements:
				if (CountIdMatches(Tile->ImprovementId, Definition))
				{
					++Count;
				}
				break;
			case EConquestModifierCountSource::Resources:
				if (CountIdMatches(Tile->Resource.ResourceId, Definition))
				{
					++Count;
				}
				break;
			case EConquestModifierCountSource::LuxuryResources:
			case EConquestModifierCountSource::StrategicResources:
				if (Tile->Resource.ResourceId.IsNone())
				{
					break;
				}
				if (const FHexResourceDefinition* Resource = GridModel->FindResourceDefinition(Tile->Resource.ResourceId))
				{
					const bool bCategoryMatches =
						Definition.CountSource == EConquestModifierCountSource::LuxuryResources
							? Resource->Category == EHexResourceCategory::Luxury
							: Resource->Category == EHexResourceCategory::Strategic;
					if (bCategoryMatches && CountIdMatches(Tile->Resource.ResourceId, Definition))
					{
						++Count;
					}
				}
				break;
			default:
				break;
			}
		}

		return Count;
	}

	int32 CountPhilosophiesInBranch(
		const AConquestGameState* GameState,
		const FConquestPlayerEmpireState& Player,
		const FConquestModifierDefinition& Definition
	)
	{
		if (!GameState || !GameState->ContentManager)
		{
			return 0;
		}

		int32 Count = 0;
		for (const FName PhilosophyId : Player.AdoptedPhilosophyIds)
		{
			const FConquestPhilosophyRow* PhilosophyRow = GameState->ContentManager->FindPhilosophy(PhilosophyId);
			if (PhilosophyRow && CountIdMatches(PhilosophyRow->BranchId, Definition))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 GetRawCountForModifier(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		const FConquestModifierDefinition& Definition
	)
	{
		const FConquestPlayerEmpireState& Player = GameState
			? GameState->GetPlayerEmpire(Context.PlayerId)
			: FConquestPlayerEmpireState();

		switch (Definition.CountSource)
		{
		case EConquestModifierCountSource::None:
			return 1;
		case EConquestModifierCountSource::Cities:
			return CountCities(GameState, Context);
		case EConquestModifierCountSource::Population:
			return CountPopulation(GameState, Context, Definition.CountScope);
		case EConquestModifierCountSource::Buildings:
			return CountBuildings(GameState, Context, Definition);
		case EConquestModifierCountSource::Units:
			return CountUnits(GameState, Context, Definition);
		case EConquestModifierCountSource::WorkedTiles:
		case EConquestModifierCountSource::OwnedTiles:
		case EConquestModifierCountSource::Improvements:
		case EConquestModifierCountSource::Resources:
		case EConquestModifierCountSource::LuxuryResources:
		case EConquestModifierCountSource::StrategicResources:
			return CountTileThings(GameState, Context, Definition);
		case EConquestModifierCountSource::ResearchedTechs:
			return Player.ResearchedTechIds.Num();
		case EConquestModifierCountSource::AdoptedPhilosophies:
			return Player.AdoptedPhilosophyIds.Num();
		case EConquestModifierCountSource::AdoptedPhilosophiesInBranch:
			return CountPhilosophiesInBranch(GameState, Player, Definition);
		default:
			return 0;
		}
	}

	int32 GetModifierApplicationCount(
		const AConquestGameState* GameState,
		const FConquestModifierContext& Context,
		const FConquestModifierDefinition& Definition
	)
	{
		if (Definition.CountSource == EConquestModifierCountSource::None)
		{
			return 1;
		}

		const int32 RawCount = GetRawCountForModifier(GameState, Context, Definition);
		const int32 Divisor = FMath::Max(1, Definition.CountDivisor);
		int32 Applications = Definition.bRoundCountUp
			? FMath::CeilToInt(static_cast<float>(RawCount) / static_cast<float>(Divisor))
			: RawCount / Divisor;

		if (Definition.MaxApplications > 0)
		{
			Applications = FMath::Min(Applications, Definition.MaxApplications);
		}

		return FMath::Max(0, Applications);
	}
}

float UConquestModifierManager::CalculateModifiedValue(
	const FConquestModifierContext& Context,
	EConquestModifierAttribute Attribute,
	float BaseValue
) const
{
	if (!GameStateRef || Attribute == EConquestModifierAttribute::None)
	{
		return BaseValue;
	}

	TArray<FConquestResolvedModifier> Modifiers;
	GatherActiveModifiers(GameStateRef, Context, Modifiers);
	Modifiers.Sort([](const FConquestResolvedModifier& A, const FConquestResolvedModifier& B)
	{
		return A.Definition.ApplicationOrder < B.Definition.ApplicationOrder;
	});

	float FlatBonus = 0.0f;
	float AdditivePercent = 0.0f;
	float Multiplier = 1.0f;
	float OverrideValue = 0.0f;
	bool bHasOverride = false;
	float MinValue = -TNumericLimits<float>::Max();
	float MaxValue = TNumericLimits<float>::Max();

	for (const FConquestResolvedModifier& Modifier : Modifiers)
	{
		if (!ModifierMatches(GameStateRef, Context, Attribute, Modifier))
		{
			continue;
		}

		const int32 Applications = GetModifierApplicationCount(GameStateRef, Context, Modifier.Definition);
		if (Applications <= 0)
		{
			continue;
		}

		const float ScaledValue = Modifier.Definition.Value * static_cast<float>(Applications);
		switch (Modifier.Definition.Operation)
		{
		case EConquestModifierOperation::AddFlat:
			FlatBonus += ScaledValue;
			break;
		case EConquestModifierOperation::AddPercent:
			AdditivePercent += ScaledValue;
			break;
		case EConquestModifierOperation::Multiply:
			Multiplier *= FMath::Pow(FMath::Max(0.0f, Modifier.Definition.Value), Applications);
			break;
		case EConquestModifierOperation::Override:
			OverrideValue = Modifier.Definition.Value;
			bHasOverride = true;
			break;
		case EConquestModifierOperation::Min:
			MinValue = FMath::Max(MinValue, ScaledValue);
			break;
		case EConquestModifierOperation::Max:
			MaxValue = FMath::Min(MaxValue, ScaledValue);
			break;
		default:
			break;
		}
	}

	float Result = bHasOverride
		? OverrideValue
		: (BaseValue + FlatBonus) * FMath::Max(0.0f, 1.0f + AdditivePercent) * Multiplier;

	Result = FMath::Max(Result, MinValue);
	Result = FMath::Min(Result, MaxValue);
	return Result;
}

float UConquestModifierManager::ApplyFinalRounding(EConquestModifierAttribute Attribute, float Value) const
{
	EConquestModifierRoundingMode RoundingMode = EConquestModifierRoundingMode::Nearest;
	float Increment = 1.0f;

	if (GameStateRef)
	{
		RoundingMode = GameStateRef->DefaultModifierRoundingMode;
		Increment = GameStateRef->DefaultModifierRoundingIncrement;

		if (const FConquestModifierRoundingRule* Rule = GameStateRef->ModifierRoundingRules.FindByPredicate(
			[Attribute](const FConquestModifierRoundingRule& Candidate)
			{
				return Candidate.Attribute == Attribute;
			}))
		{
			RoundingMode = Rule->Mode;
			Increment = Rule->Increment;
		}
	}

	if (RoundingMode == EConquestModifierRoundingMode::None)
	{
		return Value;
	}

	const float SafeIncrement = FMath::Max(0.0001f, Increment);
	const float ScaledValue = Value / SafeIncrement;
	switch (RoundingMode)
	{
	case EConquestModifierRoundingMode::Floor:
		return FMath::FloorToFloat(ScaledValue) * SafeIncrement;
	case EConquestModifierRoundingMode::Ceil:
		return FMath::CeilToFloat(ScaledValue) * SafeIncrement;
	case EConquestModifierRoundingMode::Nearest:
	default:
		return FMath::RoundToFloat(ScaledValue) * SafeIncrement;
	}
}

int32 UConquestModifierManager::CalculateModifiedIntValue(
	const FConquestModifierContext& Context,
	EConquestModifierAttribute Attribute,
	float BaseValue
) const
{
	return FMath::RoundToInt(ApplyFinalRounding(
		Attribute,
		CalculateModifiedValue(Context, Attribute, BaseValue)
	));
}

FHexYield UConquestModifierManager::ApplyYieldModifiers(
	const FConquestModifierContext& Context,
	const FHexYield& BaseYield
) const
{
	FHexYield Result = BaseYield;
	for (int32 YieldIndex = 0; YieldIndex < 5; ++YieldIndex)
	{
		const EConquestModifierAttribute Attribute = YieldAttributeFromIndex(YieldIndex);
		const float BaseValue = static_cast<float>(GetYieldByIndex(BaseYield, YieldIndex));
		SetYieldByIndex(
			Result,
			YieldIndex,
			CalculateModifiedIntValue(Context, Attribute, BaseValue)
		);
	}
	return Result;
}

int32 UConquestModifierManager::CalculateProductionCost(
	const FCityState& City,
	ECityProductionType ProductionType,
	FName ProductionId,
	int32 BaseCost
) const
{
	if (!GameStateRef)
	{
		return FMath::Max(1, BaseCost);
	}

	FConquestModifierContext Context;
	Context.Scope = EConquestModifierTargetScope::Production;
	Context.PlayerId = City.OwnerPlayerId;
	Context.CityId = City.CityId;
	Context.ProductionType = ProductionType;
	Context.ProductionId = ProductionId;
	Context.BuildingId = ProductionType == ECityProductionType::Building ? ProductionId : NAME_None;
	Context.UnitId = ProductionType == ECityProductionType::Unit ? ProductionId : NAME_None;

	return FMath::Max(
		1,
		CalculateModifiedIntValue(
			Context,
			EConquestModifierAttribute::ProductionCost,
			static_cast<float>(BaseCost)
		)
	);
}

int32 UConquestModifierManager::CalculateMovementCost(
	const FConquestUnitState& Unit,
	const FIntPoint& FromCoord,
	const FIntPoint& ToCoord,
	int32 BaseCost
) const
{
	if (!GameStateRef || BaseCost == TNumericLimits<int32>::Max())
	{
		return BaseCost;
	}

	FConquestModifierContext Context;
	Context.Scope = EConquestModifierTargetScope::Movement;
	Context.PlayerId = Unit.OwnerPlayerId;
	Context.UnitInstanceId = Unit.UnitInstanceId;
	Context.UnitId = Unit.UnitId;
	Context.TileCoord = FromCoord;
	Context.bHasTileCoord = true;
	Context.TargetTileCoord = ToCoord;
	Context.bHasTargetTileCoord = true;

	return FMath::Max(
		1,
		CalculateModifiedIntValue(
			Context,
			EConquestModifierAttribute::MovementCost,
			static_cast<float>(BaseCost)
		)
	);
}

int32 UConquestModifierManager::CalculatePhilosophyCost(int32 PlayerId, float BaseCost) const
{
	if (!GameStateRef)
	{
		return FMath::Max(0, FMath::RoundToInt(BaseCost));
	}

	FConquestModifierContext Context;
	Context.Scope = EConquestModifierTargetScope::Philosophy;
	Context.PlayerId = PlayerId;

	return FMath::Max(
		0,
		CalculateModifiedIntValue(
			Context,
			EConquestModifierAttribute::PhilosophyCost,
			BaseCost
		)
	);
}

void UConquestModifierManager::BuildUnitCombatModifiers(
	const FConquestUnitState& Unit,
	TArray<FConquestUnitCombatModifier>& OutCombatModifiers
) const
{
	OutCombatModifiers.Reset();
	if (!GameStateRef)
	{
		return;
	}

	FConquestModifierContext Context;
	Context.Scope = EConquestModifierTargetScope::Combat;
	Context.PlayerId = Unit.OwnerPlayerId;
	Context.UnitInstanceId = Unit.UnitInstanceId;
	Context.UnitId = Unit.UnitId;
	Context.TileCoord = Unit.TileCoord;
	Context.bHasTileCoord = true;

	TArray<FConquestResolvedModifier> Modifiers;
	GatherActiveModifiers(GameStateRef, Context, Modifiers);
	Modifiers.Sort([](const FConquestResolvedModifier& A, const FConquestResolvedModifier& B)
	{
		return A.Definition.ApplicationOrder < B.Definition.ApplicationOrder;
	});

	auto AddCombatModifier = [&OutCombatModifiers, this, &Context](const FConquestResolvedModifier& Modifier)
	{
		const bool bAttackModifier = Modifier.Definition.Attribute == EConquestModifierAttribute::CombatAttack;
		const bool bDefenseModifier = Modifier.Definition.Attribute == EConquestModifierAttribute::CombatDefense;
		if (!bAttackModifier && !bDefenseModifier)
		{
			return;
		}

		if (!ModifierMatches(GameStateRef, Context, Modifier.Definition.Attribute, Modifier))
		{
			return;
		}

		const int32 Applications = GetModifierApplicationCount(GameStateRef, Context, Modifier.Definition);
		if (Applications <= 0)
		{
			return;
		}

		FConquestUnitCombatModifier CombatModifier;
		CombatModifier.ModifierId = !Modifier.Definition.ModifierId.IsNone()
			? Modifier.Definition.ModifierId
			: Modifier.SourceId;
		CombatModifier.ModifierType = bAttackModifier
			? EConquestUnitCombatModifierType::Attack
			: EConquestUnitCombatModifierType::Defense;

		switch (Modifier.Definition.Operation)
		{
		case EConquestModifierOperation::AddFlat:
			CombatModifier.FlatBonus = FMath::RoundToInt(ApplyFinalRounding(
				Modifier.Definition.Attribute,
				Modifier.Definition.Value * static_cast<float>(Applications)
			));
			break;
		case EConquestModifierOperation::AddPercent:
			CombatModifier.Multiplier = FMath::Max(
				0.0f,
				1.0f + Modifier.Definition.Value * static_cast<float>(Applications)
			);
			break;
		case EConquestModifierOperation::Multiply:
			CombatModifier.Multiplier = FMath::Pow(FMath::Max(0.0f, Modifier.Definition.Value), Applications);
			break;
		default:
			return;
		}

		if (CombatModifier.FlatBonus != 0 || !FMath::IsNearlyEqual(CombatModifier.Multiplier, 1.0f))
		{
			OutCombatModifiers.Add(CombatModifier);
		}
	};

	for (const FConquestResolvedModifier& Modifier : Modifiers)
	{
		AddCombatModifier(Modifier);
	}

	const FHexGridModel* GridModel = GameStateRef->GetHexGridModel();
	const FHexTileData* Tile = GridModel ? GridModel->GetTile(Unit.TileCoord) : nullptr;
	if (
		Tile &&
		Tile->Gameplay.OwnerPlayerId == Unit.OwnerPlayerId &&
		Tile->Gameplay.DefenderModifier > 1.0f
	)
	{
		FConquestUnitCombatModifier TileDefenseModifier;
		TileDefenseModifier.ModifierId = ConquestTileDefenseModifierId;
		TileDefenseModifier.ModifierType = EConquestUnitCombatModifierType::Defense;
		TileDefenseModifier.Multiplier = Tile->Gameplay.DefenderModifier;
		OutCombatModifiers.Add(TileDefenseModifier);
	}
}

void UConquestModifierManager::BuildExternalUnitCombatModifiers(
	const FConquestUnitState& TargetUnit,
	int32 SourcePlayerId,
	TArray<FConquestUnitCombatModifier>& OutCombatModifiers
) const
{
	OutCombatModifiers.Reset();
	if (!GameStateRef || SourcePlayerId == INDEX_NONE || SourcePlayerId == TargetUnit.OwnerPlayerId)
	{
		return;
	}

	FConquestModifierContext Context;
	Context.Scope = EConquestModifierTargetScope::Combat;
	Context.PlayerId = SourcePlayerId;
	Context.OtherPlayerId = TargetUnit.OwnerPlayerId;
	Context.UnitInstanceId = TargetUnit.UnitInstanceId;
	Context.UnitId = TargetUnit.UnitId;
	Context.TileCoord = TargetUnit.TileCoord;
	Context.bHasTileCoord = true;

	TArray<FConquestResolvedModifier> Modifiers;
	GatherActiveModifiers(GameStateRef, Context, Modifiers);
	Modifiers.Sort([](const FConquestResolvedModifier& A, const FConquestResolvedModifier& B)
	{
		return A.Definition.ApplicationOrder < B.Definition.ApplicationOrder;
	});

	for (const FConquestResolvedModifier& Modifier : Modifiers)
	{
		const bool bExplicitlyTargetsOther = Modifier.Definition.Conditions.ContainsByPredicate(
			[](const FConquestModifierCondition& Condition)
			{
				return
					Condition.Type == EConquestModifierConditionType::TargetPlayerIsOther &&
					!Condition.bInvert;
			}
		);
		if (!bExplicitlyTargetsOther)
		{
			continue;
		}

		const bool bAttackModifier = Modifier.Definition.Attribute == EConquestModifierAttribute::CombatAttack;
		const bool bDefenseModifier = Modifier.Definition.Attribute == EConquestModifierAttribute::CombatDefense;
		if (!bAttackModifier && !bDefenseModifier)
		{
			continue;
		}

		if (!ModifierMatches(GameStateRef, Context, Modifier.Definition.Attribute, Modifier))
		{
			continue;
		}

		const int32 Applications = GetModifierApplicationCount(GameStateRef, Context, Modifier.Definition);
		if (Applications <= 0)
		{
			continue;
		}

		FConquestUnitCombatModifier CombatModifier;
		CombatModifier.ModifierId = !Modifier.Definition.ModifierId.IsNone()
			? Modifier.Definition.ModifierId
			: Modifier.SourceId;
		CombatModifier.ModifierType = bAttackModifier
			? EConquestUnitCombatModifierType::Attack
			: EConquestUnitCombatModifierType::Defense;

		switch (Modifier.Definition.Operation)
		{
		case EConquestModifierOperation::AddFlat:
			CombatModifier.FlatBonus = FMath::RoundToInt(ApplyFinalRounding(
				Modifier.Definition.Attribute,
				Modifier.Definition.Value * static_cast<float>(Applications)
			));
			break;
		case EConquestModifierOperation::AddPercent:
			CombatModifier.Multiplier = FMath::Max(
				0.0f,
				1.0f + Modifier.Definition.Value * static_cast<float>(Applications)
			);
			break;
		case EConquestModifierOperation::Multiply:
			CombatModifier.Multiplier = FMath::Pow(FMath::Max(0.0f, Modifier.Definition.Value), Applications);
			break;
		default:
			continue;
		}

		if (CombatModifier.FlatBonus != 0 || !FMath::IsNearlyEqual(CombatModifier.Multiplier, 1.0f))
		{
			OutCombatModifiers.Add(CombatModifier);
		}
	}
}

float UConquestModifierManager::CalculateUnitCombatValue(
	const FConquestUnitState& Unit,
	EConquestUnitCombatModifierType ModifierType,
	int32 ExternalSourcePlayerId
) const
{
	if (!GameStateRef || ExternalSourcePlayerId == INDEX_NONE || ExternalSourcePlayerId == Unit.OwnerPlayerId)
	{
		return ConquestUnitCombat::GetCombatValue(Unit, ModifierType);
	}

	FConquestUnitState UnitWithExternalModifiers = Unit;
	TArray<FConquestUnitCombatModifier> ExternalModifiers;
	BuildExternalUnitCombatModifiers(Unit, ExternalSourcePlayerId, ExternalModifiers);
	UnitWithExternalModifiers.CombatModifiers.Append(ExternalModifiers);
	return ConquestUnitCombat::GetCombatValue(UnitWithExternalModifiers, ModifierType);
}
