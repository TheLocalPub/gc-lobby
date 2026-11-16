# GC Workshop

Metadata-only collection for loading and downloading the complete GC addon set.
Open `addon.gproj` in Workbench, with the Core, Units, Missions and Lobby Dev
checkouts available. There are no scripts, overrides, bundled third-party assets,
or MCP requirements in this addon.

The development identity is `GlobalConflictsWorkshopDev`, GUID
`6B8E05204D634FA9`. This preserves the earlier Workbench collection prototype's
unpublished identity. It is a Dev collection, not a published release: its GC
dependencies use Dev identities. Do not substitute release GUIDs piecemeal or
publish it as a release collection until those matching packages are available.

## Dependency ownership

- **Core:** shared systems and mod-pack behavior. ACE is selected once through
  **ACE All in One Dev**,
  including ACE Core Dev and ACE Medical Core Dev; CSI supports the compass.
- **Units:** units, loadouts, weapons, ammunition, vehicles and their overrides.
  It also selects the shared gameplay baseline, including suppression and mortars,
  so those systems do not depend on whether a mission places a particular object.
- **Missions:** worlds, headers, terrain requirements and mission-specific data.
- **GC Workshop:** the full collection, including optional content. Nothing in
  Core, Units, Missions or Lobby depends on this collection.

The six direct entries are the base game, the four GC Dev projects and optional
Map Drawing. Native transitive dependencies supply the rest, keeping the equipment
and terrain lists with their owners. The collection retains the previous selected mod set; it does not enable every
addon installed on the author's machine. Map Drawing stays a collection option,
not a Core or Lobby requirement. Existing TilW-based crew/mission resources still
require TilW on Units/Missions; Lobby and Core do not require that framework.

## Downloading and updating

Publish this project separately through Workbench to create its Workshop listing.
In Reforger, use the collection's native **Download dependencies**, **Update
dependencies**, and **Enable dependencies** actions as offered by the Workshop.
Updating the collection package alone is not a promise that every dependency has
been updated; the native dependency update action handles that separately.
Loading it activates its dependency set; loading an addon is not a background
updater. Republish the collection when its selected dependencies change.

Do not use this full collection as a minimal mission/server dependency list.
Use the mission's actual addon requirements instead. Reforger's package dependency
graph is package-wide: GC Units currently loads the shared unit library, not just
the equipment placed in one scenario.

Keep this project outside GC Lobby's Workshop package. The migration receipt and
validation are in `../dependency-migration`; they are development evidence only.
