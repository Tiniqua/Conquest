#pragma once

#include "CoreMinimal.h"
#include "ConquestEffectTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ConquestGameplayEffect.generated.h"

class UConquestEffectCondition;

UCLASS(BlueprintType)
class CONQUEST_API UConquestGameplayEffect : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FName EffectId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FGameplayTagContainer EffectTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	EConquestEffectTrigger Trigger = EConquestEffectTrigger::Passive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	EConquestEffectScope Scope = EConquestEffectScope::Self;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	EConquestEffectTargetType TargetType = EConquestEffectTargetType::Player;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions")
	TArray<TObjectPtr<UConquestEffectCondition>> Conditions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifiers")
	TArray<FConquestEffectModifier> Modifiers;
};
