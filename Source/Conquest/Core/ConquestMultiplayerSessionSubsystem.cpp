#include "ConquestMultiplayerSessionSubsystem.h"

#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

namespace
{
	const FName ConquestSessionName = NAME_GameSession;
	const FName ConquestPresenceKey(TEXT("CONQUEST_PRESENCE"));

	IOnlineSessionPtr GetSessionInterface()
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	}

	TSharedPtr<const FUniqueNetId> GetLocalUserId()
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (!OnlineSubsystem)
		{
			return nullptr;
		}

		IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
		return IdentityInterface.IsValid() ? IdentityInterface->GetUniquePlayerId(0) : nullptr;
	}
}

void UConquestMultiplayerSessionSubsystem::HostSession(int32 PublicConnections, bool bUseLan)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!SessionInterface.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false, ConquestSessionName);
		return;
	}

	TSharedPtr<const FUniqueNetId> LocalUserId = GetLocalUserId();
	if (!bUseLan && !LocalUserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: cannot create Steam session because local Steam identity is not ready."));
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
	SessionSettings.bUsesPresence = !bUseLan;
	SessionSettings.bUseLobbiesIfAvailable = !bUseLan;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = !bUseLan;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
	SessionSettings.Set(ConquestPresenceKey, FString(TEXT("Conquest")), EOnlineDataAdvertisementType::ViaOnlineService);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Conquest sessions: creating %s session through OnlineSubsystem=%s, public connections=%d"),
		bUseLan ? TEXT("LAN") : TEXT("online"),
		OnlineSubsystem ? *OnlineSubsystem->GetSubsystemName().ToString() : TEXT("None"),
		SessionSettings.NumPublicConnections
	);

	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(
			this,
			&UConquestMultiplayerSessionSubsystem::HandleCreateSessionComplete
		)
	);

	const bool bCreateStarted = LocalUserId.IsValid()
		? SessionInterface->CreateSession(*LocalUserId, ConquestSessionName, SessionSettings)
		: SessionInterface->CreateSession(0, ConquestSessionName, SessionSettings);

	if (!bCreateStarted)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: CreateSession failed to start."));
		OnCreateSessionComplete.Broadcast(false, ConquestSessionName);
	}
}

void UConquestMultiplayerSessionSubsystem::FindSessions(int32 MaxResults, bool bUseLan)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!SessionInterface.IsValid())
	{
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	if (!bUseLan && !GetLocalUserId().IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: cannot search Steam sessions because local Steam identity is not ready."));
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = FMath::Max(1, MaxResults);
	LastSessionSearch->bIsLanQuery = bUseLan;
	// Steam lobby searches can reject custom predicates before returning candidates, especially with AppId 480.
	// Query the lobby surface first, then filter to Conquest sessions locally in HandleFindSessionsComplete.
#if defined(SEARCH_LOBBIES)
	if (!bUseLan)
	{
		LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}
#endif

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Conquest sessions: searching %s sessions through OnlineSubsystem=%s, max results=%d"),
		bUseLan ? TEXT("LAN") : TEXT("online"),
		OnlineSubsystem ? *OnlineSubsystem->GetSubsystemName().ToString() : TEXT("None"),
		LastSessionSearch->MaxSearchResults
	);

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(
			this,
			&UConquestMultiplayerSessionSubsystem::HandleFindSessionsComplete
		)
	);

	if (!SessionInterface->FindSessions(0, LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: FindSessions failed to start."));
		OnFindSessionsComplete.Broadcast(0);
	}
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

	if (!SessionInterface->JoinSession(0, ConquestSessionName, LastSessionSearch->SearchResults[ResultIndex]))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: JoinSession failed to start."));
		OnJoinSessionComplete.Broadcast(false, FString());
	}
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

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Conquest sessions: create session %s completed with success=%s"),
		*SessionName.ToString(),
		bWasSuccessful ? TEXT("true") : TEXT("false")
	);

	if (bWasSuccessful)
	{
		if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
		{
			SessionInterface->StartSession(SessionName);
		}
	}

	OnCreateSessionComplete.Broadcast(bWasSuccessful, SessionName);
}

void UConquestMultiplayerSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	if (bWasSuccessful && LastSessionSearch.IsValid())
	{
		const int32 UnfilteredResultCount = LastSessionSearch->SearchResults.Num();
		LastSessionSearch->SearchResults.RemoveAllSwap([](const FOnlineSessionSearchResult& SearchResult)
		{
			FString ProjectValue;
			return !SearchResult.Session.SessionSettings.Get(ConquestPresenceKey, ProjectValue)
				|| ProjectValue != TEXT("Conquest");
		});

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Conquest sessions: find sessions returned %d raw results and %d Conquest results"),
			UnfilteredResultCount,
			LastSessionSearch->SearchResults.Num()
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: find sessions completed with success=false."));
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
			if (!SessionInterface->GetResolvedConnectString(SessionName, TravelURL, NAME_GamePort))
			{
				SessionInterface->GetResolvedConnectString(SessionName, TravelURL);
			}
		}
	}

	OnJoinSessionComplete.Broadcast(bWasSuccessful, TravelURL);
}
