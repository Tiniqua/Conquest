#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConquestTechManager.generated.h"

class AConquestGameState;
struct FConquestPlayerEmpireState;
struct FConquestTechRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResearchChanged);

UCLASS(BlueprintType)
class CONQUEST_API UConquestTechManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AConquestGameState* InGameState);

	UPROPERTY(BlueprintAssignable)
	FOnResearchChanged OnResearchChanged;

	UFUNCTION(BlueprintCallable)
	bool SetCurrentResearchById(int32 PlayerId, FName TechId);

	UFUNCTION(BlueprintCallable)
	void ProcessResearchAtStartOfTurn(int32 PlayerId);

	UFUNCTION(BlueprintCallable)
	TArray<FName> GetAvailableResearchIds(int32 PlayerId) const;

	UFUNCTION(BlueprintCallable)
	int32 EstimateTurnsToResearchById(int32 PlayerId, FName TechId) const;

	const FConquestTechRow* GetCurrentResearchRow(int32 PlayerId) const;

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;

	int32 CalculateEmpireSciencePerTurn(int32 PlayerId) const;
	bool CanResearchTech(int32 PlayerId, FName TechId) const;
	void CacheCurrentResearchProgress(FConquestPlayerEmpireState& Player) const;
	float GetCachedResearchProgress(const FConquestPlayerEmpireState& Player, FName TechId) const;
	void ClearCachedResearchProgress(FConquestPlayerEmpireState& Player, FName TechId) const;
};
