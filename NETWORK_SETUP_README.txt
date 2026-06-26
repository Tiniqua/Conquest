Conquest Network Setup
======================

Purpose
-------
This document describes the current multiplayer/network setup for Conquest and what is required to run a packaged Steam networked build.

The project is currently configured for Steam packaged testing by default:

- Online subsystem: Steam
- Transport driver: SteamSockets
- Session type from the main menu: online Steam lobby/session, not LAN
- Hosting model: listen server
- Test Steam App ID: 480

Do not run UE builds just to validate this document. The source/config state below was checked directly from the project files.


Current Steam Packaged Configuration
------------------------------------
The active configuration is in `Config/DefaultEngine.ini`.

Online subsystem:

```
[OnlineSubsystem]
;DefaultPlatformService=Null
DefaultPlatformService=Steam

[OnlineSubsystemNull]
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
```

Important points:

- `DefaultPlatformService=Steam` means `IOnlineSubsystem::Get()` resolves to Steam for packaged multiplayer testing.
- `OnlineSubsystemNull` is still enabled, but it is not the active subsystem.
- `bInitServerOnClient=True` is required for the listen-server style Steam session path used by the main menu.
- `bRelaunchInSteam=False` means packaged App ID 480 tests are expected to launch directly from the packaged executable, using a local `steam_appid.txt`.
- `GameServerQueryPort=27015` is configured for Steam server queries.
- `bAllowP2PPacketRelay=True` allows Steam relay fallback when direct peer connectivity is not available.

Net driver:

```
[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="/Script/OnlineSubsystemSteam.SteamNetConnection"

[/Script/SteamSockets.SteamSocketsNetDriver]
NetConnectionClassName="/Script/SteamSockets.SteamSocketsNetConnection"
```

Important points:

- The config clears `NetDriverDefinitions`, then adds exactly one active `GameNetDriver`.
- The active driver is `SteamSockets.SteamSocketsNetDriver`.
- The older `OnlineSubsystemSteam.SteamNetDriver` path is present but commented out.
- The local `IpNetDriver` path is present but commented out.
- For the current packaged Steam setup, leave SteamSockets active.


Project Plugin And Module Setup
-------------------------------
`Conquest.uproject` enables:

- `OnlineSubsystemSteam`
- `SteamSockets`

`Source/Conquest/Conquest.Build.cs` includes:

- Public dependencies: `OnlineSubsystem`, `OnlineSubsystemUtils`
- Dynamic modules: `OnlineSubsystemNull`, `OnlineSubsystemSteam`

There is no direct C++ dependency on SteamSockets classes. SteamSockets is selected through config and loaded as an enabled plugin.


Runtime Session Flow
--------------------
The current C++ session implementation is in:

- `Source/Conquest/Core/ConquestMultiplayerSessionSubsystem.h`
- `Source/Conquest/Core/ConquestMultiplayerSessionSubsystem.cpp`
- `Source/Conquest/UI/ConquestMainMenuWidget.h`
- `Source/Conquest/UI/ConquestMainMenuWidget.cpp`
- `Source/Conquest/UI/ConquestHUD.cpp`

Main menu settings:

```
MaxSessionSearchResults = 10000
PublicConnections = 8
bUseLanSessions = false
JoinSessionResultIndex = 0
```

For the current Steam packaged path, `bUseLanSessions` must stay `false`. It is exposed to Blueprint subclasses, so confirm any BP-derived main menu widget has not overridden it to `true`.

Hosting:

1. The host button calls:

```
UConquestMultiplayerSessionSubsystem::HostSession(PublicConnections, bUseLanSessions)
```

2. With `bUseLanSessions=false`, hosting requires a valid local Steam identity.
3. The subsystem creates `NAME_GameSession`.
4. The session is advertised with:

```
bIsLANMatch = false
bShouldAdvertise = true
bUsesPresence = true
bUseLobbiesIfAvailable = true
bAllowJoinInProgress = true
bAllowInvites = true
bAllowJoinViaPresence = true
bAllowJoinViaPresenceFriendsOnly = false
CONQUEST_PRESENCE = Conquest
SETTING_MAPNAME = current map name
SETTING_GAMEMODE = Conquest
```

5. On successful session creation, the subsystem starts the session.
6. The main menu then opens the current level as a listen server with:

```
?listen?ConquestHostSetup=1
```

Joining from search:

1. The join button calls:

```
UConquestMultiplayerSessionSubsystem::FindSessions(MaxSessionSearchResults, bUseLanSessions)
```

2. With `bUseLanSessions=false`, searching requires a valid local Steam identity.
3. Steam searches use lobby/presence query flags where those macros are available.
4. The first search tries the Conquest service filter:

```
CONQUEST_PRESENCE = Conquest
```

5. If the filtered Steam lobby search returns zero Conquest results, the subsystem retries with a broader lobby search and filters locally.
6. The main menu automatically joins `JoinSessionResultIndex`, currently `0`.
7. On successful join, the subsystem resolves the session connect string and calls:

```
ClientTravel(ResolvedTravelURL + "?ConquestJoinSetup=1", TRAVEL_Absolute)
```

Steam invites:

- `SendSessionInviteToFriend(FriendUniqueNetId)` is exposed on the session subsystem.
- Sending an invite requires a valid local Steam identity.
- Accepted Steam session invites call `JoinSession` and then use the same auto-travel path as normal joins.
- Received invites are currently logged.

Post-travel UI:

- `AConquestHUD::BeginPlay()` checks the world URL.
- If the URL has `ConquestHostSetup` or `ConquestJoinSetup`, the HUD shows the game setup screen.
- Otherwise, it shows the main menu.


Packaged Steam Test Requirements
--------------------------------
For a packaged Steam networked build using the current setup:

1. Package a Windows build.
2. Ensure Steam is running on every test machine.
3. Use a different Steam account on each machine.
4. Add `steam_appid.txt` for App ID 480 testing.
5. Launch the packaged executable directly.
6. Host from one machine.
7. Join/search from the other machine.

`steam_appid.txt` must contain only:

```
480
```

Place `steam_appid.txt` beside the executable being launched. For Unreal packaged Windows builds, the real game executable is commonly under:

```
<PackageRoot>\Conquest\Binaries\Win64\
```

If launching through a root-level packaged executable/stub, placing a copy beside both the launched root executable and the `Binaries\Win64` game executable avoids ambiguity during App ID 480 testing.

For real Steam release builds, replace App ID 480 with the project's assigned Steam App ID and remove any temporary local App ID workflow that is no longer appropriate for distribution.


Expected Healthy Logs
---------------------
Useful log lines from the current code path include:

```
Conquest sessions: creating online session through OnlineSubsystem=Steam
Conquest sessions: create session GameSession completed with success=true
Conquest sessions: searching online sessions through OnlineSubsystem=Steam
Conquest sessions: find sessions returned <raw> raw results and <filtered> Conquest results
```

The Steam packaged path should not report that the active online subsystem is `NULL` or `None`.


Common Failure Cases
--------------------
Immediate "Could not host game":

- The subsystem could not get a valid session interface.
- Steam OSS did not initialize.
- The local Steam identity is not ready.
- Check for this project log:

```
Conquest sessions: cannot create Steam session because local Steam identity is not ready.
```

Search returns "0 found":

- Both clients may not be using the same subsystem/session mode.
- One client may have `bUseLanSessions=true` from a Blueprint override.
- Steam may not be running or logged in.
- Both clients may be using the same Steam account.
- The host may not have created an advertised Steam lobby/session.
- The first filtered search can return zero; the code retries broad search automatically, then filters locally for `CONQUEST_PRESENCE=Conquest`.

Join succeeds but client does not travel:

- The resolved connect string may be empty.
- The active net driver may not match the session transport.
- For current Steam packaged testing, `SteamSockets.SteamSocketsNetDriver` should be the active `GameNetDriver`.

Host opens the setup screen but clients cannot connect:

- Confirm only one active `GameNetDriver` exists after `!NetDriverDefinitions=ClearArray`.
- Confirm SteamSockets is enabled in `Conquest.uproject`.
- Confirm both machines are using the same packaged build/config.
- Confirm `steam_appid.txt` is in the correct packaged executable directory for App ID 480 tests.


Local Null/LAN Diagnostic Mode
------------------------------
The project is not currently configured this way, but the config keeps a local/null path available for diagnostic standalone testing.

To use Null/LAN mode manually:

1. Set the default subsystem to Null:

```
[OnlineSubsystem]
DefaultPlatformService=Null
;DefaultPlatformService=Steam
```

2. Keep Null enabled:

```
[OnlineSubsystemNull]
bEnabled=True
```

3. Switch the active net driver to `IpNetDriver`:

```
[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
```

4. Set the main menu session mode to LAN:

```
bUseLanSessions = true
```

In C++ the default is currently `false`, and because it is `EditAnywhere, BlueprintReadWrite`, Blueprint subclasses can override it.

When returning to packaged Steam testing, set `DefaultPlatformService=Steam`, restore `SteamSockets.SteamSocketsNetDriver`, and set `bUseLanSessions=false`.


Optional Legacy SteamNetDriver Path
-----------------------------------
The config still contains the older non-SteamSockets Steam net driver line:

```
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
```

Only use this deliberately when testing the older Steam net driver path. For the current intended packaged build, keep it commented and use:

```
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
```

