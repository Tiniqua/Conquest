#include "WorldGenRuleTypes.h"

TArray<EWorldGenTileType> FWorldGenRuleTypes::GetAllTileTypes()
{
	return {
		EWorldGenTileType::Grassland,
		EWorldGenTileType::GrasslandHill,
		EWorldGenTileType::Plains,
		EWorldGenTileType::PlainsHill,
		EWorldGenTileType::Desert,
		EWorldGenTileType::DesertHill,
		EWorldGenTileType::Tundra,
		EWorldGenTileType::TundraHill,
		EWorldGenTileType::Snow,
		EWorldGenTileType::SnowHill,
		EWorldGenTileType::Coast,
		EWorldGenTileType::Ocean,
		EWorldGenTileType::Mountain
	};
}

FWorldGenTileRule FWorldGenRuleTypes::GetTileRule(
	EWorldGenTileType TileType,
	float FlatHeight,
	float HillHeight,
	float MountainHeight,
	float CoastHeight,
	float OceanHeight
)
{
	FWorldGenTileRule Rule;
	Rule.TileType = TileType;
	Rule.DebugName = StaticEnum<EWorldGenTileType>()->GetNameByValue(static_cast<int64>(TileType));

	switch (TileType)
	{
	case EWorldGenTileType::Grassland:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Grassland;
		Rule.ElevationType = EWorldGenElevationType::Flat;
		Rule.Height = FlatHeight;
		Rule.TransitionPriority = 20;
		Rule.MaterialSlot = 0;
		break;

	case EWorldGenTileType::GrasslandHill:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Grassland;
		Rule.ElevationType = EWorldGenElevationType::Hill;
		Rule.Height = HillHeight;
		Rule.TransitionPriority = 80;
		Rule.MaterialSlot = 1;
		break;

	case EWorldGenTileType::Plains:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Plains;
		Rule.ElevationType = EWorldGenElevationType::Flat;
		Rule.Height = FlatHeight;
		Rule.TransitionPriority = 20;
		Rule.MaterialSlot = 2;
		break;

	case EWorldGenTileType::PlainsHill:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Plains;
		Rule.ElevationType = EWorldGenElevationType::Hill;
		Rule.Height = HillHeight;
		Rule.TransitionPriority = 80;
		Rule.MaterialSlot = 3;
		break;

	case EWorldGenTileType::Desert:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Desert;
		Rule.ElevationType = EWorldGenElevationType::Flat;
		Rule.Height = FlatHeight;
		Rule.TransitionPriority = 20;
		Rule.MaterialSlot = 4;
		break;

	case EWorldGenTileType::DesertHill:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Desert;
		Rule.ElevationType = EWorldGenElevationType::Hill;
		Rule.Height = HillHeight;
		Rule.TransitionPriority = 80;
		Rule.MaterialSlot = 5;
		break;

	case EWorldGenTileType::Tundra:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Tundra;
		Rule.ElevationType = EWorldGenElevationType::Flat;
		Rule.Height = FlatHeight;
		Rule.TransitionPriority = 20;
		Rule.MaterialSlot = 6;
		break;

	case EWorldGenTileType::TundraHill:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Tundra;
		Rule.ElevationType = EWorldGenElevationType::Hill;
		Rule.Height = HillHeight;
		Rule.TransitionPriority = 80;
		Rule.MaterialSlot = 7;
		break;

	case EWorldGenTileType::Snow:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Snow;
		Rule.ElevationType = EWorldGenElevationType::Flat;
		Rule.Height = FlatHeight;
		Rule.TransitionPriority = 20;
		Rule.MaterialSlot = 8;
		break;

	case EWorldGenTileType::SnowHill:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Snow;
		Rule.ElevationType = EWorldGenElevationType::Hill;
		Rule.Height = HillHeight;
		Rule.TransitionPriority = 80;
		Rule.MaterialSlot = 9;
		break;

	case EWorldGenTileType::Coast:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Coast;
		Rule.ElevationType = EWorldGenElevationType::Flat;
		Rule.Height = CoastHeight;
		Rule.bIsWater = true;
		Rule.TransitionPriority = 90;
		Rule.MaterialSlot = 10;
		break;

	case EWorldGenTileType::Ocean:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Ocean;
		Rule.ElevationType = EWorldGenElevationType::Flat;
		Rule.Height = OceanHeight;
		Rule.bIsWater = true;
		Rule.TransitionPriority = 95;
		Rule.MaterialSlot = 11;
		break;

	case EWorldGenTileType::Mountain:
		Rule.BaseTerrain = EWorldGenBaseTerrain::Mountain;
		Rule.ElevationType = EWorldGenElevationType::Mountain;
		Rule.Height = MountainHeight;
		Rule.bBlocksMovement = true;
		Rule.TransitionPriority = 100;
		Rule.MaterialSlot = 12;
		break;

	default:
		break;
	}

	return Rule;
}

FWorldGenTransitionRule FWorldGenRuleTypes::GetTransitionRule(
	EWorldGenTileType ThisTileType,
	EWorldGenTileType OtherTileType,
	float FlatHeight,
	float HillHeight,
	float MountainHeight,
	float CoastHeight,
	float OceanHeight
)
{
	const FWorldGenTileRule ThisRule = GetTileRule(
		ThisTileType,
		FlatHeight,
		HillHeight,
		MountainHeight,
		CoastHeight,
		OceanHeight
	);

	const FWorldGenTileRule OtherRule = GetTileRule(
		OtherTileType,
		FlatHeight,
		HillHeight,
		MountainHeight,
		CoastHeight,
		OceanHeight
	);

	FWorldGenTransitionRule Result;
	Result.ThisHeight = ThisRule.Height;
	Result.OtherHeight = OtherRule.Height;
	Result.TransitionMaterialTileType = ThisTileType;

	const bool bSameHeight = FMath::IsNearlyEqual(
		ThisRule.Height,
		OtherRule.Height,
		0.1f
	);

	if (bSameHeight)
	{
		Result.bHasTransition = false;
		Result.bThisTileOwnsTransition = false;
		Result.TransitionStyle = EWorldGenTransitionStyle::None;
		Result.TransitionMaterialTileType = ThisTileType;
		return Result;
	}

	if (AreTilesSeamless(ThisTileType, OtherTileType))
	{
		Result.bHasTransition = false;
		Result.bThisTileOwnsTransition = false;
		Result.TransitionStyle = EWorldGenTransitionStyle::None;
		return Result;
	}

	const EWorldGenTileType Owner = ChooseTransitionOwner(
		ThisTileType,
		OtherTileType,
		ThisRule,
		OtherRule
	);

	Result.bHasTransition = true;
	Result.bThisTileOwnsTransition = Owner == ThisTileType;
	Result.TransitionMaterialTileType = Owner;

	if (ThisRule.ElevationType == EWorldGenElevationType::Mountain ||
		OtherRule.ElevationType == EWorldGenElevationType::Mountain)
	{
		Result.TransitionStyle = EWorldGenTransitionStyle::Cliff;
	}
	else
	{
		Result.TransitionStyle = EWorldGenTransitionStyle::Slope;
	}

	return Result;
}

bool FWorldGenRuleTypes::AreTilesSeamless(
	EWorldGenTileType A,
	EWorldGenTileType B
)
{
	if (A == B)
	{
		return true;
	}

	const FWorldGenTileRule RuleA = GetTileRule(A, 0.0f, 80.0f, 120.0f, -10.0f, -20.0f);
	const FWorldGenTileRule RuleB = GetTileRule(B, 0.0f, 80.0f, 120.0f, -10.0f, -20.0f);

	// Same elevation layer should be geometrically seamless.
	// Different materials can still meet, but they should not create slope geometry.
	if (RuleA.ElevationType == RuleB.ElevationType &&
		FMath::IsNearlyEqual(RuleA.Height, RuleB.Height, 0.1f))
	{
		return true;
	}

	// Water of the same height/elevation should also be seamless.
	if (RuleA.bIsWater && RuleB.bIsWater &&
		FMath::IsNearlyEqual(RuleA.Height, RuleB.Height, 0.1f))
	{
		return true;
	}

	return false;
}

bool FWorldGenRuleTypes::IsWater(EWorldGenTileType TileType)
{
	return TileType == EWorldGenTileType::Coast ||
		TileType == EWorldGenTileType::Ocean;
}

bool FWorldGenRuleTypes::IsHill(EWorldGenTileType TileType)
{
	return GetElevationType(TileType) == EWorldGenElevationType::Hill;
}

bool FWorldGenRuleTypes::IsMountain(EWorldGenTileType TileType)
{
	return TileType == EWorldGenTileType::Mountain;
}

EWorldGenBaseTerrain FWorldGenRuleTypes::GetBaseTerrain(EWorldGenTileType TileType)
{
	return GetTileRule(TileType, 0.0f, 1.0f, 2.0f, -0.25f, -0.5f).BaseTerrain;
}

EWorldGenElevationType FWorldGenRuleTypes::GetElevationType(EWorldGenTileType TileType)
{
	return GetTileRule(TileType, 0.0f, 1.0f, 2.0f, -0.25f, -0.5f).ElevationType;
}

EWorldGenTileType FWorldGenRuleTypes::GetFlatVariantForBaseTerrain(
	EWorldGenBaseTerrain BaseTerrain
)
{
	switch (BaseTerrain)
	{
	case EWorldGenBaseTerrain::Grassland:
		return EWorldGenTileType::Grassland;

	case EWorldGenBaseTerrain::Plains:
		return EWorldGenTileType::Plains;

	case EWorldGenBaseTerrain::Desert:
		return EWorldGenTileType::Desert;

	case EWorldGenBaseTerrain::Tundra:
		return EWorldGenTileType::Tundra;

	case EWorldGenBaseTerrain::Snow:
		return EWorldGenTileType::Snow;

	case EWorldGenBaseTerrain::Coast:
		return EWorldGenTileType::Coast;

	case EWorldGenBaseTerrain::Ocean:
		return EWorldGenTileType::Ocean;

	case EWorldGenBaseTerrain::Mountain:
		return EWorldGenTileType::Mountain;

	default:
		return EWorldGenTileType::Grassland;
	}
}

EWorldGenTileType FWorldGenRuleTypes::GetHillVariantForBaseTerrain(
	EWorldGenBaseTerrain BaseTerrain
)
{
	switch (BaseTerrain)
	{
	case EWorldGenBaseTerrain::Grassland:
		return EWorldGenTileType::GrasslandHill;

	case EWorldGenBaseTerrain::Plains:
		return EWorldGenTileType::PlainsHill;

	case EWorldGenBaseTerrain::Desert:
		return EWorldGenTileType::DesertHill;

	case EWorldGenBaseTerrain::Tundra:
		return EWorldGenTileType::TundraHill;

	case EWorldGenBaseTerrain::Snow:
		return EWorldGenTileType::SnowHill;

	default:
		return GetFlatVariantForBaseTerrain(BaseTerrain);
	}
}

EWorldGenTileType FWorldGenRuleTypes::ChooseTransitionOwner(
	EWorldGenTileType A,
	EWorldGenTileType B,
	const FWorldGenTileRule& RuleA,
	const FWorldGenTileRule& RuleB
)
{
	const bool bAIsRaised =
		RuleA.ElevationType == EWorldGenElevationType::Hill ||
		RuleA.ElevationType == EWorldGenElevationType::Mountain;

	const bool bBIsRaised =
		RuleB.ElevationType == EWorldGenElevationType::Hill ||
		RuleB.ElevationType == EWorldGenElevationType::Mountain;

	// Raised terrain owns its sides.
	// This prevents water materials from climbing up hills/mountains.
	if (bAIsRaised && !bBIsRaised)
	{
		return A;
	}

	if (!bAIsRaised && bBIsRaised)
	{
		return B;
	}

	// Mountain owns against hill.
	if (RuleA.ElevationType == EWorldGenElevationType::Mountain &&
		RuleB.ElevationType != EWorldGenElevationType::Mountain)
	{
		return A;
	}

	if (RuleB.ElevationType == EWorldGenElevationType::Mountain &&
		RuleA.ElevationType != EWorldGenElevationType::Mountain)
	{
		return B;
	}

	// Water owns gentle shoreline against flat land.
	if (RuleA.bIsWater && !RuleB.bIsWater)
	{
		return A;
	}

	if (!RuleA.bIsWater && RuleB.bIsWater)
	{
		return B;
	}

	// Deep water owns coast/ocean transitions.
	if (A == EWorldGenTileType::Ocean && B == EWorldGenTileType::Coast)
	{
		return A;
	}

	if (B == EWorldGenTileType::Ocean && A == EWorldGenTileType::Coast)
	{
		return B;
	}

	return RuleA.TransitionPriority >= RuleB.TransitionPriority ? A : B;
}