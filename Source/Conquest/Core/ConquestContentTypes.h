#pragma once

#include "CoreMinimal.h"
#include "ConquestContentTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestProductionCategory : uint8
{
	Building UMETA(DisplayName = "Building"),
	Wonder   UMETA(DisplayName = "Wonder"),
	Project  UMETA(DisplayName = "Project"),
	Unit     UMETA(DisplayName = "Unit")
};

UENUM(BlueprintType)
enum class EConquestChoiceContentType : uint8
{
	None       UMETA(DisplayName = "None"),
	Building   UMETA(DisplayName = "Building"),
	Unit       UMETA(DisplayName = "Unit"),
	Tech       UMETA(DisplayName = "Tech"),
	Philosophy UMETA(DisplayName = "Philosophy")
};

USTRUCT(BlueprintType)
struct FConquestContentId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Id = NAME_None;

	bool IsValid() const
	{
		return !Id.IsNone();
	}

	bool operator==(const FConquestContentId& Other) const
	{
		return Id == Other.Id;
	}
};

FORCEINLINE uint32 GetTypeHash(const FConquestContentId& Value)
{
	return GetTypeHash(Value.Id);
}