#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestTileHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class CONQUEST_API UConquestTileHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile")
	void SetTileHealth(int32 CurrentHealth, int32 MaxHealth, int32 CombatStrength);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StrengthText = nullptr;
};
