# GC dependency ownership migration

2026-11-16. Changes target the existing Core, Units and Missions **dev** branches.
GC Workshop is a separate metadata-only development collection.

Core selects the official **ACE All in One Dev** collection (`65AD7BE594439A52`)
and CSI alongside the base game. The umbrella includes ACE Core Dev and ACE
Medical Core Dev (`6586079789278413`), including the ACE features Core modifies.
This follows the user's simplification request after the initial split: ACE's
own collection maintains its full Dev module list, and Units has no duplicate
individual ACE entries. Unit/vehicle/weapon overrides, models, sights, effects and
ballistic tables belong to Units. Worlds, headers and terrain-specific AI/map
resources belong to Missions. The generic AI settings, spawner, map-area helper,
UI and behavior settings remain Core-owned.

`moves.json` records each moved file, its original hash and destination. Resource
GUIDs and paths are preserved. Three overlapping prefab overlays were combined:
T-72A keeps Core's passenger/damage/action changes and Units' material/inventory
changes; PG9/OG9 keep both the original physics/suppression and the Units tracer
changes. T-72A metadata retains the additional Xbox configuration. All other moves
preserve bytes. No third-party addon files are changed.

The previous Core dependency list is preserved as evidence in
`previous-core-dependencies.json`. The full Workshop collection retains that
selection; removed Core dependencies are owned by Units, Missions, or the collection.
The existing TilW crew configurations in Units and legacy mission resources are
declared honestly as dependencies; this migration does not convert that content
to the GC Lobby framework. Missions has only its existing/relocated mission
compatibility overrides, not vehicle or weapon overrides.

Run `python validate.py` from this directory (or pass `--addons-dir` for a different
checkout layout). It verifies move integrity, the intended package boundaries,
dependency availability/cycles and preservation of the full collection. It is a
development check, not a runtime updater or an exhaustive resource compatibility
test. Existing unresolved old equipment references are not repaired by this move.

## Initial split validation (before ACE consolidation)

- The integrity/graph check passes all 172 files, duplicate/cycle checks, the
  Core boundary checks and preservation of the previous full collection.
- All 20 mission-launcher prototype tests pass, including collection rejection.
- A fresh Core-only Workbench 1.9.0.37 startup loaded nine addons: engine core,
  Arma Reforger, GC Core and its six required systems. Compilation completed
  without script compiler errors (`logs_2026-11-16_08-42-54`). Core was tested
  without Units, Missions, Lobby, TilW, drones or the terrain/equipment packs.
- A subsequent Core-only world-open operation crashed Workbench; cause is
  unresolved. Startup/compilation is verified, but standalone Core world play
  is not certified by this test. The session was restored with the full collection.
- Fresh GC Workshop startup loaded 139 addons including engine core and all five
  GC Dev projects. The Arland Lobby world reopened, Reload completed and Play
  rendered the lobby preview with map markers and shapes at 13:48 UTC on
  2026-11-16 (`logs_2026-11-16_08-45-33`). Existing Pronto/TF/resource errors and
  a tool leak remain. Comparing normalized error messages against the earlier
  `logs_2026-11-16_08-11-35` session found no new distinct error message.
- The collection's redundant direct entries were reduced to its four GC projects,
  base game and Map Drawing after the initial load test. The graph check confirms
  the same full dependency closure; the content requirements remain with their
  owning packages.
- Independent review approved the split after the paper-map and vehicle-base
  overlays were also moved to Units. Individual vehicles, multiplayer peers,
  consoles, Workshop publication and network download/update were not exercised.

The static counts (Core 8, Units 119, Missions 136, Workshop 138) include the
selected addon and Arma Reforger; the native counts also include engine core.

## ACE consolidation

The final Core dependency list is base game + **ACE All in One Dev**
(`65AD7BE594439A52`) + CSI. The official downloaded `dev.gproj` explicitly includes
ACE Core Dev (`65AD7D0D9941A380`) and ACE Medical Core Dev (`6586079789278413`),
alongside the other Dev modules. Individual ACE entries were removed from Units.
The official collection is the authority for its evolving module list.

Source: https://reforger.armaplatform.com/workshop/65AD7BE594439A52-ACEAllinOneDev
and its native project declaration downloaded on 2026-11-16. Seven missing official
packages were installed: the umbrella, Backblast, Ballistics, Medical Breathing,
Medical Hitzones, Radio and Scopes. Existing installed ACE packages were retained;
no third-party source files were edited. The temporary loopback download server was
stopped after native downloads completed.

The final graph validator passes: Core 27, Units 126, Missions 143, Workshop 145
including self and Arma Reforger (native counts also include engine core).
It explicitly checks that ACE Core Dev and Medical Core Dev are in Core's closure.

A second pre-consolidation Core-only check successfully opened stock Game Master
Arland and entered Play, so the earlier native world-open crash did not recur.
That check exposed an existing Core voting-template exception for EDITOR_IN in
stock Game Master, plus stale ACE settings/resource warnings; it is not a clean
stock-mode compatibility certification. This dependency migration does not repair
those existing behavior/configuration problems.

Final fresh GC Workshop startup loaded 146 addons including engine core, the ACE
umbrella and all 23 ACE Dev modules (`logs_2026-11-16_08-59-33`). Game compilation
completed without script compiler errors. Arland Lobby opened and entered Play;
a live capture at 14:06 UTC showed its lobby map, markers and shapes.
Startup logged one unknown `SCR_ChimeraCharacter` class while loading
`Character_Base.et` immediately before the Game module finished loading (lines
24453–24461). Its originating addon is unresolved; successful subsequent Play
does not establish that this resource warning is harmless. Existing scenario
event and optional map-recovery messages also remain. This validates dependency
resolution and lobby startup, not every ACE feature or multiplayer behavior.
