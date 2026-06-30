#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ConquestMultiplayerSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConquestCreateSessionComplete, bool, bWasSuccessful, FName, SessionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConquestFindSessionsComplete, int32, ResultCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConquestJoinSessionComplete, bool, bWasSuccessful, FString, TravelURL);

class FOnlineSessionSearch;

UCLASS()
class CONQUEST_API UConquestMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="Conquest|Multiplayer")
	FOnConquestCreateSessionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category="Conquest|Multiplayer")
	FOnConquestFindSessionsComplete OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category="Conquest|Multiplayer")
	FOnConquestJoinSessionComplete OnJoinSessionComplete;

	UFUNCTION(BlueprintCallable, Category="Conquest|Multiplayer")
	void HostSession(int32 PublicConnections = 8, bool bUseLan = false);

	UFUNCTION(BlueprintCallable, Category="Conquest|Multiplayer")
	void FindSessions(int32 MaxResults = 10000, bool bUseLan = false);

	UFUNCTION(BlueprintCallable, Category="Conquest|Multiplayer")
	void JoinSessionByIndex(int32 ResultIndex);

	UFUNCTION(BlueprintCallable, Category="Conquest|Multiplayer")
	void DestroyCurrentSession();

	UFUNCTION(BlueprintCallable, Category="Conquest|Multiplayer")
	bool SendSessionInviteToFriend(const FString& FriendUniqueNetId);

	UFUNCTION(BlueprintPure, Category="Conquest|Multiplayer")
	int32 GetLastSessionSearchResultCount() const;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle SessionUserInviteAcceptedHandle;
	FDelegateHandle SessionInviteReceivedHandle;

	int32 LastRequestedMaxSearchResults = 10000;
	bool bLastSearchWasLan = false;
	bool bRetryingWithoutConquestFilter = false;
	bool bAutoTravelAfterJoin = false;

	void StartFindSessions(int32 MaxResults, bool bUseLan, bool bUseConquestServiceFilter);
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleSessionUserInviteAccepted(
		const bool bWasSuccessful,
		const int32 ControllerId,
		TSharedPtr<const FUniqueNetId> UserId,
		const FOnlineSessionSearchResult& InviteResult
	);
	void HandleSessionInviteReceived(
		const FUniqueNetId& UserId,
		const FUniqueNetId& FromId,
		const FString& AppId,
		const FOnlineSessionSearchResult& InviteResult
	);
	void TravelToJoinedSession(const FString& TravelURL) const;
};
