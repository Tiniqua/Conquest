#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ConquestDataTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestYieldType : uint8
{
	Food        UMETA(DisplayName = "Food"),
	Production  UMETA(DisplayName = "Production"),
	Gold        UMETA(DisplayName = "Gold"),
	Science     UMETA(DisplayName = "Science"),
	Culture     UMETA(DisplayName = "Culture"),
	Faith       UMETA(DisplayName = "Faith"),
	Happiness   UMETA(DisplayName = "Happiness")
};

UENUM(BlueprintType)
enum class EConquestEra : uint8
{
	Ancient      UMETA(DisplayName = "Ancient"),
	Classical    UMETA(DisplayName = "Classical"),
	Medieval     UMETA(DisplayName = "Medieval"),
	Renaissance  UMETA(DisplayName = "Renaissance"),
	Industrial   UMETA(DisplayName = "Industrial"),
	Modern       UMETA(DisplayName = "Modern"),
	Atomic       UMETA(DisplayName = "Atomic"),
	Information  UMETA(DisplayName = "Information")
};

UENUM(BlueprintType)
enum class EConquestDomain : uint8
{
	Land  UMETA(DisplayName = "Land"),
	Sea   UMETA(DisplayName = "Sea"),
	Air   UMETA(DisplayName = "Air")
};

UENUM(BlueprintType)
enum class EConquestUnitRole : uint8
{
	Melee        UMETA(DisplayName = "Melee"),
	Ranged       UMETA(DisplayName = "Ranged"),
	Siege        UMETA(DisplayName = "Siege"),
	Cavalry      UMETA(DisplayName = "Cavalry"),
	Recon        UMETA(DisplayName = "Recon"),
	NavalMelee   UMETA(DisplayName = "Naval Melee"),
	NavalRanged  UMETA(DisplayName = "Naval Ranged"),
	Civilian     UMETA(DisplayName = "Civilian"),
	Settler      UMETA(DisplayName = "Settler"),
	Worker       UMETA(DisplayName = "Worker")
};

UENUM(BlueprintType)
enum class EConquestModifierOperation : uint8
{
	Add              UMETA(DisplayName = "Add"),
	Subtract         UMETA(DisplayName = "Subtract"),
	Multiply         UMETA(DisplayName = "Multiply"),
	Divide           UMETA(DisplayName = "Divide"),
	Override         UMETA(DisplayName = "Override"),
	PercentAdd       UMETA(DisplayName = "Percent Add"),
	PercentMultiply  UMETA(DisplayName = "Percent Multiply")
};

UENUM(BlueprintType)
enum class EConquestStackingRule : uint8
{
	StackNormally  UMETA(DisplayName = "Stack Normally"),
	HighestOnly    UMETA(DisplayName = "Highest Only"),
	LowestOnly     UMETA(DisplayName = "Lowest Only"),
	UniqueBySource UMETA(DisplayName = "Unique By Source"),
	DoesNotStack   UMETA(DisplayName = "Does Not Stack")
};

USTRUCT(BlueprintType)
struct CONQUEST_API FConquestYieldAmount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield")
	EConquestYieldType YieldType = EConquestYieldType::Food;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield")
	int32 Amount = 0;
};

USTRUCT(BlueprintType)
struct CONQUEST_API FConquestYieldModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield")
	EConquestYieldType YieldType = EConquestYieldType::Food;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield")
	EConquestModifierOperation Operation = EConquestModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yield")
	float Value = 0.0f;
};
