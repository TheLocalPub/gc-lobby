# Unconscious weapon retention

Production option: `GC_LobbyGameMode.m_bKeepWeaponWhenUnconscious`, default true,
under **GC Lobby Gameplay** on the game-mode entity. Configure before starting
the mission; this is not a runtime admin setting. No third-party dependency is
required by Lobby.

The synchronous consciousness hook always calls the inherited method. It then
extends vanilla AI's contextual stowing to non-AI characters only when the active
GC Lobby mode enables it and the character is incapacitated. Recovery and death
do not run the added behavior. Active AI remains owned by the inherited method.
Triggered/throwing grenades use the native drop-live-grenade path; a missing
handler never turns a live grenade into a safely stowed item. There is no queued
callback, cached mode policy or persistent character state to leak across modes.

## Native research

Installed game/Workbench 1.9.0.37, PAK revision
`c52648a62fcb4229ad089610d060ea44`:

- `game/data/data007.pak/scripts/Game/Character/SCR_CharacterControllerComponent.c:127–163`
  implements the AI-only consciousness policy used as the base-game reference.
- `game/data/data007.pak/scripts/Game/generated/Components/CharacterControllerComponent.c:341,504`
  exposes native contextual equip and the synchronous consciousness event.
- `game/data/data007.pak/scripts/Game/Building/SCR_CampaignBuildingGadgetToolComponent.c:226,260–280`
  demonstrates life-state notifications. These notifications expose no drop veto;
  they are not the synchronous consciousness callback used by vanilla stowing.
- `addons/NoWeaponDropUnconscious_60612C225328522E/data.pak/scripts/Game/Character/Maeh_CharacterControllerComponent.c:1–40`
  was inspected as the requested behavior reference. It replaces the vanilla
  callback without its active-AI restriction. GC's implementation instead keeps
  inherited behavior and gates its extension on the GC mode and option.

An addon-owned character subclass would require replacing every mission-selected
character prefab, including third-party units. The narrow shared callback avoids
that dependency and coverage problem. No polling or world scan is introduced.

## Fixture

Mount this directory's `scripts/Game` temporarily at the Workbench profile's
`scripts/Game/zzzWeaponRetentionChecks`, and mount `lobby-support/Game` at
`scripts/Game/zzzzRetentionPolicyChecks` when GC Lobby is loaded. Reload and
start the saved Arland Lobby example. The fixture is excluded from the Workshop release and production
code never references it. Invoke `SCR_GameModeHealthSettings.GC_TestWeaponRetention(stage,
enabled)` on `GC_MissionTemplate` through MCP. For the stock Arland Game Master
world, use entity `GameMode_GameMaster_Full`. The common fixture also works
without GC Lobby: remove the policy-support junction before opening the base
game project. The common fixture logs the mode name at initialization and gives
an otherwise unnamed mode a temporary name only during Play.

Run stages 0 (spawn temporary character and take local control), 1 (equip),
2 (native damage-manager unconsciousness), 3 (observe), and 4 (restore original
body/setting and delete fixture). Allow simulation/animation between calls and
inspect every result. Run independently with enabled=true and enabled=false.
Use only an otherwise idle embedded session; this temporarily changes the local
controlled character. Stop Play afterward, remove only the two fixture junctions,
and Reload production scripts.

## Recorded validation (2026-11-15)

Workbench 1.9.0.37, native M16A2 and a locally controlled US rifleman, using the
real damage manager's `ForceUnconsciousness`, with AI inactive:

| Context | Setting | Unconscious | Rifle retained | Current weapon |
| --- | --- | --- | --- | --- |
| GC Lobby only | Enabled | Yes | Yes | None (stowed) |
| GC Lobby only | Disabled | Yes | No (dropped) | None |
| Full GC Missions Dev pack, Maeh addon absent | Enabled | Yes | Yes | None (stowed) |
| Full GC Missions Dev pack, Maeh addon absent | Disabled | Yes | No | None |
| Stock Arland Game Master, GC Lobby loaded | Inactive outside GC | Yes | No | None |
| Stock Arland Game Master, only the two base-game addons loaded | No GC policy | Yes | No | None |

The common development fixture was present in both Game Master runs; production
GC Lobby was absent from the baseline. The two GC-only observations followed by
the two Game Master observations are:
`db82ca66d4d5456e897021faefb0b4a4`, `7afc260c69a94c84b32578e9d0b306ce`,
`8e8c8ec8b5e54f5998b466bceee60852`, `0d9d2d3c9fe14387a219437ac2d29323`.
Full-pack confirmation operations are `23bc4bcdc76c45f98731dcfdabe7a2c7`
(enabled) and `c7693d888db04a50bbffcc9996bfcfb4` (disabled).
Every fixture restored the original controlled entity and setting and deleted
its spawned character and dropped rifle. The weapon-equip request can return
false when the native character has already equipped the rifle; stage 2 checks
the actual held weapon before requesting unconsciousness.

The original Missions Dev project and Arland Lobby world were restored with
Play stopped. Both fixture mounts were moved out of `profile/scripts/Game`.
Final production-only Reload `f75c8912f29f497480094b6f9bd7fccc` completed without
compiler errors; pre-existing `PacejkaPlotTool`/`BlockListApi` leak diagnostics
remain in session `logs_2026-11-15_11-01-32`.

The Lobby-to-Game-Master editor-world change was exercised with script reloads
while updating the fixture; it is not an uninterrupted server mission-rotation
test. Live grenade throws, active AI, recovery/death sequences, remote clients
and dedicated-server replication remain untested. Their inherited paths and
the grenade branch were reviewed against the installed native scripts. Existing
resource GUID and editor script-instance leak diagnostics remain outside this
change; compilation has no new errors.
