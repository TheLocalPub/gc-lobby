# RHS load-only experiment

Passed on 2026-11-16 with a dedicated server and two separate local diagnostic clients launched by Workbench's stock Dedicated Server Tool. The server instantiated the placed RHS rifle, both clients resolved its prefab, both received player-controller ownership, and the server reported `Players connected: 2 / 2` and both `OnPlayerAuditSuccess` callbacks. Repeated samples retained two players for more than a minute. This fixture is development-only and excluded from Workshop releases with the rest of `external`.

## What was tested

`addon.gproj` declares only GC Lobby Dev (`6A115D5DFF7ED870`); GC Lobby declares only the base game (`58D0FB3206B6F859`). Neither declares RHS. The world nevertheless places the RHS M4A1 prefab `{5961913EE3FF9467}Prefabs/Weapons/Rifles/M4A1/Rifle_M4A1_BLOCK_0.et`. RHS is an explicit startup requirement for this mission, not an addon dependency of GC Lobby or the fixture.

Workbench was opened on this isolated project with `-addons RHS_Core`. Its loaded set contained exactly seven addons: core, base game, GC Lobby Dev, this fixture and the three RHS addons. No broad authoring collection was loaded. The production **GC → Create Mission Data → Refresh metadata only** action saved the world and updated [the native mission header](Configs/Missions/RhsLoadOnly.conf). Readback from disk showed:

1. The placed rifle's `m_aAddonIds` contains `RHS_Core`.
2. `m_aReferencedAddonIds` contains `RHS_Core`.
3. `m_aLoadedAddons` maps that name to GUID `595F2BF2F44836FB`.
4. Both RHS content packs are recorded as loaded but not directly referenced by a placed prefab. The installed RHS Core project declares them as dependencies, so they belong in the required closure.

| Addon ID | GUID | Selection reason |
|---|---|---|
| GCRhsLoadOnlyTest | 7E161126A01D0001 | Mission owner (not inferred from placed-prefab references) |
| GlobalConflictsLobbyDev | 6A115D5DFF7ED870 | Fixture dependency and placed content |
| RHS_Core | 595F2BF2F44836FB | Placed rifle source addon |
| RHS_Content_01 | 1337C0DE5DABBEEF | RHS Core dependency |
| RHS_Content_02 | BADC0DEDABBEDA5E | RHS Core dependency |

The stock dedicated tool forwards **all currently loaded non-vanilla addon IDs**, rather than reading our metadata. The observed startup selection on the server and both clients was:

```text
-addons GlobalConflictsLobbyDev,GCRhsLoadOnlyTest,RHS_Content_01,RHS_Content_02,RHS_Core
```

Thus this experiment verifies explicit load-only startup and the metadata association separately. It does not implement automatic metadata-to-server orchestration. A production selector must read the saved header, map referenced IDs through `m_aLoadedAddons`, add the mission owner and native dependency closure, and reject unresolved/incomplete inputs. Workshop server configuration identifies a mod by its addon GUID (`modId`); this diagnostic launch used addon names and already installed resources instead of a Workshop server JSON configuration. Do not confuse prefab GUID `5961913EE3FF9467` with mod GUID `595F2BF2F44836FB`.

## Reproduce

1. Install RHS Core and its two content packs. Open this `.gproj` with RHS Core explicitly loaded, and make the base game, installed addons, local addons and this nested fixture discoverable through addon directories. Do not add RHS to either project dependency list.
2. Open `Worlds/RhsLoadOnly.ent`. Refresh metadata with the production action above and inspect the saved header. Ordinary Ctrl+S does not refresh this metadata.
3. In Dedicated Server Tool, select `{7E161126A01D0002}Worlds/RhsLoadOnly.ent`, `ArmaReforgerServerDiag.exe`, and `ArmaReforgerSteamDiag.exe` for peers. Enable Run Peers and two separate peer entries with auto-join and separate profile suffixes. The tested tool settings used `-client`, reconnect and timeout-disable for local diagnostic clients.
4. Set `-gcRhsProbe <unique-run-id>` in server and both peer parameters. Use dedicated test profiles to separate logs. Append a quoted `-addonsDir "<comma-separated addon directories>"` to those parameters when the stock plugin's unquoted path list contains spaces; the test logs confirmed the final argument took effect.
5. Start through the tool's **Run** button. Verify server and both client logs, not merely running processes: seven expected addons, `rhsResource=1` on all three, `rhsEntity=1` on the server, two audit callbacks, two owned client controllers, and sustained `players=2`.
6. Stop through the same tool and verify all three owned processes exit. Restore previous settings and editing project.

The observer runs only with the explicit CLI flag, performs no spawning or ownership changes, samples at five-second intervals for three minutes, and removes its callback on deletion. Clients in this empty lobby are spectators near the origin. The distant rifle is absent from their name lookup (`rhsEntity=0`), so client entity visibility/streaming is **not** asserted. Each client explicitly resolves the RHS prefab (`rhsResource=1`), while the server checks the actual placed entity.

## Evidence and limits

[Process observations](evidence/processes.json) record server PID 43780 and client PIDs 50964 and 32124, exact startup arguments and separate profiles for run `rhs-local-02`. The server password is redacted. [Server](evidence/server.txt), [client 1](evidence/client1.txt) and [client 2](evidence/client2.txt) contain selected original console lines and line numbers. Full local logs remain in the recorded profile directories. The initial run `rhs-local-01` also connected both clients; it exposed the client name-lookup limitation, prompting the explicit resource-resolution check in the repeat run.

Workbench 1.9.0.37 required a versioned multiplayer native adapter repair in the separate Reforger MCP repository. Its callback tests passed for current and legacy profiles; real settings writes, nested peer settings, process observation, launch and stop were exercised. GC Lobby and this test addon do not depend on MCP at runtime.

The saved metadata explicitly excludes other subscenes, runtime spawns and expanded inventory/random variants. It is content evidence, not a complete dependency guarantee. The existing saved-source launcher remains fail-closed for unsupported packed/scripted resources; this test does not bypass that contract. No mission-transition pruning, remote players, clean-machine download, Workshop publication, production server JSON launch or rifle gameplay/visual test was performed. The clients logged an existing map icon mask error; Workbench reload logged a `PacejkaPlotTool` leaked-instance diagnostic. Neither prevented the observed joins, and neither is claimed fixed here.
