# Dev mod conflict review — 2026-11-15

Reviewed GC Lobby, Core, Units and Missions together in Workbench 1.9.0.37.
Missions Dev is the root project; the installed Workshop directory is included.
Fresh session: `logs_2026-11-15_09-10-36`, process 46852.

## Verified

- All 143 expected addon identities mounted, including base-game data and all four
  local Dev checkouts. No missing dependency or cycle in the GC dependency graph.
- No duplicate resource GUIDs found across the four production source trees
  (Lobby's external development files excluded).
- Combined scripts compiled at startup and after Reload. Embedded Play reached
  the authoritative PREGAME lobby, registered both example groups and opened the
  preview menu. Play was stopped afterward; no world was saved by the review.
- The existing standalone launcher tests passed, including the legacy `Lobby2`
  argument/environment contract and stale-cache fallback.
- All four upstreams were fetched. No repository was behind or diverged at review
  start. Core, Units and Missions were clean and already pushed. Lobby's saved
  identity/example edits and the compatibility fixes are included in this sync.

## Corrected

The saved Lobby addon rename changed the mount to `GlobalConflictsLobbyDev` while
Workbench launchers still addressed `$Lobby2:`. Updated GC Tools, external setup,
and mod-workflow project discovery to the new mount. Kept the old tool argument
keys and profile file names so existing installations/data remain compatible.
Updated the example mission's cached addon metadata and README to match.

## Remaining findings

1. **Units resource registration:** startup reports six unregistered `.et` paths
   (three US 7th ID rifleman-AT variants, an Afghan sniper `.et.et`, a San Serriffe
   MG section and an AFRF 2001 CHQ). These need a content/metadata repair before
   being relied upon. No replacement resource identities were guessed.
2. **Third-party duplicate GUIDs:** seven warnings inside TF Weapons/Core,
   British Forces and ACE Overheating. These are internal to those dependencies,
   not GUID collisions between the four GC source trees. The selected resource
   can depend on database/load order; those addon authors need to reconcile them.
3. **Startup resource diagnostics:** 71 orphan metadata files, one unreadable
   metadata file, 52 localization GUID/name errors and one Steam statistics error.
   Units' `Images/donovia_flag.edds.meta` is among the orphan files. Successful
   compilation does not establish that every model, texture or string is usable.
4. **Unregistered-file noise:** of 23,120 startup registration warnings, 22,856
   concern the Workbench profile and 46 concern Lobby external development/build
   files. These counts are not 23,120 gameplay conflicts; packaging excludes
   external development content. The six Units prefabs above are distinct.
5. **Example-world authoring:** the saved Arland example contains an event with an
   empty area condition and no actions. Production validation skips it with an
   error. The saved author edits were preserved, not assigned invented behavior.
6. **Integration assumptions:** local map recovery reports that the optional GC
   Map Drawing Plans companion is absent. Core loads Map Drawing itself, which
   does not provide the GC companion. Bacon's arsenal integration also reports
   missing loadout managers/templates in this example world. These require the
   appropriate optional integration/world configuration before those features
   can be considered operational.
7. **Workbench/editor diagnostics:** reopening the editor referenced the obsolete
   `{CA75436A3C860FB9}Worlds/ArlandLobby/ArlandLobby.ent` world. That reference was
   not found in the four GC source trees. Opening the current example GUID
   succeeded. Reload also reported a `PacejkaPlotTool` instance leak; no GC script
   compilation failure accompanied it.
8. **Existing Units content limitation:** the migration review separately found
   49 legacy US desert/DBDU characters lacking role info through unresolved parent
   resources. This review does not claim to have repaired those parents.
9. **Vehicle/content compatibility:** Play's catalog loading reports unsupported
   vehicle fields including `Pacejka`, clutch settings and wheel/solver settings
   in M923A1 artillery ammunition, Motorbike, M113 and TIGR resources; it also
   reports unknown `MagazineWellSPC_PKMTBMP3`. These are concrete installed-content
   compatibility diagnostics on 1.9, not proof those vehicles function correctly.
   They need targeted equipment tests and dependency updates before mission use.
10. **Units editor icon:** `Configs/Core/EditableEntityCore.conf:46` references
    `{8B17988E290DC6CC}Images/GC_Spt_icon.edds`, which fails GUID/name resolution
    during Play. Restore the intended image/resource identity rather than choosing
    a different icon silently.

## Coverage limits

This was a dependency, source, startup and local lobby smoke review. It was not a
full multiplayer, voice-routing, equipment-spawn or unrelated-mode regression
test. Core retains global graphics settings, voice-display and AI LOD policies;
successful compilation alone does not prove all combinations of those behaviors.
The full mod pack includes TilW through Core's existing dependency; GC Lobby's
own project still depends only on base-game data.
