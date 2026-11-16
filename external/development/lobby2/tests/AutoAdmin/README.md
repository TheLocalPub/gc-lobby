# Automatic admin checks

Production entry point: `GC_AutoAdmin.OnPlayerAuditSuccess`, called by the GC
Lobby manager after the engine's successful identity audit. Configuration now
uses the native `game.admins` list in the main server config, queried through
`BackendApi.IsListedServerAdmin`. The former custom JSON parser and its tests
were removed along with the separate-file contract.

Temporarily mount this fixture's `scripts/Game` under the Workbench profile
`scripts/Game`, reload, start a saved GC Lobby world, and explicitly call
`GC_LobbyManager.GC_TestAutoAdmin` through MCP. Remove the fixture mount and reload
after testing. Do not include it in the production addon.

Installed API evidence: catalogue revision `366e99137dac49b7a1fde7c094cd92b9`,
`game/core/data.pak/scripts/GameLib/generated/online/BackendApi.c:223` declares
`IsListedServerAdmin(int)`. Bohemia's server config documentation defines
`game.admins` and states that this query is independent of logged-in state.
Only the join bootstrap uses it; admin actions continue checking current native
roles. Native membership supports identity IDs and Steam IDs with a 20-ID limit.

Coverage still required on a backend-authenticated dedicated server: listed
player joins and receives native admin/UI access, unlisted player stays without
admin, logout rejects subsequent admin actions without mutation, reconnect
promotes again, and audit output reflects results. Embedded guard checks do not
prove that multiplayer path or native role propagation timing.

Validation on 2026-11-16, Workbench 1.9.0.37: reload operation
`664639c2cb3a4ba7906ba44b86c19792` reported zero errors. Live MCP operation
`ee9340c1c8a34fa4a19ba40f7086f5c1` returned **2 guard checks, zero failures**
in the existing `GCRhsLoadOnlyTest/Worlds/RhsLoadOnly.ent` world. Incorrect owner
and invalid player ID did not change the local player's native roles.
