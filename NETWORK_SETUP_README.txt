Conquest Network Setup Notes
============================

Purpose
-------
This file lists the current project network/session touch points and the manual toggles needed to switch between:

1. Local editor standalone/listen-server testing with Null OSS.
2. Packaged Steam executable testing with Steam OSS and SteamSockets.

Do not leave both local and Steam net drivers active at the same time. DefaultEngine.ini clears NetDriverDefinitions first, so exactly one GameNetDriver line should be uncommented.


Project Files Involved
----------------------
Config:
- Config/DefaultEngine.ini
  - Selects DefaultPlatformService.
  - Enables OnlineSubsystemNull and OnlineSubsystemSteam.
  - Selects the active GameNetDriver.
  - Configures SteamNetConnection or SteamSocketsNetConnection.

Plugin/module setup:
- Conquest.uproject
  - OnlineSubsystemSteam plugin is enabled.
  - SteamSockets plugin is enabled.
- Source/Conquest/Conquest.Build.cs
  - PublicDependencyModuleNames includes OnlineSubsystem and OnlineSubsystemUtils.
  - DynamicallyLoadedModuleNames includes OnlineSubsystemNull and OnlineSubsystemSteam.

C++ session/hosting flow:
- Source/Conquest/Core/ConquestMultiplayerSessionSubsystem.h/.cpp
  - HostSession(int32 PublicConnections, bool bUseLan)
  - FindSessions(int32 MaxResults, bool bUseLan)
  - JoinSessionByIndex(int32 ResultIndex)
  - HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
  - HandleFindSessionsComplete(bool bWasSuccessful)
  - HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
  - Uses IOnlineSubsystem::Get(), GetSessionInterface(), GetIdentityInterface(), GetUniquePlayerId(0).
  - Uses NAME_GameSession as the session name.
  - Uses CONQUEST_PRESENCE to tag and filter Conquest sessions.
  - For Steam/non-LAN: requires a valid local Steam identity before hosting/searching.
  - For Steam/non-LAN: sets bUsesPresence=true, bUseLobbiesIfAvailable=true, bAllowJoinViaPresence=true.
  - For LAN: sets bIsLANMatch=true and does not require Steam identity.

Main menu flow:
- Source/Conquest/UI/ConquestMainMenuWidget.h/.cpp
  - Host button calls SessionSubsystem->HostSession(PublicConnections, bUseLanSessions).
  - Join button calls SessionSubsystem->FindSessions(MaxSessionSearchResults, bUseLanSessions), then JoinSessionByIndex(JoinSessionResultIndex).
  - On successful host session creation, opens the current level with:
    ?listen?ConquestHostSetup=1
  - On successful join, calls ClientTravel(TravelURL + "?ConquestJoinSetup=1", TRAVEL_Absolute).
  - bUseLanSessions is the C++/BP toggle between LAN/null style sessions and Steam online sessions.


Current Packaged Steam Setup
----------------------------
DefaultEngine.ini is currently configured for packaged Steam executable testing:

[OnlineSubsystem]
;DefaultPlatformService=Null
DefaultPlatformService=Steam

[OnlineSubsystemNull]
;bEnabled=True
bEnabled=True

[OnlineSubsystemSteam]
bEnabled=True
SteamDevAppId=480
SteamAppId=480
bInitServerOnClient=True
bRelaunchInSteam=False
GameServerQueryPort=27015
bAllowP2PPacketRelay=True
P2PConnectionTimeout=90

[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="/Script/OnlineSubsystemSteam.SteamNetConnection"

[/Script/SteamSockets.SteamSocketsNetDriver]
NetConnectionClassName="/Script/SteamSockets.SteamSocketsNetConnection"

Main menu/session setting:
- Source/Conquest/UI/ConquestMainMenuWidget.h
  - bUseLanSessions should be false for Steam online sessions.

Packaged Steam test requirements:
- Steam must be running.
- Each PC should be logged into a different Steam account.
- For direct AppId 480 executable tests, put steam_appid.txt beside the packaged game exe.
- steam_appid.txt should contain only:
  480
- If Steam OSS fails to initialize, hosting/searching will fail because non-LAN sessions require a valid Steam identity.
- In logs, a healthy Steam path should not fall back to OnlineSubsystem=NULL.


Toggle To Local Editor Standalone / Null OSS Listen Testing
----------------------------------------------------------
Use this when testing local standalone/listen-server behavior without Steam.

DefaultEngine.ini:

1. In [OnlineSubsystem], uncomment Null and comment Steam:

[OnlineSubsystem]
DefaultPlatformService=Null
;DefaultPlatformService=Steam

2. In [OnlineSubsystemNull], enable Null:

[OnlineSubsystemNull]
bEnabled=True

3. In [OnlineSubsystemSteam], comment Steam settings if you want a clean local config:

[OnlineSubsystemSteam]
;bEnabled=True
;SteamDevAppId=480
;SteamAppId=480
;bInitServerOnClient=True
;bRelaunchInSteam=False
;GameServerQueryPort=27015
;bAllowP2PPacketRelay=True
;P2PConnectionTimeout=90

4. In [/Script/Engine.Engine], use IpNetDriver and comment both Steam drivers:

[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

5. Net connection classes can remain present, but commenting them keeps the active path obvious:

[/Script/OnlineSubsystemSteam.SteamNetDriver]
;NetConnectionClassName="/Script/OnlineSubsystemSteam.SteamNetConnection"

[/Script/SteamSockets.SteamSocketsNetDriver]
;NetConnectionClassName="/Script/SteamSockets.SteamSocketsNetConnection"

Main menu/session setting:
- Source/Conquest/UI/ConquestMainMenuWidget.h
  - Set bUseLanSessions to true for local LAN/null style session queries:

// Local standalone/listen-server testing:
bool bUseLanSessions = true;
// Packaged Steam multiplayer testing:
// bool bUseLanSessions = false;

Notes:
- Host still opens the current level with ?listen?ConquestHostSetup=1.
- Join still uses the resolved session travel URL and adds ?ConquestJoinSetup=1.
- Null OSS does not require a valid Steam identity.


Toggle To Packaged Steam EXE / Steam OSS Testing
------------------------------------------------
Use this when packaging and testing through Steam OSS with SteamSockets.

DefaultEngine.ini:

1. In [OnlineSubsystem], comment Null and uncomment Steam:

[OnlineSubsystem]
;DefaultPlatformService=Null
DefaultPlatformService=Steam

2. In [OnlineSubsystemNull], keeping Null enabled is acceptable as a module fallback, but it should not be the default platform service:

[OnlineSubsystemNull]
bEnabled=True

3. In [OnlineSubsystemSteam], uncomment Steam settings:

[OnlineSubsystemSteam]
bEnabled=True
SteamDevAppId=480
SteamAppId=480
bInitServerOnClient=True
bRelaunchInSteam=False
GameServerQueryPort=27015
bAllowP2PPacketRelay=True
P2PConnectionTimeout=90

4. In [/Script/Engine.Engine], comment IpNetDriver and SteamNetDriver, then use SteamSocketsNetDriver:

[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

5. Ensure SteamSockets connection is uncommented:

[/Script/SteamSockets.SteamSocketsNetDriver]
NetConnectionClassName="/Script/SteamSockets.SteamSocketsNetConnection"

6. The non-SteamSockets Steam driver should stay commented unless deliberately testing the older SteamNetDriver path:

[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="/Script/OnlineSubsystemSteam.SteamNetConnection"

Main menu/session setting:
- Source/Conquest/UI/ConquestMainMenuWidget.h
  - Set bUseLanSessions to false for Steam online sessions:

// Local standalone/listen-server testing:
// bool bUseLanSessions = true;
// Packaged Steam multiplayer testing:
bool bUseLanSessions = false;

Packaged test checklist:
- Package the game.
- Place steam_appid.txt beside the packaged exe with contents: 480
- Run Steam on each PC.
- Use different Steam accounts on each PC.
- Host on one PC.
- Join/search from the other PC.
- If the host button says "Could not host game", check the log for:
  - "Unable to create OnlineSubsystem instance Steam"
  - "Created online subsystem instance for: NULL"
  - "cannot create Steam session because local Steam identity is not ready"


Optional SteamNetDriver Path
----------------------------
There is also a commented non-SteamSockets Steam driver line:

+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

Only use this if deliberately testing without SteamSockets.

For that path:
- Uncomment the OnlineSubsystemSteam.SteamNetDriver line.
- Comment the SteamSockets.SteamSocketsNetDriver line.
- Keep this connection class uncommented:

[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="/Script/OnlineSubsystemSteam.SteamNetConnection"

- Comment or ignore the SteamSockets connection section.


Quick Symptoms
--------------
"Could not host game" immediately:
- UConquestMultiplayerSessionSubsystem::HostSession failed before or during CreateSession.
- For Steam mode, most common cause is Steam OSS not initialized or local Steam identity missing.

"0 found" when joining:
- UConquestMultiplayerSessionSubsystem::FindSessions returned no filtered Conquest results.
- Check that both clients use the same mode: both LAN/null or both Steam.
- In Steam mode, confirm both clients are running under Steam with valid identities.

Host opens level but client cannot connect:
- Session creation succeeded, but travel/net driver path may be wrong.
- Check active GameNetDriver in DefaultEngine.ini.
- Local/null should use IpNetDriver.
- Packaged Steam should use SteamSocketsNetDriver.

