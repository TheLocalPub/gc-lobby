# Group description checks

Open this separate test addon, then the production Arland example world. Reload,
start embedded Play and advance Preview to Lobby using an authenticated admin.
Invoke `GC_TestGroupDescription(description, clickHeader)` through MCP on
`GC_MissionTemplate` / `GC_LobbyManager`. The helper changes only the first visible
group's runtime description, refreshes the production card, and optionally
dispatches its production header click handler. Stop Play to discard the fixture.
Do not mount this script in the profile: its group extension must load after
GC Lobby through the addon dependency.

2026-11-15, Workbench 1.9.0.37, embedded Arland:

- Reload `9b285aab11494e549cb5c496834f478f` passed compilation. Existing stale
  ArlandLobby resource references and a PacejkaPlotTool leak remained in diagnostics.
- `38a5c7861f3c4b6aa2d9d7b8fb088dff`: padded "Weapons team" trimmed, visible,
  callsign unchanged, expanded. Actual screenshot confirmed smaller gray text
  immediately below the callsign.
- `8c33e302cf0543799418c9aef0d1253f`: whitespace hidden; header click collapsed slots.
- `596e35721ef54833b20237daf6a38d4b`: long description visible; second click expanded
  slots. Actual screenshot confirmed wrapping clear of the player counter.
- `c5834906c35d4dc1b1690000157a0ba0`: empty description hidden after a populated value.
- Production `TryAdvanceLobbyPhase` accepted the authenticated local admin and
  emitted its normal audit entry. No permissions bypass or world save was used.

Captures: external development workspace
`reforger-script-tools/artifacts/group-description/{short,long,empty}.png`.
Remote peers/JIP, additional viewport/UI scales, controller input, and saved
authoring round-trip were not exercised. Runtime mutation here is test-only;
production descriptions are authored on prefab/world resources, not network setters.

Independent code and screenshot review approved. Restored the production-only
GC Lobby project; `logs_2026-11-15_07-07-26/console.log` lines 23037-23063 confirm
successful Game compilation and creation without the test addon. The loaded
project snapshot contained only core, ArmaReforger and Lobby2.
