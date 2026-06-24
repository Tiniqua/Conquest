#include "ConquestPlayerEmpireState.h"

namespace
{
	constexpr int32 ConquestHappinessCityCost = 4;
	constexpr int32 ConquestHappinessPopulationCost = 1;
	constexpr int32 ConquestSevereUnhappyThreshold = -10;
	constexpr float ConquestUnhappyPenaltyMultiplier = 0.8f;
	constexpr float ConquestSevereUnhappyPenaltyMultiplier = 0.6f;
}

namespace ConquestUnitCombat
{
	float GetHealthCombatMultiplier(const FConquestUnitState& Unit)
	{
		return FMath::Clamp(
			static_cast<float>(FMath::Max(1, Unit.CurrentHealth)) /
			static_cast<float>(FMath::Max(1, Unit.CachedMaxHealth)),
			0.01f,
			1.0f
		);
	}

	float GetCombatValue(const FConquestUnitState& Unit, EConquestUnitCombatModifierType ModifierType)
	{
		float Value = static_cast<float>(FMath::Max(1, Unit.CachedStrength));
		float Multiplier = GetHealthCombatMultiplier(Unit);

		for (const FConquestUnitCombatModifier& Modifier : Unit.CombatModifiers)
		{
			if (Modifier.ModifierType != ModifierType)
			{
				continue;
			}

			Value += static_cast<float>(Modifier.FlatBonus);
			Multiplier *= FMath::Max(0.0f, Modifier.Multiplier);
		}

		return FMath::Max(1.0f, Value * Multiplier);
	}

	int32 CalculateDeterministicDamage(float AttackerValue, float DefenderValue, float EqualStrengthDamage)
	{
		const float Ratio = AttackerValue / FMath::Max(1.0f, DefenderValue);
		return FMath::Clamp(FMath::RoundToInt(EqualStrengthDamage * Ratio), 1, 100);
	}

	FConquestCombatPreviewData CalculatePreview(
		const FConquestUnitState& Attacker,
		const FConquestUnitState& Defender,
		int32 AttackDistance
	)
	{
		FConquestCombatPreviewData Preview;
		Preview.AttackerUnitInstanceId = Attacker.UnitInstanceId;
		Preview.DefenderUnitInstanceId = Defender.UnitInstanceId;
		Preview.AttackerCurrentHealth = Attacker.CurrentHealth;
		Preview.AttackerMaxHealth = Attacker.CachedMaxHealth;
		Preview.DefenderCurrentHealth = Defender.CurrentHealth;
		Preview.DefenderMaxHealth = Defender.CachedMaxHealth;
		Preview.AttackDistance = AttackDistance;

		if (Attacker.UnitInstanceId == INDEX_NONE || Defender.UnitInstanceId == INDEX_NONE)
		{
			Preview.InvalidReason = NSLOCTEXT("Conquest", "CombatPreviewInvalidUnit", "Invalid unit");
			return Preview;
		}

		if (Attacker.OwnerPlayerId == Defender.OwnerPlayerId)
		{
			Preview.InvalidReason = NSLOCTEXT("Conquest", "CombatPreviewFriendlyTarget", "Friendly target");
			return Preview;
		}

		if (Attacker.CurrentMovementPoints <= 0)
		{
			Preview.InvalidReason = NSLOCTEXT("Conquest", "CombatPreviewNoMovement", "No actions remaining");
			return Preview;
		}

		if (AttackDistance <= 0 || AttackDistance > FMath::Max(1, Attacker.CachedAttackRange))
		{
			Preview.InvalidReason = NSLOCTEXT("Conquest", "CombatPreviewOutOfRange", "Out of range");
			return Preview;
		}

		const float AttackerCombatValue = GetCombatValue(Attacker, EConquestUnitCombatModifierType::Attack);
		const float DefenderDefenseValue = GetCombatValue(Defender, EConquestUnitCombatModifierType::Defense);
		Preview.DamageDealt = CalculateDeterministicDamage(AttackerCombatValue, DefenderDefenseValue, 50.0f);
		Preview.DefenderProjectedHealth = FMath::Clamp(
			Defender.CurrentHealth - Preview.DamageDealt,
			0,
			Defender.CachedMaxHealth
		);
		Preview.bDefenderKilled = Preview.DefenderProjectedHealth <= 0;

		Preview.bHasCounterAttack = !Preview.bDefenderKilled && AttackDistance <= 1;
		if (Preview.bHasCounterAttack)
		{
			const float DefenderCounterValue = GetCombatValue(Defender, EConquestUnitCombatModifierType::Attack);
			Preview.DamageTaken = CalculateDeterministicDamage(DefenderCounterValue, AttackerCombatValue, 25.0f);
		}

		Preview.AttackerProjectedHealth = FMath::Clamp(
			Attacker.CurrentHealth - Preview.DamageTaken,
			0,
			Attacker.CachedMaxHealth
		);
		Preview.bAttackerKilled = Preview.AttackerProjectedHealth <= 0;
		Preview.bCanAttack = true;
		Preview.bIsValid = true;

		const float AttackerHealthRatio =
			static_cast<float>(Preview.AttackerProjectedHealth) /
			static_cast<float>(FMath::Max(1, Preview.AttackerMaxHealth));
		if (Preview.bAttackerKilled)
		{
			Preview.Rating = EConquestCombatPreviewRating::Costly;
			Preview.RatingText = NSLOCTEXT("Conquest", "CombatPreviewCostlyAttack", "Costly Attack");
		}
		else if (
			Preview.bDefenderKilled ||
			Preview.DamageTaken <= 0 ||
			(AttackerHealthRatio >= 0.75f && Preview.DamageDealt >= Preview.DamageTaken)
		)
		{
			Preview.Rating = EConquestCombatPreviewRating::Safe;
			Preview.RatingText = NSLOCTEXT("Conquest", "CombatPreviewSafeAttack", "Safe Attack");
		}
		else if (AttackerHealthRatio >= 0.5f && Preview.DamageDealt >= Preview.DamageTaken)
		{
			Preview.Rating = EConquestCombatPreviewRating::Neutral;
			Preview.RatingText = NSLOCTEXT("Conquest", "CombatPreviewNeutralAttack", "Neutral Attack");
		}
		else
		{
			Preview.Rating = EConquestCombatPreviewRating::Costly;
			Preview.RatingText = NSLOCTEXT("Conquest", "CombatPreviewCostlyAttack", "Costly Attack");
		}

		return Preview;
	}
}

namespace ConquestHappiness
{
	int32 GetCityCost()
	{
		return ConquestHappinessCityCost;
	}

	int32 GetPopulationCost()
	{
		return ConquestHappinessPopulationCost;
	}

	int32 GetSevereUnhappyThreshold()
	{
		return ConquestSevereUnhappyThreshold;
	}

	bool IsUnhappy(int32 Happiness)
	{
		return Happiness < 0;
	}

	bool IsSeverelyUnhappy(int32 Happiness)
	{
		return Happiness <= ConquestSevereUnhappyThreshold;
	}

	float GetPenaltyMultiplier(int32 Happiness)
	{
		if (IsSeverelyUnhappy(Happiness))
		{
			return ConquestSevereUnhappyPenaltyMultiplier;
		}

		return IsUnhappy(Happiness)
			? ConquestUnhappyPenaltyMultiplier
			: 1.0f;
	}

	FText GetPenaltyText(int32 Happiness)
	{
		if (IsSeverelyUnhappy(Happiness))
		{
			return NSLOCTEXT("Conquest", "SevereUnhappyPenaltyText", "-40% Unhappy");
		}

		if (IsUnhappy(Happiness))
		{
			return NSLOCTEXT("Conquest", "UnhappyPenaltyText", "-20% Unhappy");
		}

		return FText::GetEmpty();
	}
}
