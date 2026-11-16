# AI-world discovery checks

These optional Workbench checks are outside the release tree. Production GC Lobby
does not reference them. To run explicitly through MCP, temporarily mount the
`scripts/WorkbenchGame` test source in the Workbench profile, reload, and invoke
the checks from Plugins > GC Development. Remove the profile copy and reload
afterward; the development menu must not remain installed.

## Evidence, 2026-11-16

Workbench 1.9.0.37 with GC Workshop and Belleau Wood loaded. Log session:
`logs_2026-11-16_16-58-02/console.log` under the Workbench profile's sibling logs
directory.

- 17:03:07: production discovery selected
  `{CFDC9C151CAB3F8D}Prefabs/AI/SCR_AIWorld_BelleauWood.et`, one candidate, 119 ms.
  Non-AI rejection, duplicate suppression and renamed resource loading passed.
- 17:13:35: a nonexistent terrain name exercised the broad fallback: 13 eligible
  AI-world prefabs, stable sorted selection of Arland, 19.759 seconds. Notice
  identified the count and lack of terrain-name matches. Progress dialog shown.
  Loading unrelated modpack resources emitted existing resource/class errors;
  this does not establish that every prefab in the modpack is valid.
- Canceling native New World returned without placement.
- 17:12:49 and 17:15:38: placement selected Belleau Wood's AI prefab and reused one
  existing perception manager, setting the bundled child's native DISABLED flag.
- 17:12:22: native subscene creation changed the terrain path from a mounted path
  to a relative path. This exposed the continuation comparison bug; both paths
  now pass through the same normalization.
- Final reload starting at 17:17:43 completed with no compiler errors in the
  changed scripts (the pre-existing PacejkaPlotTool leak was logged).
- 17:20:26, operation `786f5edf4d234492a06efda8bb333718`: from the clean Belleau
  base world, invoked the production command once, selected native Sub-scene,
  and confirmed. The same invocation continued and placed the root plus Belleau
  AI world, reusing one existing perception manager. No second invocation was
  needed. See `selection-notice.png` for the actual selection notice.
- After removing the profile test copy and reloading, live Plugins enumeration
  retained GC and contained no GC Development category. The new scene remains
  unsaved for the author; no mission was silently saved.

No game/peer test or AI navigation traversal was performed. Automatic selection
still requires the author's navigation-mesh check. Known terrain mappings are
retained. The production prefab keeps its existing resource GUID.
