#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ConquestGameMode.generated.h"

class UConquestCivilisationData;

UCLASS()
class CONQUEST_API AConquestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AConquestGameMode();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conquest|Game Setup")
	TArray<TObjectPtr<UConquestCivilisationData>> AvailableCivilisations;

	UFUNCTION(BlueprintPure, Category = "Conquest|Game Setup")
	TArray<UConquestCivilisationData*> GetAvailableCivilisations() const;

	UFUNCTION(BlueprintCallable)
	void StartSinglePlayerGame();

	UFUNCTION(BlueprintCallable)
	void EndCurrentTurn();

	UFUNCTION(BlueprintCallable)
	bool CanEndCurrentTurn(UPARAM(ref) FText& OutBlockReason) const;

	UFUNCTION(BlueprintCallable)
	bool FoundStartingCity(const FIntPoint& TileCoord, FName CityName);
};
