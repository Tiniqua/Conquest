#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConquestEffectSet.generated.h"

class UConquestGameplayEffect;

UCLASS(BlueprintType)
class CONQUEST_API UConquestEffectSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect Set")
	FName EffectSetId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect Set")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect Set")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TArray<TObjectPtr<UConquestGameplayEffect>> Effects;
};
