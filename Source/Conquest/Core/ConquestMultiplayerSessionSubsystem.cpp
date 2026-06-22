#include "ConquestMultiplayerSessionSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

namespace
{
	const FName ConquestSessionName(TEXT("ConquestGameSession"));
	const FName ConquestPresenceKey(TEXT("CONQUEST_PRESENCE"));

	IOnlineSessionPtr GetSessionInterface()
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	}
}

void UConquestMultiplayerSessionSubsystem::HostSession(int32 PublicConnections, bool bUseLan)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false, ConquestSessionName);
		return;
	}

	if (SessionInterface->GetNamedSession(ConquestSessionName))
	{
		SessionInterface->DestroySession(ConquestSessionName);
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = bUseLan;
	SessionSettings.NumPublicConnections = FMath::Max(1, PublicConnections);
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.Set(ConquestPresenceKey, FString(TEXT("Conquest")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(
			this,
			&UConquestMultiplayerSessionSubsystem::HandleCreateSessionComplete
		)
	);

	SessionInterface->CreateSession(0, ConquestSessionName, SessionSettings);
}

void UConquestMultiplayerSessionSubsystem::FindSessions(int32 MaxResults, bool bUseLan)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = FMath::Max(1, MaxResults);
	LastSessionSearch->bIsLanQuery = bUseLan;
	LastSessionSearch->QuerySettings.Set(ConquestPresenceKey, FString(TEXT("Conquest")), EOnlineComparisonOp::Equals);

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(
			this,
			&UConquestMultiplayerSessionSubsystem::HandleFindSessionsComplete
		)
	);

	SessionInterface->FindSessions(0, LastSessionSearch.ToSharedRef());
}

void UConquestMultiplayerSessionSubsystem::JoinSessionByIndex(int32 ResultIndex)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || !LastSessionSearch.IsValid() || !LastSessionSearch->SearchResults.IsValidIndex(ResultIndex))
	{
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(
			this,
			&UConquestMultiplayerSessionSubsystem::HandleJoinSessionComplete
		)
	);

	SessionInterface->JoinSession(0, ConquestSessionName, LastSessionSearch->SearchResults[ResultIndex]);
}

int32 UConquestMultiplayerSessionSubsystem::GetLastSessionSearchResultCount() const
{
	return LastSessionSearch.IsValid() ? LastSessionSearch->SearchResults.Num() : 0;
}

void UConquestMultiplayerSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}

	OnCreateSessionComplete.Broadcast(bWasSuccessful, SessionName);
}

void UConquestMultiplayerSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	OnFindSessionsComplete.Broadcast(
		bWasSuccessful && LastSessionSearch.IsValid()
			? LastSessionSearch->SearchResults.Num()
			: 0
	);
}

void UConquestMultiplayerSessionSubsystem::HandleJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type Result
)
{
	FString TravelURL;
	const bool bWasSuccessful = Result == EOnJoinSessionCompleteResult::Success;

	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		if (bWasSuccessful)
		{
			SessionInterface->GetResolvedConnectString(SessionName, TravelURL);
		}
	}

	OnJoinSessionComplete.Broadcast(bWasSuccessful, TravelURL);
}
