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
