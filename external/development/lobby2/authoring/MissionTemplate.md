# Workbench GC Lobby game mode

Open a terrain or an inherited mission and choose **Plugins > GC > Spawn GC Lobby
Game Mode**. On the base terrain this opens Workbench's native **New World**
dialog. Select **Sub-scene (of current world)** and confirm; placement then
continues in the new subscene. Cancel leaves the terrain unchanged. Choosing
**Base scene** performs the native operation but does not place the game mode.
In an existing mission, select an unlocked mission layer first.

Workbench lists this command under Plugins; Tools contains interactive tools
such as Map / AO Polygon.

The plugin places the draggable `Prefabs/MP/Modes/GC_LobbyGameMode.et` at the world origin and creates the
terrain's AI-world prefab as its child. The root inherits `GameMode_Lobby.et` and
contains the faction, radio, GC chat, and perception entities. GC Lobby's mission
framework has no TilW Mission Framework dependency or integration.
The terrain-specific AI world is a placed mission entity, not a runtime spawn.
Save the world normally to persist it; one Undo removes the complete entity
placement. Native subscene creation is a separate operation, outside that Undo.

The initial scaffold has US/USSR factions. Authors still configure playable
groups, briefing content, objectives, setup zones and the separate mission
header for their scenario. Keep the systems root at the origin; its AI navigation
resources belong to the terrain's coordinate system.

## AI-world selection

The known Eden, Arland and Cain terrain mappings remain preferred. Other terrains
are discovered from `.et` resources in loaded addons, including inherited prefab
root types derived from `AIWorld`. No new per-terrain mapping is required.

Discovery first checks resource paths containing the terrain scene's filename
(without extension). If none are valid AI worlds, it checks all prefab paths in
the loaded addons. It de-duplicates eligible resources and chooses the first in
sorted resource-name order, so multiple candidates give a repeatable result.
Names prioritize candidates; they never establish type or navigation compatibility.

Every discovered selection shows a notice with the chosen prefab, candidate count,
search scope and terrain. Check `GC_MissionAIWorld` and its navigation meshes and
replace it if necessary before saving or playing. Failed resource loads and
non-AI roots are excluded. A failed search or no eligible candidates prevents
placement. The scan runs only on demand in the editor and does not spawn candidates.

The command refuses prefab mode, Play and locked mission layers. It never places
entities into the base terrain subscene. Existing game modes, AI worlds and
faction/radio/chat managers are conflicts, including inherited layers
and prefab children. It reports the first conflict without deleting, replacing
or reparenting existing work. Failed AI placement removes the new root; if cleanup
fails, the message directs the author to inspect or Undo before retrying.

## Existing perception manager

The plugin reuses a perception manager anywhere in the loaded world, including
terrain layers and prefab children. It sets the native `EntityFlags.DISABLED` flag on only the newly placed game-mode
instance's bundled perception child, in the same Undo action. This prevents the
child from spawning anywhere while keeping it in the prefab hierarchy. The shared
prefab definition and existing managers remain unchanged.
Without an existing manager the bundled child stays. The game mode and supporting
entities remain one prefab hierarchy; there is no separate replacement prefab,
external executable or runtime dependency. Manual dragging uses the prefab's
normal contents and does not run the plugin's AI discovery or perception checks.
