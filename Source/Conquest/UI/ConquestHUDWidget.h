#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/World/Generation/HexMapTypePresets.h"
#include "ConquestHUDWidget.generated.h"

class AConquestHUD;
class UConquestMainMenuWidget;
class UConquestGameSetupWidget;
class UConquestGameWidget;

UCLASS()
class CONQUEST_API UConquestHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void InitializeHUDWidget(
		AConquestHUD* InOwningHUD,
		TSubclassOf<UConquestMainMenuWidget> InMainMenuWidgetClass,
		TSubclassOf<UConquestGameSetupWidget> InGameSetupWidgetClass,
		TSubclassOf<UConquestGameWidget> InGameWidgetClass
	);

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowGameSetup();

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowGame();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void RequestStartGame(EHexMapTypePreset MapPreset);

protected:
	UPROPERTY(Transient)
	TObjectPtr<AConquestHUD> OwningConquestHUD = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CurrentScreenWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UConquestMainMenuWidget> MainMenuWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UConquestGameSetupWidget> GameSetupWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UConquestGameWidget> GameWidget = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<UConquestMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(Transient)
	TSubclassOf<UConquestGameSetupWidget> GameSetupWidgetClass;

	UPROPERTY(Transient)
	TSubclassOf<UConquestGameWidget> GameWidgetClass;

	void HideCurrentScreen();
};