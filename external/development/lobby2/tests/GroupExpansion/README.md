# Group auto-expansion regression

Temporarily mount `scripts/Game` under the Workbench profile's `scripts/Game/GroupExpansionChecks`, reload, and start a disposable Arland Lobby session. Through MCP, call `GC_LobbyManager.GC_TestGroupExpansion(-1)` on `GC_MissionTemplate`, then call with an available slot ID (2 in the example). This uses the production self-selection handler and the actual local player. Read again after a frame. Stop Play, remove only the verified junction, and reload afterward.

2026-11-14, Workbench 1.9.0.37: initial groups=2; selection accepted with groups=2, native/controller membership matching, slot group matching, US group 1000 containing the player and USSR group 1001 empty. Both authored groups reported max members=1. User confirmed the extra empty group was fixed.

Evidence: MCP operations `38aa1d04f5084fb2a12ef1efc2b1870a`, `bde9d3d8aa914580a686c23ec4be31b3`. Before the fix, session `logs_2026-11-14_09-56-06` lines 23337/23340 registered the authored groups and line 23467 registered a third immediately on slot selection. Installed base source `game/data/data007.pak/scripts/Game/Groups/SCR_GroupsManagerComponent.c` lines 771-812 creates spare groups when full. Catalogue revision `c52648a62fcb4229ad089610d060ea44`.

Independent review approved. Remote clients, unrelated-mode/addon-free baseline, and mode transitions were not exercised. Inherited fallback was source-reviewed. Existing resource GUID warnings/errors and a PacejkaPlotTool reload leak are unrelated to this fix.
