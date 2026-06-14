#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ConquestGameMode.generated.h"

UCLASS()
class CONQUEST_API AConquestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AConquestGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void StartSinglePlayerGame();

	UFUNCTION(BlueprintCallable)
	void EndCurrentTurn();

	UFUNCTION(BlueprintCallable)
	bool FoundStartingCity(const FIntPoint& TileCoord, FName CityName);
};