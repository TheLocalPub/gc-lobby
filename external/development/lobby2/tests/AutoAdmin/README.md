# Automatic admin checks

Production entry point: `GC_AutoAdmin.OnPlayerAuditSuccess`, called from the
GC Lobby manager's engine audit-success callback. The server configuration is
`$profile:Lobby2Admins.json`, a JSON string array. No test config or real UIDs are
shipped.

Temporarily mount this fixture's `scripts/Game` under the Workbench profile
`scripts/Game`, reload, start a saved GC Lobby world, and explicitly call
`GC_LobbyManager.GC_TestAutoAdmin` through MCP. Remove the fixture mount and reload
after testing. Do not include it in the production addon.

2026-11-16, Workbench 1.9.0.37: reload
`649c7312ca8f43789371a16bcc45beab` reported zero errors. Runtime operation
`1aa45de6167b48f2a1cfd5c0ef77eec6` returned **15 checks, zero failures** in the
already-open `GCRhsLoadOnlyTest/Worlds/RhsLoadOnly.ent`. Covers empty/populated
arrays, whitespace, exact UID preservation, rejected non-arrays/malformed JSON
(including injected sibling and duplicate members),
and no role mutation for an incorrect owner or invalid player ID.

Installed source reference: catalogue revision
`366e99137dac49b7a1fde7c094cd92b9`, Server Admin Tools
`scripts/Game/ServerAdminTools_GamemodeComponent.c:162-211` uses the successful
identity audit and native role assignment. Its delayed two-role workaround is
not copied: native `PlayerManager.GivePlayerRole` permits authoritative role
assignment, and `SCR_Global.IsAdmin` accepts `ADMINISTRATOR` directly.

Coverage still required on a backend-authenticated dedicated server: listed
player joins and receives native admin/UI access, unlisted player stays without
admin, logout rejects subsequent admin actions without mutation, reconnect
promotes again, invalid/missing config grants nothing, and audit output reflects
the results. Embedded parser/guard checks do not prove that multiplayer path or
the timing of native role propagation.
