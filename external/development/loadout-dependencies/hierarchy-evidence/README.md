# Loadout tree UI

Active concept: concept-final.png; earlier concepts superseded by user feedback. No plus/minus glyphs, variant labels or variant filenames. Inherited saved prefabs form a native parent/child tree. Clicking selects and expands; clicking the selected prefab again collapses. Selecting a child keeps its ancestor path visible. Numeric variants appear only below the selected prefab. Parent information is inline in the list; local saved parent links navigate, external base parents remain read-only. No right-side relationship box.

Native BaseContainer ancestry is queried on list refresh, skipping repeated same-GUID global overrides. The maps only cache browser presentation; they do not own prefab relationships. Scope filters retain ancestors of matching descendants. Existing native row components and selection events are reused; widgets are not rebuilt merely to select/expand/collapse.

Live verification: Workbench 47504, GC Workshop + temporary test addon, Arland_Lobby, native Reload succeeded. Known existing pack resource errors and PacejkaPlotTool teardown leak remain. MCP component events exercised initial collapsed view, root/child expansion, ancestor preservation, numeric variant selection, parent navigation, repeated-click collapse, Current collapsing all, variant add/change/save with updated requirements, Save gear creating HierarchySaved, close/reopen. See receipts.json and reload.json. selected-root.png is the final expanded root; variant.png/final-tree.png show numeric variant selection. expanded.png predates adding the second test variant but uses final production UI.

Tests dispatch native widget-component events through the external test addon, not physical input. Controller navigation, a large real catalogue, deep malformed inheritance, and broad resolution/translation coverage remain untested. Test fixtures and temporary project dependency are removed after verification; unrelated user prefabs/world edits are preserved.

Review follow-up: final-reload.json has no SCRIPT errors. reviewed-tree.png shows final muted-yellow selection. cross-scope.json verifies a child in DependencyCrossTest remains reachable from its parent in DependencyTest, including parent navigation. Filters retain relevant families; expanding a retained family reveals children across authoring scopes. Earlier screenshots predate the reviewed selection colors.

Independent review approved after cross-scope and selection-color fixes. Final production changes preserve the preceding required-mod metadata implementation.
