#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConquestPhilosophyManager.generated.h"

class AConquestGameState;
struct FConquestPhilosophyRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhilosophiesChanged);

UCLASS(BlueprintType)
class CONQUEST_API UConquestPhilosophyManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AConquestGameState* InGameState);

	UPROPERTY(BlueprintAssignable)
	FOnPhilosophiesChanged OnPhilosophiesChanged;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Philosophy")
	TArray<FName> GetAvailablePhilosophyIds(int32 PlayerId) const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Philosophy")
	bool CanAdoptPhilosophy(int32 PlayerId, FName PhilosophyId) const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Philosophy")
	int32 GetNextPhilosophyCultureCost(int32 PlayerId) const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Philosophy")
	bool AdoptPhilosophyById(int32 PlayerId, FName PhilosophyId);

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;
};
