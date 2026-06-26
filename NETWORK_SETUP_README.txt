Conquest Network Setup
======================

Use one of these two setups at a time.


Steam Setup Settings
--------------------

Use this for packaged Steam multiplayer testing.

In `Config/DefaultEngine.ini`:

```
[OnlineSubsystem]
;DefaultPlatformService=Null
DefaultPlatformService=Steam

[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
```

In `Source/Conquest/UI/ConquestMainMenuWidget.h`:

```
// Local standalone/listen-server testing:
// bool bUseLanSessions = true;
// Packaged Steam multiplayer testing:
bool bUseLanSessions = false;
```

In `Source/Conquest/Core/ConquestMultiplayerSessionSubsystem.cpp`, use the Steam-only session advertisement:

```
SessionSettings.Set(ConquestPresenceKey, ConquestPresenceValue, EOnlineDataAdvertisementType::ViaOnlineService);
```


LAN Setup Settings
------------------

Use this for local standalone/listen-server LAN testing with OnlineSubsystemNull.

In `Config/DefaultEngine.ini`:

```
[OnlineSubsystem]
DefaultPlatformService=Null
;DefaultPlatformService=Steam

[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
;+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
```

In `Source/Conquest/UI/ConquestMainMenuWidget.h`:

```
// Local standalone/listen-server testing:
bool bUseLanSessions = true;
// Packaged Steam multiplayer testing:
// bool bUseLanSessions = false;
```

In `Source/Conquest/Core/ConquestMultiplayerSessionSubsystem.cpp`, use ping advertisement so LAN search filtering can read the Conquest key through OnlineSubsystemNull:

```
SessionSettings.Set(ConquestPresenceKey, ConquestPresenceValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
```
