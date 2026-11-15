# GC core-system audit

Audit date: 2026-11-15. Scope: all **137 non-vanilla addons** in the current
Global Conflicts Workbench collection's transitive dependency closure. This is
an inventory and source-based classification, not a compatibility certification
or an enabled server preset. The four obsolete tools listed below have been
removed from GC Core Dev and the Workbench collection at the user's request.

## Removed tools

Server Admin Tools (`5AAAC70D754245DD`), Scenario Reload Menu
(`606D03292879EF5B`), Bacon Loadout Editor (`606B100247F5C709`) and Everything
Arsenal (`645AAE6848D75398`) were removed on 2026-11-15. They are no longer core
candidates or Workbench collection dependencies. The legacy SAT AdminTools and
MOTD pause-menu buttons were also removed from GC Core. The GC Lobby admin and
mission interfaces remain the supported GC controls. Installed Workshop caches
and already-published release packages are not changed by source dependency edits.

Validation: a fresh Missions Dev session in Workbench 1.9.0.37 loaded 139 addons
(137 non-vanilla plus two base-game addons), down from 143. The saved dependency
closure contains none of the four removed GUIDs. Startup compilation and a fresh
Reload completed, and the Arland Lobby world entered Play and rendered its lobby
preview. Every pause-layout token outside the two removed button subtrees was
preserved. Log session `logs_2026-11-15_10-37-03` still contains world/content
errors, including Pronto/TF component and equipment-resource references also
present in the earlier `logs_2026-11-15_09-10-36` session; this is not a clean
whole-pack compatibility test. The pause menu itself was not exercised through
the available MCP controls, so its rendered state remains unverified.

## Meaning of core

Core systems supply GC's chosen gameplay rules and shared functionality, even
when a particular mission places no object that references them. Adult Mortars
and GC Suppression are explicit examples from the requested policy. They must
not disappear because a mission-content scan cannot discover their script hooks.

A system mod can legitimately contain UI, helper prefabs, ammunition overrides,
medical supplies or other supporting assets. Counting `.et` files is not a
useful core/content classifier. Classify the feature and its dependency cost.
Conversely, a mod named Core, Framework or Arsenal is not automatically a
universal GC dependency.

The prototype now accepts repeated `scan --core-mod GUID` arguments. These
become explicit `coreAddonIds` in the mission sidecar and contribute native
dependencies before resource traversal. No default list is silently enabled.
Runtime-script and packed-resource coverage checks still apply.

**Required mods = selected core systems + mission content + dependencies of both.**

## Core-system candidates

These are the system-first candidates in the current collection. “Candidate”
does not mean that every feature should be enabled together, that their current
versions are compatible, or that a source read proves their runtime behavior.
GC Lobby and a reduced GC Core would be the GC-owned framework packages; the
current GC Core cannot serve as the reduced package until its content and
integration dependencies have been separated.

| Mod | Why it belongs in the system discussion | Supporting content / qualification |
| --- | --- | --- |
| Adult Mortars | Changes mortar ammunition ballistics/charge behavior; explicitly requested as core | Existing mortar/ammunition/ballistic-table changes; not a reason to load every artillery pack |
| GC Suppression | Registers fired projectiles and runs suppression behavior | Projectile/player prefab changes support the system |
| GC Line of Sight | Map UI and line-of-sight tracing system | Registered through map/system configurations |
| Grays Adjustable Doors | Adds adjustable-door actions to the door base | Existing door behavior, not a terrain pack |
| Maeh-NoWeapDropUnCon | Unconscious weapon-retention behavior | Existing character behavior |
| Mountable BTR-70 | Extends interaction with the existing BTR-70 | Vehicle-specific system; does not imply all vehicle packs |
| CSI - COALITION Squad Interface | Squad/HUD/player-data systems | Current Core directly extends CSI classes |
| Enhanced Maps | Replaces satellite imagery in existing map entities | Arland/Everon configurations; do not infer all custom terrain support |
| Woodys Soviet Map Markers | Map-marker presentation | Shared UI feature |
| ACE Core Dev | Shared ACE runtime/settings/helper support | Required by the selected ACE features; also contains small supporting/example items |
| ACE Captives Dev | Captive-state and restraint actions | Adds zip cuffs and helpers |
| ACE Carrying Dev | Casualty carry/drag behavior | Character/helper prefabs |
| ACE Chopping Dev | Foliage cutting/tool behavior | Existing entrenching-tool configuration and cutting helpers |
| ACE Compass Dev | Compass/HUD behavior, including magnetic north | Existing compass/controller integration |
| ACE Cook-Off Dev | Ammo cook-off scheduling and flammable-hit-zone behavior | Helper resources |
| ACE Explosives Dev | Mine/explosive interaction systems | Also adds a detector and dead-man switch |
| ACE Finger Dev | Pointing, ping and map-pointer functionality | UI/sound/helper resources |
| ACE Magazine Repack Dev | Magazine-repacking behavior/timing | Reload helper and magazine configuration |
| ACE Medical Core Dev | Bleeding, pain, second-chance and treatment systems | Medical items/litter; not a zero-content package |
| ACE Medical Circulation Dev | Patient vitals, circulation, medication and CPR | Depends on Medical Core, which brings ACE Core |
| ACE Overheating Dev | Barrel heat and weapon-jamming behavior | Spare-barrel/helper assets and existing weapon configuration |
| ACE Trenches Dev | Entrenching/deployment/building functionality | Adds trench/cover structures: a system with real supporting content |
| ACE Weather Dev | Weather/temperature behavior | Adds Kestrel equipment and changes time/weather configuration |
| GC Splints | Splint/treatment functionality | Includes a splint item; review with the selected medical system |

## Other systems that need an explicit decision

- **Map Drawing:** a system feature, but the existing GC Lobby integration is
  explicitly optional and belongs in its companion addon. This audit does not
  make it mandatory or merge it into Lobby/Core.

- **FPTR:** system-like existing-vehicle changes, but its native project directly
  requires **RHS: Status Quo**, which in turn requires its two content packs.
  Making FPTR unconditional currently makes RHS content unconditional too. Keep
  that cost explicit; split/replace the integration if a lean baseline must not
  require RHS.
- **FunctionalBangalore / Wirecutters2:** interaction mechanics coupled to
  equipment. They can be core capabilities if GC wants them universally, or
  selected equipment features. Do not decide that solely from their scripts.
- **ACE Facepaint / Tactical Ladder / Tactical Periscope:** equipment-led
  features with behavior scripts. Keep as selected capabilities/content unless
  GC chooses them for the shared baseline.

- **Chungus Core, WCS Weapon Scripts, SightSystem, MFD Framework, Grey Man's
  Parachute Framework and similar support packages:** they contain systems,
  but are required by particular equipment families. Preserve them through
  those packages' dependency closure instead of automatically making every
  support library a universal gameplay requirement.
- **TilW Mission Framework:** it is a systems framework, but project policy
  explicitly excludes it as a GC Lobby dependency. Its presence in the legacy
  collection is not a reason to add it to the new baseline.

## Evidence and limits

Native `.gproj` declarations were read from the installed/source addons, and
the installed PAK catalogue was checked through Reforger MCP. Archive revision:
`c52648a62fcb4229ad089610d060ea44`. PAK file paths establish package contents;
selected script/prefab reads establish the system classifications. They do not
prove effective override order, behavior in every mode, or multiplayer safety.

Representative inspected sources (addon-relative paths, one-based lines):

- Adult Mortars: `Prefabs/Weapons/Ammo/Ammo_Shell_81mm_HE_M821.et:1` inherits
  mortar-shell base and changes charge-ring configuration, speed variation and
  ballistic/wind-table references (lines 4–13).
  `Prefabs/Items/Equipment/BallisticTable/BalisticTable_US.et:4` supplies revised
  ballistic pages. Its inspected diagnostic scripts are `WORKBENCH`-guarded;
  the runtime classification rests on resource configuration, not those scripts.
- GC Suppression source checkout: `scripts/Game/Modded/SCR_MuzzleEffectComponent_M.c:1`
  overrides firing and registers projectiles; `scripts/Game/GC_SuppressionSystem.c:1`
  defines the suppression GameSystem and its settings.
- CSI: `Scripts/Game/Core/WorldSystems/CSI_HUDSystem.c:1` defines the HUD
  GameSystem, with client/frame registration at lines 21–26.

- TroopFit: `Scripts/Game/MorphPatchSystem.c:1` describes patch-slot morphing;
  the package inventory also contains many actual character/patch prefabs. It
  is not just a generic arsenal system despite its title.
- GC Line of Sight: `Configs/Map/MapFullscreen.conf:1` adds the map UI component,
  and `Configs/Systems/ChimeraSystemsConfig.conf:1` registers its tracing system.
- GC Splints: `Prefabs/Items/Medicine/Splint_01/GC_Splint_01_Base.et:1` supplies
  the item; its `GC_ConsumableSplint.c` health-item effect applies treatment.
- FPTR: native `.gproj` requires RHS; its `Prefabs/Vehicles/Wheeled/BTR70/BTR70_Base.et:6`
  adds RHS periscope slots to the vehicle configuration.
- Map Drawing: `Configs/Systems/ChimeraSystemsConfig.conf:1` registers a client
  drawing system and `Prefabs/MP/Modes/GameMode_Base.et:1` adds synchronization.
- Enhanced Maps: `Prefabs/World/Game/Arland/MapEntity_Arland.et:1` and
  `Prefabs/World/Game/MapEntity_Everon.et:1` replace satellite imagery.
- Chungus Core: `scripts/Game/Character/BCC_CharacterControllerComponent.c:1`
  documents and implements custom bolt/shotgun reload/rack/round insertion.
- ACE Captives: `scripts/Game/ACE_Captives/Systems/ACE_Captives_CaptiveSystem.c:2`;
  Carrying: `Scripts/Game/ACE_Carrying/Character/SCR_CharacterControllerComponent.c:5`;
  Chopping: `scripts/Game/ACE_Chopping/Building/SCR_CampaignBuildingGadgetToolComponent.c:5`.
- ACE Compass: `scripts/Game/ACE_Compass/Components/Gadgets/SCR_CompassComponent.c:24`;
  Cook-Off: `scripts/Game/ACE_CookOff/HitZone/SCR_FlammableHitZone.c:2`;
  Magazine Repack: `scripts/Game/ACE_MagRepack/Systems/SCR_MagazineRepackingSystem.c:22`.
- ACE Medical: `scripts/Game/ACE_Medical_SecondChance/Systems/ACE_Medical_SecondChanceSystem.c:2`,
  `scripts/Game/ACE_Medical_Bleeding/HitZone/SCR_CharacterBloodHitZone.c:2`, and
  `scripts/Game/ACE_Medical_Circulation/Components/ACE_Medical_PatientData.c:2`.
- ACE Overheating: `Prefabs/Weapons/Core/Rifle_Base.et:1` adds barrel/helper
  components; Trenches: `scripts/Game/ACE_Trenches/Building/ACE_Trenches_BuildingCompositionComponent.c:2`;
  Weather: `scripts/GameCode/ACE_Weather/World/TimeAndWeatherManagerEntity.c:2`.
- ACE equipment evidence: `Prefabs/Items/ACE_Facepaint_01_Base.et:1`,
  `Prefabs/Items/Equipment/TacticalLadder/ACE_TacticalLadder.et:1`, and
  `Prefabs/Items/Equipment/Periscope/ACE_TacticalPeriscope.et:1`.

All 17 ACE packages have only base-game/ACE dependencies in this installed
snapshot. ACE Core requires only the game; the other features require ACE Core,
except Medical Circulation directly requires Medical Core. This does not verify
that mixing their current versions is safe.

## Complete collection inventory

The table includes transitive dependencies, not only the 101 non-vanilla direct
addons in the collection prototype (102 direct entries including the base game).
A representative source path is an evidence locator,
not a claim that the package contains only that file or behavior. `System+assets`
still belongs in the core discussion; `Conditional system` is a feature decision;
`Content support` follows selected content rather than becoming mandatory by name.

| Mod | Addon GUID | Classification | Representative package source |
| --- | --- | --- | --- |
| 6B5 | `652CFAA0BB9EFE74` | Content | `Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AAT_Late.et` |
| 6TP_Military_Free_Fall | `6867754CA817D856` | Content | `Prefabs/Items/Equipment/Backpacks/6TP_Jump_Bag.et` |
| ACE Captives Dev | `65AD7C249E4ECDFB` | System+assets | `scripts/Game/646D52AF8BB3FF15.c` |
| ACE Carrying Dev | `65AD7C379CBD394D` | System | `Scripts/Game/5DBD560C5148E1DA.c` |
| ACE Chopping Dev | `65AD7BCC9F6B3B4E` | System | `scripts/Game/5EB744C5F42E0800.c` |
| ACE Compass Dev | `65AD7CF69FAF1FDD` | System | `scripts/Game/60C53A9372ED3964.c` |
| ACE Cook-Off Dev | `664AFDC993C9CE1A` | System | `scripts/Game/ACE_CookOff/Helpers/ACE_CookOff_HelperEntity.c` |
| ACE Core Dev | `65AD7D0D9941A380` | Core system support | `scripts/Game/60C4CE4888FF4621.c` |
| ACE Explosives Dev | `65AD7D1E9EEAFA53` | System+assets | `scripts/Game/61B7763A8AEB53B7.c` |
| ACE Facepaint Dev | `68EAA4B96976D557` | Equipment feature | `Prefabs/Items/ACE_Facepaint_01_Base.et` |
| ACE Finger Dev | `65AD7D2E9866FA6E` | System | `scripts/Game/606C369BAC3F6CC3.c` |
| ACE Magazine Repack Dev | `65AD7D4099944EBD` | System | `scripts/Game/611CB1D409001EB0.c` |
| ACE Medical Circulation Dev | `65AD7D4F994EA327` | System+assets | `scripts/Game/6214CEA5DA92BFF3.c` |
| ACE Medical Core Dev | `6586079789278413` | System+assets | `scripts/Game/ACE_Medical_SecondChance/Systems/ACE_Medical_SecondChanceSystem.c` |
| ACE Overheating Dev | `65B8EA178A3E94E3` | System+assets | `scripts/Game/ACE_Overheating/Character/SCR_CharacterControllerComponent.c` |
| ACE Tactical Ladder Dev | `65AD7CB89F219E38` | Equipment feature | `Prefabs/Items/Equipment/TacticalLadder/ACE_TacticalLadder.et` |
| ACE Tactical Periscope Dev | `65AD7CD09CAEC4E2` | Equipment feature | `Prefabs/Items/Equipment/Periscope/ACE_TacticalPeriscope.et` |
| ACE Trenches Dev | `65AD7CE59E8DB349` | System+assets | `scripts/Game/60EAEA0389DB3CC2.c` |
| ACE Weather Dev | `667B230F9505C8BA` | System+assets | `scripts/Game/ACE_Weather/Components/Gadgets/ACE_Weather_KestrelGadgetComponent.c` |
| Adult Mortars | `6318DCB19B389CC8` | System | `Prefabs/Weapons/Mortars/M252/Mortar_M252.et` |
| AKI_Core | `62CCD69DD17E4F2F` | Content support | `Prefabs/Characters/HeadGear/Helmet_Deathwish/Helmet_Deathwish_base.et` |
| ALHadra | `68957914EA45BA6C` | Content | `Worlds/AL_Hadra/AL_Hadra_terrain.ent` |
| Anizay | `6266DCA9193C705E` | Content | `Worlds/Anizay.ent` |
| Arma Terrain Core | `60B524F7A5AE96A8` | Content support | `worlds/atc_world.ent` |
| ARMA-RY AK74 | `666ADD0DDD59F6E7` | Content | `Prefabs/Weapons/Attachments/Furniture/ARY_AK74_Furniture_Polymer/ARY_AK74_Furniture_Base.et` |
| ARMA-RY CLAW MOUNT | `6614BC5EC4E1675B` | Content | `Prefabs/Weapons/Attachments/Optics/ARY_Optic_APElectronic/ARY_Collim_APElectronic.et` |
| ARMA-RY CORE | `6614B4EBC1A63458` | Content support | `Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_AAT.et` |
| ARMA-RY G3 Battle Rifles | `6614B9D9C6D7D565` | Content | `Prefabs/Weapons/Attachments/Bayonets/ARY_Bayonet_G3.et` |
| ARMA-RY M1 HELMETS | `664928B1C3E554ED` | Content | `Prefabs/Characters/Eyewear/Goggles_PASGT_01_M1Helmet.et` |
| ARMA-RY SOVIET EQUIPMENT | `671F3CDAC20CDDC1` | Content | `Prefabs/Characters/Footwear/ARY_CombatBoots_Soviet_Kirza_01.et` |
| ARMA-RY UZI | `668F458EC133AE50` | Content | `Prefabs/Characters/Factions/BLUFOR/US_Army/GreenBerets/Suppressed/ARY_Character_US_SF_S_UZI.et` |
| AUS_CORE | `6276E6E3CC97A22B` | Content support | `Prefabs/Characters/Footwear/AUS_AirCrew_Boots_01/AUS_AirCrew_Boots_Bwn.et` |
| Bad Orb 5km | `64A51967EC407267` | Content | `Worlds/Dev Branch/Bad_Orb_5km.ent` |
| Big Chungus Bolt Guns | `61BD6595183FCEBD` | Content | `Prefabs/Particles/Bullet_Case/BC_Bullet_Case_762x51_M40A1.et` |
| Big Chungus LMGs | `61344BDC155A5A28` | Content | `Prefabs/Weapons/Ammo/BC_Ammo_762x51_Ball_M2.et` |
| Big Chungus Rifles | `62A711001B8FDEEA` | Content | `Prefabs/Particles/Bullet_Case/BC_Bullet_Case_30_Carbine.et` |
| British Forces | `5AE50EC5B8D6F4AE` | Content | `Worlds/Editor/Slots/AssetImages/Eden_AssetImages_BritishForces.ent` |
| British Forces Desert DPM | `64FB0148B8957DEE` | Content | `Prefabs/Characters/HeadGear/Helmet_Mk6_01/Helmet_Mk6_DesertDPM.et` |
| Bystrany | `6626CB89B6B9C913` | Content | `Worlds/Bystrany/Bystrany.ent` |
| CADPAT Covers for RHS ECH | `689744F549F471FF` | Content | `Prefabs/Characters/HeadGear/Helmet_ECH/Headgear_ECH_cover_c_arid.et` |
| Chungus Core | `66D6340C1D565C98` | Content support | `scripts/Game/Character/BCC_CharacterCommandHandlerComponent.c` |
| Chungus Launchers | `6190F1B505C08562` | Content | `Prefabs/Weapons/Ammo/BC_Ammo_Grenade_M576_Buckshot.et` |
| Chungus Shotguns | `620E584B1D2C96A4` | Content | `Prefabs/Weapons/Ammo/Ammo_12ga_buckshot_12_in_barrel.et` |
| Chungus SMGs | `60E6F54E174C53C5` | Content | `Prefabs/Characters/Handwear/Gloves_Leather_01/BC_Gloves_Leather_01.et` |
| CSI - COALITION Squad Interface | `5B0D1E4380971EBD` | System | `Scripts/Game/Core/Enums & Containers/CSI_Enums.c` |
| Cuban Lizard | `624E761E6B1DE8DF` | Content | `Prefabs/Characters/HeadGear/Hat_Patrol_US_01/Hat_Patrol_Castro.et` |
| Drums Gear and Clothing Pack | `654F178CCD7D1122` | Content | `Prefabs/Characters/Eyewear/Bolle X800/Bolle X800 1.et` |
| Enhanced Maps | `644B042109700804` | System | `Prefabs/World/Game/Arland/MapEntity_Arland.et` |
| FPTR | `607D472207F3EE65` | Conditional system | `Prefabs/Vehicles/Wheeled/BRDM2/BRDM2_base.et` |
| FunctionalBangalore | `66A68CAFC8B00A5C` | Conditional system | `scripts/Game/Components/UserActions/SCR_BangaloreDuplicateAction.c` |
| GC Line of Sight | `6A4AF7AA127CC8A1` | System | `scripts/Game/LOS/GC_LineOfSightUI.c` |
| GC Splints | `668694557A25D236` | System+assets | `scripts/Game/Components/Gadgets/GC_ConsumableSplint.c` |
| GC Suppression | `684CE8AA3B1D6573` | System | `scripts/Game/Modded/SCR_MuzzleEffectComponent_M.c (source checkout)` |
| Global Conflicts CORE Dev | `623555110E2B2CA1` | GC package (migration/ownership) | `addon.gproj (source checkout)` |
| Global Conflicts Lobby - Dev | `6A115D5DFF7ED870` | GC package (migration/ownership) | `addon.gproj (source checkout)` |
| Global Conflicts Missions Dev | `62D55563549DA97B` | GC package (migration/ownership) | `addon.gproj (source checkout)` |
| Global Conflicts Units Dev | `6156F2F771D5D73E` | GC package (migration/ownership) | `addon.gproj (source checkout)` |
| Grays Adjustable Doors | `655D72AF8DB4CB84` | System | `scripts/game/Player/GRAY_DoorActions.c` |
| Grey Man's Parachute Framework | `69EEAA01F4A11001` | Content support | `worlds/GameMaster/GM_Arland.ent` |
| IBA Vest | `6858EC86812228AE` | Content | `Prefabs/Characters/Vests/Vest_IBA/Variants/Collar/Vest_IBA_NeckProtection_v2_3CD.et` |
| IF Plate Carrier | `644B2579745935DA` | Content | `unnamed.ent` |
| Insurgents SAF | `6167529D6B3F6BE4` | Content | `Prefabs/Characters/Footwear/CombatBoots_Insurgents.et` |
| JAM NVA | `646156E84AA96246` | Content | `Prefabs/Characters/Footwear/Boots_Jackboots/Boots_Jackboots.et` |
| Kunar Province | `5C9691EA7FD7A79F` | Content | `Worlds/GameMaster/GameMaster_Kunar.ent` |
| Laser Rangefinder | `620E426C34BE0D17` | Content | `Prefabs/Items/Equipment/Rangefinder/Rangefinder.et` |
| Longmoor | `61BD1072371B5470` | Content | `Worlds/GM_Longmoor/GM_Longmoor.ent` |
| M113 | `5E5C154FEE1094BB` | Content | `Prefabs/Vehicles/Core/Vehicle_Part_US_Radio.et` |
| M4MAAWS | `61C2509A023211FE` | Content | `Walds/StarsRange.ent` |
| Maeh-NoWeapDropUnCon | `60612C225328522E` | System | `scripts/Game/Character/Maeh_CharacterControllerComponent.c` |
| Map Drawing | `656AC01634459D8D` | Conditional system | `scripts/Game/Components/TILW_DrawingSyncComponent.c` |
| MES_2 | `68A2C605FC3B76A6` | Content support | `Prefabs/MES_Prefabs/Apartment_13_WinDoor.et` |
| MFD Framework | `64EE818E08AFCF94` | Content support | `Scripts/Game/Components/AG0_MapGadgetComponent.c` |
| Modular bits | `662D4D48EBB10146` | Content support | `MES_TERRAIN/Gamemaster.ent` |
| Motorbike | `61B84088181A8FA4` | Content | `Prefabs/Vehicles/Core/Lights/Motorbike_Brake_Light.et` |
| Mountable BTR-70 | `617A2E90EA218AC4` | System | `Prefabs/Vehicles/Wheeled/BTR70/roof_comp_v1.conf` |
| Outside Terrain Core | `61F0D506562D9BC9` | Content support | `Scripts/Game/Entities/OTC_OutsideTerrainChunkEntity.c` |
| Pronto Hub | `631A9C6C92283C44` | Content support | `worlds/Editor/Slots/AssetImages/Eden_AssetImagesBetter.ent` |
| Prontos AR15s | `66F125D025E4C36B` | Content | `Prefabs/Weapons/Attachments/ChargingHandles/BCM/Pronto_ChargingHandle_BCM_01.et` |
| Prontos Attachments | `64656BF043969017` | Content | `Prefabs/Weapons/Attachments/Lights/Core/Pronto_IR_Flashlight.et` |
| Prontos Brit Kit | `5E8CA06EE59E7F13` | Content | `PBK_World.ent` |
| Prontos FN Minimi | `692AB768AE4E8CEB` | Content | `Prefabs/Weapons/Attachments/Muzzle/FlashHider_FN_Minimi/FlashHider_FN_Minimi_01.et` |
| Prontos L119s | `62A61619A64F97BD` | Content | `Prefabs/Weapons/Attachments/Grips/Magpul_VFG_Short_BLK.et` |
| Prontos L403A1 | `624702ECDEC10628` | Content | `KS1.ent` |
| Prontos Parachutes | `68996556700D5CA1` | Content | `Prefabs/Characters/Core/Character_Base.et` |
| Prontos SA80 | `62CAF6D0B5268190` | Content | `Prefabs/Weapons/Ammo/Ammo_556x45_Blanks.et` |
| Rail Enfusion Redux | `5DE612963E4B005A` | Content support | `Prefabs/Props/Industrial/Train/loco_chme3/loco_chme3_base.et` |
| Railway Generator | `5F0D245931200FD1` | Content support | `Prefabs/WEGenerators/Railway/RWG_base.et` |
| Realistic Combat Drones | `65AD60E204191D37` | Content | `Prefabs/Characters/Core/Character_Base.et` |
| RHS: Status Quo | `595F2BF2F44836FB` | Content | `Worlds/Arland_empty_RHS/Arland_empty_RHS.ent` |
| RHS: Status Quo - Content Pack 01 | `1337C0DE5DABBEEF` | Content support | `addon.gproj (source checkout)` |
| RHS: Status Quo - Content Pack 02 | `BADC0DEDABBEDA5E` | Content support | `addon.gproj (source checkout)` |
| Ruha | `653CB36244ADBE0F` | Content | `worlds/Gamemaster/GM_Ruha.ent` |
| SB300 | `687268B1992E5B17` | Content | `Prefabs/Drones/DJIBase/SB300RECON.et` |
| SightSystem | `696AFA6207142000` | Content support | `Prefabs/Vehicles/Core/Vehicle_Base.et` |
| Sikorsky H60 Project | `60ED3CC6E7E40221` | Content | `Worlds/AUS_AssetImages.ent` |
| Tactical Flava | `5D550926D43F1409` | Content | `Prefabs/Compositions/Misc/Tutorial/BaseSandbags.et` |
| Tactical Flava - 2B9 Vasilek | `DEADC0DE00000008` | Content | `Prefabs/Items/Equipment/BallisticTable/TF_BallisticTable_USSR_2B9.et` |
| Tactical Flava - BM-21 | `DEADC0DE00000002` | Content | `Prefabs/Items/Equipment/9B370MKey/9B370M_Key.et` |
| Tactical Flava - Clothes | `DEADC0DE00000004` | Content | `Prefabs/Characters/Footwear/ACU_boots/ACU_boots1_base.et` |
| Tactical Flava - Core | `DEADC0DE00000000` | Content support | `Assets/Weapons/HeavyWeapons/M2/HMG_M2HB_Pintle_GPK_Shield.et` |
| Tactical Flava - GAZ TIGR | `DEADC0DE00000003` | Content | `Prefabs/Vehicles/Wheeled/TIGR/AGS/GAZ_TIGR_v2_AGS17.et` |
| Tactical Flava - HMMWV | `DEADC0DE00000005` | Content | `Prefabs/Vehicles/Wheeled/M998/Door/M1167_Hatch.et` |
| Tactical Flava - M-ATV | `DEADC0DE00000006` | Content | `Prefabs/Vehicles/Wheeled/IvecoLMV/VehParts/Turret/Gun_Shield_IvecoLMV.et` |
| Tactical Flava - Turrets | `DEADC0DE00000007` | Content | `Prefabs/Items/Equipment/BallisticTable/TF_1L271_Manual.et` |
| Tactical Flava - Weapons | `DEADC0DE00000001` | Content | `Prefabs/Weapons/Ammo/Ammo_Grenade_Smoke_GDM40.et` |
| Takistan | `615EEBD9BDFEEE9B` | Content | `worlds/Takistan/Takistan.ent` |
| TilW Mission Framework | `61F0E6AB2D06B1C6` | Legacy framework — excluded | `Prefabs/Characters/Core/DefaultPlayerControllerMP_Coop.et` |
| TOFTS_M777_Howitzer | `686AC479236F235D` | Content | `Worlds/GM_Arland_range.ent` |
| ToH ReCharacters African | `65FB50F7FA33DECA` | Content | `Prefabs/Characters/Heads/Head_AfricanHead_01.et` |
| ToH ReCharacters Core | `64C912EF952E1075` | Content support | `Prefabs/Characters/Basebody/CharacterBasebody_01.et` |
| ToH ReCharacters Persian | `65FB762EB503CCF9` | Content | `Prefabs/Characters/Heads/Head_Persian_01.et` |
| TroopFit Arsenal | `686146A940C9D3EA` | Content | `Prefabs/Characters/Core/Character_Base.et` |
| WCS_AH-1S | `64CB39E57377C861` | Content | `Prefabs/Vehicles/Helicopters/AH1S/AH1S_Base.et` |
| WCS_AH-64D | `6303360DA719E832` | Content | `Prefabs/Vehicles/Helicopters/AH64D/AH64D_Base.et` |
| WCS_AH-6M | `6273146ADFE8241D` | Content | `Prefabs/Props/Military/SupplyBox/SupplyStack/SupplyStack_01/SupplyBox_Base.et` |
| WCS_Armaments | `629B2BA37EFFD577` | Content support | `Worlds/WCS_Armaments.ent` |
| WCS_BMP-1 | `5E524E4FEECCA92B` | Content | `Prefabs/Vehicles/Tracked/BMP1/BMP1P_Base.et` |
| WCS_JLTV | `5C721177A220B42F` | Content | `Worlds/WCS_JLTV/WCS_JLTV.ent` |
| WCS_M1A1 | `5D1880C4AD410C14` | Content | `Particles/Vehicle/M1A1_Fire_Light.et` |
| WCS_M2A2 | `63120AE07E6C0966` | Content | `Worlds/WCS_M2A2/WCS_M2A2.ent` |
| WCS_Mi-24V | `628933A0D3A0D700` | Content | `Prefabs/Vehicles/Helicopters/Mi24/Lights/VehicleLight_Mi24V_Cabin_Bright.et` |
| WCS_NATO | `615806DC6C57AF02` | Content support | `addon.gproj (source checkout)` |
| WCS_RU | `615818DA7C0343FD` | Content support | `addon.gproj (source checkout)` |
| WCS_Scopes | `62A668F513428630` | Content | `Prefabs/Weapons/Attachments/Optics/Collimators/Aimpoint_ACRO_BT/Optic_Aimpoint_ACRO_BT.et` |
| WCS_Sounds | `631C3C1AEE9C90BC` | Content support | `Prefabs/Weapons/Ammo/Ammo_Rocket_PG7VM.et` |
| WCS_SpaceCore | `5E389BB9F58B79A6` | Content support | `Worlds/ImageGeneration/ImageGenerator.ent` |
| WCS_Stryker | `5B02128D896F7DE8` | Content | `Prefabs/Vehicles/Wheeled/Stryker/M1126/CROWS/Javelin/Stryker_CROWS_Javelin.et` |
| WCS_T-72 | `5E0AB16BEB16D6A4` | Content | `Prefabs/Vehicles/Core/ERA/Kontakt_Base.et` |
| WCS_Weapon_Scripts | `68F006D910E7546F` | Content support | `Scripts/Game/Character/Cameras/ADS/WCS_Scopes_CharacterCameraADS.c` |
| Wirecutters2 | `62F364B35E9B51B0` | Conditional system | `scripts/Game/Components/Gadgets/SCR_WirecutterComponent.c` |
| WolfsBuildingPack | `5D84C738432BFE6A` | Content support | `Preview World/WorldPreivew.ent` |
| Woodys Soviet Map Markers | `6268CA95211B65E9` | System | `Configs/Map/MapMarkerConfig.conf` |
| Zagoria 89 BMP2 | `6730D59067916E3D` | Content | `IFV/ACSpall.et` |
| Zagoria BMP-2 FIX | `68C322898FDCBBEA` | Content | `IFV/BMP2/30mm 250rnds.et` |
| Zarichne | `65EA8C9C25882838` | Content | `Worlds/GameMaster/GM_Zarichne.ent` |
| Zelik's Character | `5D0551624969C92E` | Content support | `Worlds/MP/unnamed.ent` |
| ZU-23 | `5C9AD9EF76F6B5EA` | Content | `worlds/ZU23/GM_Arland/GM_Arland.ent` |

Inventory counts: 4 Conditional system, 75 Content, 26 Content support, 1 Core system support, 3 Equipment feature, 4 GC package (migration/ownership), 1 Legacy framework — excluded, 15 System, 8 System+assets. Total: 137.
