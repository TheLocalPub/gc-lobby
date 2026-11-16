# Loadout requirements verification

The existing loadout save dialog displays requirements for the selected saved prefab. The root view includes referenced variants; selecting a variant shows its concrete equipment. Unsaved gear and variant edits have explicit pending states. Saving writes `<loadout>.et.dependencies.json` beside the Units Dev prefab and keeps the dialog open.

## Discovery contract

The loadout view opts into a restricted policy on the existing bounded mission resource collector. It recursively follows native inventory/loadout slots (`Prefab`), weapon slots (`WeaponTemplate`), muzzle magazines (`MagazineTemplate`), initial inventory (`PrefabsToSpawn`), visual identity (`Head`, `Body`), variant references, and magazine ammunition (`AmmoConfig`, `AmmoResourceArray`). Native subclasses inherit slot coverage. Temporary hand/saline/tourniquet storage is excluded, matching the loadout writer.

Effective container values include overrides. Explicit ancestor prefab ownership is retained, but ancestor equipment is not rescanned: a child replacing an RHS weapon with a vanilla weapon can clear RHS. Project dependency lists and unrelated loaded addons never populate requirements.

ResourceDatabase enumeration is keyed by resource GUID, not path capitalization. Vanilla definitions take precedence over global overrides; unique third-party definitions supply owner IDs. Multiple third-party definitions are ambiguous and produce partial evidence, not a guessed requirement. Loaded addons only supply GUID/title lookup. Newly authored Units resources use the writer's verified writable-meta check while the Play-time database is paused.

This is native loadout reference evidence, not a complete dependency solver. Custom script-defined resource fields, dynamically spawned items, model/texture-only changes, or ambiguous non-vanilla overrides need separate coverage. JSON explicitly records partial coverage, unresolved resources, field origins, IDs/GUIDs and reasons. It is authoring metadata; this change does not connect the sidecar to the dedicated launch resolver.

## Live test (2026-11-16)

Workbench 49496, GC Workshop with temporary GCLoadoutDependencyTests addon; Arland_Lobby embedded Play. Native MCP Reload compiled the changed scripts without compilation errors. Existing project resource/class warnings and BlockListApi teardown leak remain; this is not a globally clean mod pack.

- Spawned a no-RHS US rifleman, saved NoRHSFinal through the production Save button event: dialog stayed open, sidecar persisted, RHS and WCS Sounds absent.
- Spawned a fixture inheriting that character with an RHS M4A1, saved RHSFinal: RHS present in UI and sidecar, including nested magazine, weapon parts, attachments and ammunition config/projectiles.
- WithRHSSaved replaces its inherited RHS weapon with an M16: no RHS requirement after scan.
- Closed/reopened the menu and selected RHSFinal: RHS displayed again. Current gear shows Unsaved gear.
- Missing resource scan produces unresolved evidence rather than Base game only.

Evidence: rhs.json, no-rhs.json, rhs-save.png, no-rhs-save.png, receipts.json. The concept is illustrative; PNG save captures are actual MCP screenshots. Tests invoke native UI component events via a temporary test addon, not physical mouse/controller input. Broad viewport scaling, overflow scrolling, translations and arbitrary third-party custom fields were not tested. Variant results are recorded separately.

## Reproduction

Use the normal GC Workshop project, with Units Dev writable. Temporarily add test addon GUID 7E161126B1000100 through an addon search root containing this external directory. Keep it out of Workshop releases. Start embedded Play and call GC_TestLoadout on GC_MissionTemplate/SCR_GameModeHealthSettings through MCP. `spawn` accepts a character resource (empty means native US rifleman); `open`, `select`, `name`, `Save`, `state`, `variant`, and `variantPrefab` dispatch the existing authoring flow. Save destinations are restricted to US/DependencyTest. `scan`, `summary`, `search` and `ancestry` are diagnostic-only. Remove generated Units test files and temporary project dependency after testing.

Variant check: selected original variant (no RHS), changed it to RHSFinal (Unsaved variants), saved (RHS shown and persisted), changed it to WithoutRHS and saved again (RHS removed). See variant-*.json and variant-pending/variant-saved.png. The no-rhs.json snapshot predates these intentional variant edits.
