#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ConquestCheats.generated.h"

UCLASS()
class CONQUEST_API UConquestCheats : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(exec)
	void revealFOW();

	UFUNCTION(exec)
	void ImproveAllResources();

	UFUNCTION(exec)
	void granttech(const FString& TechId);

	UFUNCTION(exec)
	void grantphil(const FString& PhilosophyId);

	UFUNCTION(exec)
	void spawnunit(const FString& UnitId);

private:
	bool GetHoveredTile(FIntPoint& OutCoord) const;
};
