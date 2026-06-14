#pragma once

#include "CoreMinimal.h"
#include "Conquest/World/Generation/ConquestGameSetupTypes.h"
#include "GameFramework/HUD.h"
#include "Conquest/World/Generation/HexMapTypePresets.h"
#include "ConquestHUD.generated.h"

class UConquestResearchPanelWidget;
class UConquestCityPanelWidget;
class UConquestHUDWidget;
class UConquestMainMenuWidget;
class UConquestGameSetupWidget;
class UConquestGameWidget;
class AModularHexGridActor;

UCLASS()
class CONQUEST_API AConquestHUD : public AHUD
{
	GENERATED_BODY()

public:
	AConquestHUD();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowGameSetup();

	UFUNCTION(BlueprintCallable, Category = "Conquest|HUD")
	void ShowGame();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void RequestStartGame(const FConquestGameSetupSettings& SetupSettings);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Game Setup")
	void StartGameWithMapPreset(EHexMapTypePreset MapPreset);

	UFUNCTION(BlueprintPure, Category = "Conquest|HUD")
	UConquestGameWidget* GetGameWidget() const;

	UFUNCTION(BlueprintPure, Category = "Conquest|HUD")
	UConquestGameWidget* GetActiveGameWidget() const;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestCityPanelWidget> CityPanelWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestResearchPanelWidget> ResearchPanelWidgetClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UConquestGameWidget> GameWidget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UConquestCityPanelWidget> CityPanelWidget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UConquestResearchPanelWidget> ResearchPanelWidget;

	UFUNCTION(BlueprintCallable)
	void ShowCityPanel(int32 CityId);

	UFUNCTION(BlueprintCallable)
	void HideCityPanel();

	UFUNCTION(BlueprintCallable)
	void ShowResearchPanel();

	UFUNCTION(BlueprintCallable)
	void HideResearchPanel();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestGameSetupWidget> GameSetupWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|HUD")
	TSubclassOf<UConquestGameWidget> GameWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Conquest|World")
	TSubclassOf<AModularHexGridActor> HexGridActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|World")
	FTransform HexGridSpawnTransform = FTransform::Identity;

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> HUDWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AModularHexGridActor> SpawnedHexGridActor = nullptr;

	void ConfigureMenuInputMode();
	void ConfigureGameInputMode();
};