#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ConquestLeaderDefinition.generated.h"

class UTexture2D;
class UConquestEffectSet;

UCLASS(BlueprintType)
class CONQUEST_API UConquestLeaderDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leader")
	FName LeaderId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leader")
	FText LeaderName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leader")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leader")
	TObjectPtr<UTexture2D> Portrait = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leader")
	FGameplayTagContainer LeaderTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leader|Effects")
	TObjectPtr<UConquestEffectSet> LeaderEffectSet = nullptr;
};
