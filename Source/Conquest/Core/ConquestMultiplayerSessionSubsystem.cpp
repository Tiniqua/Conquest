#include "ConquestMultiplayerSessionSubsystem.h"

#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FName ConquestSessionName = NAME_GameSession;
	const FName ConquestPresenceKey(TEXT("CONQUEST_PRESENCE"));
	const FString ConquestPresenceValue(TEXT("Conquest"));
	
	IOnlineSessionPtr GetSessionInterface()
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	}

	bool ShouldUseLanSessions(bool bRequestedUseLan)
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		return bRequestedUseLan
			|| !OnlineSubsystem
			|| OnlineSubsystem->GetSubsystemName() == FName(TEXT("NULL"));
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

void UConquestMultiplayerSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionUserInviteAcceptedHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(
				this,
				&UConquestMultiplayerSessionSubsystem::HandleSessionUserInviteAccepted
			)
		);

		SessionInviteReceivedHandle = SessionInterface->AddOnSessionInviteReceivedDelegate_Handle(
			FOnSessionInviteReceivedDelegate::CreateUObject(
				this,
				&UConquestMultiplayerSessionSubsystem::HandleSessionInviteReceived
			)
		);
	}
}

void UConquestMultiplayerSessionSubsystem::Deinitialize()
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionUserInviteAcceptedHandle);
		SessionInterface->ClearOnSessionInviteReceivedDelegate_Handle(SessionInviteReceivedHandle);
	}

	Super::Deinitialize();
}

void UConquestMultiplayerSessionSubsystem::HostSession(int32 PublicConnections, bool bUseLan)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	bUseLan = ShouldUseLanSessions(bUseLan);
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
	SessionSettings.bAllowInvites = !bUseLan;
	SessionSettings.bAllowJoinViaPresence = !bUseLan;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
	// Local LAN searches need this in ping data so the Conquest filter can see it through OnlineSubsystemNull.
	SessionSettings.Set(ConquestPresenceKey, ConquestPresenceValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// Packaged Steam-only filtering:
	// SessionSettings.Set(ConquestPresenceKey, ConquestPresenceValue, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_MAPNAME, UWorld::RemovePIEPrefix(GetWorld() ? GetWorld()->GetMapName() : FString()), EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_GAMEMODE, FString(TEXT("Conquest")), EOnlineDataAdvertisementType::ViaOnlineService);

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
	LastRequestedMaxSearchResults = FMath::Max(1, MaxResults);
	bLastSearchWasLan = ShouldUseLanSessions(bUseLan);
	bRetryingWithoutConquestFilter = false;

	StartFindSessions(LastRequestedMaxSearchResults, bLastSearchWasLan, true);
}

void UConquestMultiplayerSessionSubsystem::StartFindSessions(
	int32 MaxResults,
	bool bUseLan,
	bool bUseConquestServiceFilter
)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	bUseLan = ShouldUseLanSessions(bUseLan);
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
#if defined(SEARCH_LOBBIES)
	if (!bUseLan)
	{
		LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}
#endif
#if defined(SEARCH_PRESENCE)
	if (!bUseLan)
	{
		LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	}
#endif

	if (!bUseLan && bUseConquestServiceFilter)
	{
		LastSessionSearch->QuerySettings.Set(ConquestPresenceKey, ConquestPresenceValue, EOnlineComparisonOp::Equals);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Conquest sessions: searching %s sessions through OnlineSubsystem=%s, max results=%d, conquest service filter=%s"),
		bUseLan ? TEXT("LAN") : TEXT("online"),
		OnlineSubsystem ? *OnlineSubsystem->GetSubsystemName().ToString() : TEXT("None"),
		LastSessionSearch->MaxSearchResults,
		bUseConquestServiceFilter ? TEXT("true") : TEXT("false")
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

	bAutoTravelAfterJoin = true;

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

void UConquestMultiplayerSessionSubsystem::DestroyCurrentSession()
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		if (SessionInterface->GetNamedSession(ConquestSessionName))
		{
			SessionInterface->DestroySession(ConquestSessionName);
		}
	}
}

bool UConquestMultiplayerSessionSubsystem::SendSessionInviteToFriend(const FString& FriendUniqueNetId)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || FriendUniqueNetId.IsEmpty())
	{
		return false;
	}

	const TSharedPtr<const FUniqueNetId> LocalUserId = GetLocalUserId();
	if (!LocalUserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: cannot send Steam invite because local Steam identity is not ready."));
		return false;
	}

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	const IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> FriendId = IdentityInterface.IsValid()
		? IdentityInterface->CreateUniquePlayerId(FriendUniqueNetId)
		: nullptr;

	if (!FriendId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: cannot send invite because friend id '%s' is not a valid unique net id."), *FriendUniqueNetId);
		return false;
	}

	return SessionInterface->SendSessionInviteToFriend(0, ConquestSessionName, *FriendId);
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
		TArray<FOnlineSessionSearchResult> ConquestResults;
		ConquestResults.Reserve(LastSessionSearch->SearchResults.Num());

		for (const FOnlineSessionSearchResult& SearchResult : LastSessionSearch->SearchResults)
		{
			FString ProjectValue;
			const bool bHasConquestTag = SearchResult.Session.SessionSettings.Get(ConquestPresenceKey, ProjectValue);
			const bool bIsConquestSession = bHasConquestTag && ProjectValue == ConquestPresenceValue;

			UE_LOG(
				LogTemp,
				Log,
				TEXT("Conquest sessions: raw result owner=%s hasConquestTag=%s tagValue=%s openPublic=%d"),
				*SearchResult.Session.OwningUserName,
				bHasConquestTag ? TEXT("true") : TEXT("false"),
				bHasConquestTag ? *ProjectValue : TEXT("<none>"),
				SearchResult.Session.NumOpenPublicConnections
			);

			if (bIsConquestSession)
			{
				ConquestResults.Add(SearchResult);
			}
		}

		LastSessionSearch->SearchResults = MoveTemp(ConquestResults);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Conquest sessions: find sessions returned %d raw results and %d Conquest results"),
			UnfilteredResultCount,
			LastSessionSearch->SearchResults.Num()
		);

		if (!bLastSearchWasLan
			&& !bRetryingWithoutConquestFilter
			&& LastSessionSearch->SearchResults.Num() == 0)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Conquest sessions: filtered Steam lobby search returned no results, retrying broad lobby search with local Conquest filtering.")
			);
			bRetryingWithoutConquestFilter = true;
			StartFindSessions(LastRequestedMaxSearchResults, bLastSearchWasLan, false);
			return;
		}
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

	if (bWasSuccessful && bAutoTravelAfterJoin)
	{
		TravelToJoinedSession(TravelURL);
	}

	bAutoTravelAfterJoin = false;
}

void UConquestMultiplayerSessionSubsystem::HandleSessionUserInviteAccepted(
	const bool bWasSuccessful,
	const int32 ControllerId,
	TSharedPtr<const FUniqueNetId> UserId,
	const FOnlineSessionSearchResult& InviteResult
)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: Steam session invite was accepted but reported unsuccessful."));
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false, FString());
		return;
	}

	bAutoTravelAfterJoin = true;

	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(
			this,
			&UConquestMultiplayerSessionSubsystem::HandleJoinSessionComplete
		)
	);

	if (!SessionInterface->JoinSession(ControllerId, ConquestSessionName, InviteResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		bAutoTravelAfterJoin = false;
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: JoinSession from Steam invite failed to start."));
		OnJoinSessionComplete.Broadcast(false, FString());
	}
}

void UConquestMultiplayerSessionSubsystem::HandleSessionInviteReceived(
	const FUniqueNetId& UserId,
	const FUniqueNetId& FromId,
	const FString& AppId,
	const FOnlineSessionSearchResult& InviteResult
)
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Conquest sessions: received Steam session invite from %s for app %s."),
		*FromId.ToDebugString(),
		*AppId
	);
}

void UConquestMultiplayerSessionSubsystem::TravelToJoinedSession(const FString& TravelURL) const
{
	if (TravelURL.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("Conquest sessions: cannot travel after Steam invite because no local player controller is available."));
		return;
	}

	const FString TravelSeparator = TravelURL.Contains(TEXT("?")) ? TEXT("&") : TEXT("?");
	PlayerController->ClientTravel(TravelURL + TravelSeparator + TEXT("ConquestJoinSetup=1"), TRAVEL_Absolute);
}
