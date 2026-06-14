#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "ConquestTurnManager.generated.h"

class AConquestGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, int32, NewTurn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnPhaseChanged, EConquestTurnPhase, NewPhase);

UCLASS(BlueprintType)
class CONQUEST_API UConquestTurnManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AConquestGameState* InGameState);

	UPROPERTY(BlueprintAssignable)
	FOnTurnChanged OnTurnChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTurnPhaseChanged OnTurnPhaseChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CurrentTurn = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EConquestTurnPhase CurrentPhase = EConquestTurnPhase::None;

	UFUNCTION(BlueprintCallable)
	void BeginGameSetup();

	UFUNCTION(BlueprintCallable)
	void EnterAwaitingFirstCity();

	UFUNCTION(BlueprintCallable)
	void BeginFirstTurn();

	UFUNCTION(BlueprintCallable)
	void RequestEndTurn();

	UFUNCTION(BlueprintCallable)
	void SetPhase(EConquestTurnPhase NewPhase);

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;

	void StartTurnProcessing();
	void EnterPlayerActions();
	void EndTurnProcessing();
};