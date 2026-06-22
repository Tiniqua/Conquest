#include "ConquestMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Conquest/Core/ConquestMultiplayerSessionSubsystem.h"
#include "ConquestHUDWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UConquestMainMenuWidget::InitializeMainMenuWidget(UConquestHUDWidget* InParentHUDWidget)
{
	ParentHUDWidget = InParentHUDWidget;
}

void UConquestMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HostGameButton)
	{
		HostGameButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonClicked);
		HostGameButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonClicked);
	}

	if (JoinGameButton)
	{
		JoinGameButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonClicked);
		JoinGameButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonClicked);
	}

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleCreateSessionComplete);
		SessionSubsystem->OnCreateSessionComplete.AddDynamic(this, &UConquestMainMenuWidget::HandleCreateSessionComplete);

		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleFindSessionsComplete);
		SessionSubsystem->OnFindSessionsComplete.AddDynamic(this, &UConquestMainMenuWidget::HandleFindSessionsComplete);

		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinSessionComplete);
		SessionSubsystem->OnJoinSessionComplete.AddDynamic(this, &UConquestMainMenuWidget::HandleJoinSessionComplete);
	}

	SetLoadingStatus(FText::GetEmpty());
	SetMenuButtonsEnabled(true);
}

void UConquestMainMenuWidget::NativeDestruct()
{
	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleCreateSessionComplete);
		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleFindSessionsComplete);
		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinSessionComplete);
	}

	Super::NativeDestruct();
}

void UConquestMainMenuWidget::HandleHostGameButtonClicked()
{
	SetMenuButtonsEnabled(false);
	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuHostingGame", "Hosting game"));

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->HostSession(PublicConnections, bUseLanSessions);
	}
	else
	{
		HandleCreateSessionComplete(false, NAME_None);
	}
}

void UConquestMainMenuWidget::HandleJoinGameButtonClicked()
{
	SetMenuButtonsEnabled(false);
	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuSearchingSessions", "Searching"));

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->FindSessions(MaxSessionSearchResults, bUseLanSessions);
	}
	else
	{
		HandleFindSessionsComplete(0);
	}
}

void UConquestMainMenuWidget::HandleCreateSessionComplete(bool bWasSuccessful, FName)
{
	if (!bWasSuccessful)
	{
		SetMenuButtonsEnabled(true);
		SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuHostFailed", "Could not host game"));
		return;
	}

	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuHostReady", "Host ready"));

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!CurrentLevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(
			this,
			FName(*CurrentLevelName),
			true,
			TEXT("listen?ConquestHostSetup=1")
		);
	}
}

void UConquestMainMenuWidget::HandleFindSessionsComplete(int32 ResultCount)
{
	SetLoadingStatus(FText::Format(
		NSLOCTEXT("Conquest", "MainMenuSessionsFound", "{0} found"),
		FText::AsNumber(ResultCount)
	));

	OnFindSessionsFinished(ResultCount);

	if (ResultCount <= JoinSessionResultIndex)
	{
		SetMenuButtonsEnabled(true);
		return;
	}

	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuJoiningGame", "Joining game"));

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->JoinSessionByIndex(JoinSessionResultIndex);
	}
	else
	{
		HandleJoinSessionComplete(false, FString());
	}
}

void UConquestMainMenuWidget::HandleJoinSessionComplete(bool bWasSuccessful, FString TravelURL)
{
	OnJoinSessionFinished(bWasSuccessful, TravelURL);

	if (!bWasSuccessful || TravelURL.IsEmpty())
	{
		SetMenuButtonsEnabled(true);
		SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuJoinFailed", "Could not join game"));
		return;
	}

	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuJoiningGame", "Joining game"));

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->ClientTravel(TravelURL + TEXT("?ConquestJoinSetup=1"), TRAVEL_Absolute);
	}
}

void UConquestMainMenuWidget::SetLoadingStatus(const FText& NewLoadingStatus)
{
	LoadingStatus = NewLoadingStatus;

	if (LoadingStatusText)
	{
		LoadingStatusText->SetText(LoadingStatus);
	}
}

FText UConquestMainMenuWidget::GetLoadingStatus() const
{
	return LoadingStatus;
}

void UConquestMainMenuWidget::SetMenuButtonsEnabled(bool bEnabled)
{
	if (HostGameButton)
	{
		HostGameButton->SetIsEnabled(bEnabled);
	}

	if (JoinGameButton)
	{
		JoinGameButton->SetIsEnabled(bEnabled);
	}
}

UConquestMultiplayerSessionSubsystem* UConquestMainMenuWidget::GetMultiplayerSessionSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UConquestMultiplayerSessionSubsystem>() : nullptr;
}
