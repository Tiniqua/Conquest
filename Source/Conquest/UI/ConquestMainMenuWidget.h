#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Conquest/UI/ConquestMainMenuButton.h"
#include "ConquestMainMenuWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UWidget;
class UConquestHUDWidget;
class UConquestMultiplayerSessionSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FConquestMainMenuActionSelectedSignature,
	EConquestMainMenuButtonAction,
	Action,
	UConquestMainMenuButton*,
	Button
);

UCLASS()
class CONQUEST_API UConquestMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Main Menu")
	void InitializeMainMenuWidget(UConquestHUDWidget* InParentHUDWidget);

	UPROPERTY(BlueprintAssignable, Category = "Conquest|Main Menu")
	FConquestMainMenuActionSelectedSignature OnMainMenuActionSelected;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SingleplayerButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MultiplayerButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HostGameButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> JoinGameButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReturnButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LoadingStatusText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> MenuButtonVertBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Multiplayer", meta = (ClampMin = "1"))
	int32 MaxSessionSearchResults = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Multiplayer", meta = (ClampMin = "1"))
	int32 PublicConnections = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Multiplayer")
	// Local standalone/listen-server testing:
	bool bUseLanSessions = true;
	// Packaged Steam multiplayer testing:
	// bool bUseLanSessions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Multiplayer", meta = (ClampMin = "0"))
	int32 JoinSessionResultIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Multiplayer")
	FText LoadingStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu|Animation")
	bool bScaleButtonsOnHover = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu|Animation", meta = (EditCondition = "bScaleButtonsOnHover", ClampMin = "0.01"))
	float ButtonHoverScale = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu|Animation", meta = (EditCondition = "bScaleButtonsOnHover", ClampMin = "0.01"))
	float ButtonDefaultScale = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> ParentHUDWidget = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UConquestMainMenuButton>> BoundMainMenuButtons;

	UFUNCTION()
	void HandleMainMenuButtonClicked(UConquestMainMenuButton* Button);

	UFUNCTION()
	void HandleSingleplayerButtonClicked();

	UFUNCTION()
	void HandleSingleplayerButtonHovered();

	UFUNCTION()
	void HandleSingleplayerButtonUnhovered();

	UFUNCTION()
	void HandleMultiplayerButtonClicked();

	UFUNCTION()
	void HandleMultiplayerButtonHovered();

	UFUNCTION()
	void HandleMultiplayerButtonUnhovered();

	UFUNCTION()
	void HandleSettingsButtonClicked();

	UFUNCTION()
	void HandleSettingsButtonHovered();

	UFUNCTION()
	void HandleSettingsButtonUnhovered();

	UFUNCTION()
	void HandleQuitButtonClicked();

	UFUNCTION()
	void HandleQuitButtonHovered();

	UFUNCTION()
	void HandleQuitButtonUnhovered();

	UFUNCTION()
	void HandleHostGameButtonClicked();

	UFUNCTION()
	void HandleHostGameButtonHovered();

	UFUNCTION()
	void HandleHostGameButtonUnhovered();

	UFUNCTION()
	void HandleJoinGameButtonClicked();

	UFUNCTION()
	void HandleJoinGameButtonHovered();

	UFUNCTION()
	void HandleJoinGameButtonUnhovered();

	UFUNCTION()
	void HandleReturnButtonClicked();

	UFUNCTION()
	void HandleReturnButtonHovered();

	UFUNCTION()
	void HandleReturnButtonUnhovered();

	UFUNCTION()
	void HandleCreateSessionComplete(bool bWasSuccessful, FName SessionName);

	UFUNCTION()
	void HandleFindSessionsComplete(int32 ResultCount);

	UFUNCTION()
	void HandleJoinSessionComplete(bool bWasSuccessful, FString TravelURL);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Multiplayer")
	void OnFindSessionsFinished(int32 ResultCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Multiplayer")
	void OnJoinSessionFinished(bool bWasSuccessful, const FString& TravelURL);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Main Menu")
	void OnMultiplayerRequested(UConquestMainMenuButton* Button);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Main Menu")
	void OnSettingsRequested(UConquestMainMenuButton* Button);

	UFUNCTION(BlueprintImplementableEvent, Category = "Conquest|Main Menu")
	void OnCustomMainMenuButtonRequested(UConquestMainMenuButton* Button, FName CustomActionName);

	UFUNCTION(BlueprintPure, Category = "Conquest|Multiplayer")
	FText GetLoadingStatus() const;

	void BindMainMenuButtons();
	void UnbindMainMenuButtons();
	EConquestMainMenuButtonAction ResolveButtonAction(const UConquestMainMenuButton* Button) const;
	EConquestMainMenuButtonAction ResolveButtonActionFromName(FName ButtonName) const;
	void ExecuteMainMenuAction(EConquestMainMenuButtonAction Action, UConquestMainMenuButton* Button);
	void ShowPrimaryMainMenuButtons();
	void ShowMultiplayerMenuButtons();
	void ConfigureBoundMainMenuButtonHoverScale();
	void ApplyButtonHoverScale(UButton* Button) const;
	void ApplyButtonDefaultScale(UButton* Button) const;
	void RefreshMenuButtonSpacerVisibility();
	UPanelWidget* GetMenuButtonPanel() const;
	bool HasVisibleNonSpacerChildBefore(const UPanelWidget* Panel, int32 ChildIndex) const;
	bool HasVisibleNonSpacerChildAfter(const UPanelWidget* Panel, int32 ChildIndex) const;
	bool IsVisibleNonSpacerWidget(const UWidget* Widget) const;
	void SetLoadingStatus(const FText& NewLoadingStatus);
	void SetMenuButtonsEnabled(bool bEnabled);
	UConquestMultiplayerSessionSubsystem* GetMultiplayerSessionSubsystem() const;
};
