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
	void FindSessions(int32 MaxResults = 100, bool bUseLan = false);

	UFUNCTION(BlueprintCallable, Category="Conquest|Multiplayer")
	void JoinSessionByIndex(int32 ResultIndex);

	UFUNCTION(BlueprintPure, Category="Conquest|Multiplayer")
	int32 GetLastSessionSearchResultCount() const;

private:
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
