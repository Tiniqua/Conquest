#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConquestMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UConquestHUDWidget;
class UConquestMultiplayerSessionSubsystem;

UCLASS()
class CONQUEST_API UConquestMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Main Menu")
	void InitializeMainMenuWidget(UConquestHUDWidget* InParentHUDWidget);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HostGameButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> JoinGameButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LoadingStatusText = nullptr;

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

	UPROPERTY(Transient)
	TObjectPtr<UConquestHUDWidget> ParentHUDWidget = nullptr;

	UFUNCTION()
	void HandleHostGameButtonClicked();

	UFUNCTION()
	void HandleJoinGameButtonClicked();

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

	UFUNCTION(BlueprintPure, Category = "Conquest|Multiplayer")
	FText GetLoadingStatus() const;

	void SetLoadingStatus(const FText& NewLoadingStatus);
	void SetMenuButtonsEnabled(bool bEnabled);
	UConquestMultiplayerSessionSubsystem* GetMultiplayerSessionSubsystem() const;
};
