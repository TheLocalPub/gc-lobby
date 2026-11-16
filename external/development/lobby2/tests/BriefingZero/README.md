# Zero-index briefing checks

Temporarily mount or copy `scripts/Game` into the Workbench profile's
`scripts/Game/BriefingZeroChecks`; reload, then start the Arland preview with
an authored index-0 briefing. Invoke these methods through MCP on
`GC_MissionTemplate` / `GC_LobbyManager`:

- `GC_TestBriefingZeroCatalog()` checks zero lookup, in-place update,
  replicated update application and removal using an isolated production catalog.
- `GC_TestBriefingZero("click")` dispatches the production row click invoker.
- `GC_TestBriefingZero("refresh")` refreshes the production view.
- Any other string reads the current selection, panel, title and body.

Remove the temporary profile copy and reload after testing. This helper is
excluded from Workshop content with the rest of `external`.

2026-11-16, Workbench 1.9.0.37: compilation passed. Catalog operation
`e2706915e00b454a95ae72c0f740e777` passed. Selection and refresh operations
`848a2c0593ca472687b427ca92a0a201` and `44889c9a26c147a1a4760e6786c7df3c`
returned index/display 0, selected/document true, title `00  BRIEFING`, and
the authored body. Close `b9c96872ef4d49cd9f832a1d073ea2f8` hid the panel;
reopen `11173bcfb0c1408992989a7b67ac5677` restored it. MCP screenshot at
13:12:36 UTC showed the selected 00 row and its body in the document panel.

Remote peers, JIP transport, multiple authored indices, rich text and other
viewport sizes were not exercised in this pass. Existing unrelated project
resource and mission-event errors remain.
