#pragma once

#include "CoreMinimal.h"
#include "HexYieldTypes.generated.h"

USTRUCT(BlueprintType)
struct FHexYield
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Food = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Production = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Gold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Science = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Culture = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Yield")
	int32 Faith = 0;

	void Reset()
	{
		Food = 0;
		Production = 0;
		Gold = 0;
		Science = 0;
		Culture = 0;
		Faith = 0;
	}

	bool IsZero() const
	{
		return Food == 0 &&
			Production == 0 &&
			Gold == 0 &&
			Science == 0 &&
			Culture == 0 &&
			Faith == 0;
	}

	int32 GetTotal() const
	{
		return Food + Production + Gold + Science + Culture + Faith;
	}

	float GetWeightedScore(
		float FoodWeight = 1.0f,
		float ProductionWeight = 1.0f,
		float GoldWeight = 1.0f,
		float ScienceWeight = 1.0f,
		float CultureWeight = 1.0f,
		float FaithWeight = 1.0f
	) const
	{
		return Food * FoodWeight +
			Production * ProductionWeight +
			Gold * GoldWeight +
			Science * ScienceWeight +
			Culture * CultureWeight +
			Faith * FaithWeight;
	}

	FString ToCompactString() const
	{
		return FString::Printf(
			TEXT("Food %d | Prod %d | Gold %d | Sci %d | Cult %d | Faith %d"),
			Food,
			Production,
			Gold,
			Science,
			Culture,
			Faith
		);
	}

	FHexYield& operator+=(const FHexYield& Other)
	{
		Food += Other.Food;
		Production += Other.Production;
		Gold += Other.Gold;
		Science += Other.Science;
		Culture += Other.Culture;
		Faith += Other.Faith;
		return *this;
	}

	friend FHexYield operator+(FHexYield Left, const FHexYield& Right)
	{
		Left += Right;
		return Left;
	}
};
