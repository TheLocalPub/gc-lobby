# Admin gear pointer checks

Mount `scripts/Game` under the Workbench profile's `scripts/Game/GearTransferChecks`, reload, and use disposable Arland Play. The helper runs only through explicit MCP calls on `GC_MissionTemplate` / `GC_LobbyManager`. It uses real phase, slot and admin-gear requests; it does not grant permissions. `GC_TestGear("inspect")` reports rendered item centers and native removal eligibility. `GC_TestGearInput(x, y, action)` reports native widget hits and invokes the normal inventory handler for `right`, `down`, `drag`, or `up`. Coordinates are viewport pixels returned by the snapshot. This exercises widget dispatch, not physical mouse input.

## Regression evidence, 2026-11-16

Workbench 1.9.0.37, session `logs_2026-11-16_14-51-23`:

- Original MCP screenshot: the unused Vicinity heading was visible over the inventory column containing the jacket and Lifchik – Grenadier vest.
- Runtime snapshot `66f6dd44fa404d8183ad3420443ebcdc`: admin session active, inventory unlocked, `crateVisible=1`. Contents had native removal permission.
- Pointer inspection `db079fc9890344219b706c9a75850f49`: the tourniquet's rendered center hit `GearCrateScroll` instead of the item.
- Explicit diagnostic hide `e6e11ebb62d24115b0e380858ec4e51c`, followed by inspection `f188066e9bba43ff87b1bd0fce6112fd`: the same center hit `ItemButton` with the expected tourniquet.
- Installed base-game layout reference: `game/data/data010.pak/UI/layouts/Campaign/BaseElement.layout`, line 49, catalogue revision `366e99137dac49b7a1fde7c094cd92b9`, uses `"Is Visible" 0`. The admin crate panel incorrectly used `Visible 0`.

The production fix corrects that layout property. No inventory permission, transfer, phase or multiplayer behavior changes.

After fix, session `logs_2026-11-16_15-19-33`:

- Fresh startup compilation had no SCRIPT errors. The preceding Reload crashed Workbench; the same GC Workshop project and Arland world were restored through MCP.
- Snapshot `98244ef35eeb47ccb8a5941747c49267`: `crateVisible=0` without the diagnostic hide. MCP screenshots show the unused Vicinity heading gone.
- Drag operations `2d9998393ed74aff9c81cab26cd5493f`, `c9726131601f4f9baa045ffe8a7e75b1`, `0bb8460b9cca4e298066a2f221f39ff1`: tourniquet moved from pants to jacket. Snapshot `a1a2fc219bbc476f8eb0281f78ee3f5a` confirms its new rendered position; console lines 29802–29803 confirm native removal from Pants_M88 and insertion into Jacket_M88 for the same replicated item.
- Right-click operation `f43de23008284fa88c5a32cd7efe794b`: flashlight removed from Lifchik GL; console line 29870 confirms native removal.
- Drag operations `35c9480f65ea4e25a435098d410cb936`, `2a6717fd5c414a5ab4cc8f104ab2f3f5`, `c59ae74589f547e9bc55a8fb4f9914c4`: tracer magazine moved from Lifchik GL into pants; console lines 29877–29878 confirm both sides for the same replicated item. The receiving pants occupied the previously obstructed right column.
- Final MCP capture at approximately 15:21:39 shows jacket contents and the vest without its flashlight. Separate remote multiplayer, controller input and crate-view switching were not retested for this one-property layout correction.
