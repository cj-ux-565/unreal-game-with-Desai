# SOTM Chapter 1 — Foundation Cleanup Report

**Date:** 29 July 2026  
**Milestone:** Foundation cleanup and packaged smoke test  
**Scope:** Production Main Menu, Mansion, and Forest architecture only

No health, lives, checkpoints, coins, AI, objectives, upgrades, Timmy, boss-fight, or cutscene system was implemented in this milestone.

## Executive result

| Area | Result |
|---|---|
| Production startup map | Passed — `/Game/Main_Menu_Map` |
| New Game destination | Passed — authoritative `/Game/Mansion_GameStart` |
| Production Forest | Passed — `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1` |
| Correct Mansion GameMode/player contract | Passed — `BP_PlayLevelGameMode`, whose configured pawn is `BP_MenuSystemCharacter` |
| Mansion missing environment/audio packages | Restored or repaired |
| Duplicate Mansion navigation data | Repaired |
| Zero-scale Mansion wall physics actor | Repaired in the authoritative map |
| Localized duplicate hijacking Mansion travel | Repaired by recoverable quarantine |
| Production Blueprint compile audit | Completed; two inherited menu-widget compiler blockers remain |
| Three-map Development cook/package | Passed, UAT `ExitCode=0` |
| Packaged Main Menu → New Game → Mansion | Passed interactively |
| Packaged Mansion → Forest | Passed through the Development console; the in-world overlap trigger was already verified in PIE in the preceding foundation milestone |
| Development map launcher | Added for Main Menu, Mansion, and Forest |

## Approved production maps

| Role | Asset |
|---|---|
| Main Menu | `/Game/Main_Menu_Map` |
| Mansion | `/Game/Mansion_GameStart` |
| Forest | `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1` |

The Forest remains in the marketplace-style `MenuSystemPro/ExampleContent` path because it is a populated World Partition map with hundreds of external packages. It was not moved in this milestone.

## Changes made

### Mansion environment and audio dependencies

The following missing packages were restored at their referenced paths:

- `/Game/StarterContent/Architecture/Floor_400x400`
- `/Game/StarterContent/Architecture/Wall_400x400`
- `/Game/StarterContent/Materials/M_Basic_Floor`
- `/Game/StarterContent/Materials/M_Wood_Walnut`
- `/Game/StarterContent/Textures/T_Wood_Walnut_N`
- `/Game/StarterContent/Textures/T_Wood_Walnut_D`
- `/Game/StarterContent/Textures/T_Wood_Walnut_Mat`
- `/Game/StarterContent/Textures/T_MacroVariation`
- `/Game/ProMainMenuV3/Ms_footstep`

The restored `Wall_400x400` uses `M_Basic_Floor` as a safe neutral fallback because the expected `M_Basic_Wall` source package was absent and recreating that missing material did not complete reliably. No Mansion layout or authored lighting was redesigned.

Two imported materials with missing parents were repaired:

- `/Game/Fab/Voxel_Desk/palette` → `/Game/gltf/MaterialInstances/MI_Default_Opaque_DS`
- `/Game/Fab/Abandoned_Old_Horror_Mansion_Hotel/MI_Worn_Wooden_Wall_Panel_tkyjfgrbw_2K` → `/Game/gltf/M_Default`

Stale `/Engine/EngineMeshes/Humanoid` references were cleared by resaving:

- `/Game/HorrorBear/Mesh/SK_HorrorBear_Skeleton`
- `/Game/Mage/Mesh/SKE_Mage`

After these repairs, a clean Mansion reload no longer reported the original missing-package errors.

### Footstep MetaSound consolidation

Restoring `/Game/ProMainMenuV3/Ms_footstep` initially exposed that it was byte-identical to `/Game/Ms_footstep`, including the same MetaSound node-class GUID. Registering both packages produced a cook error.

The original was moved to the production path and the root package is now a small redirector:

- Canonical asset: `/Game/ProMainMenuV3/Ms_footstep`
- Compatibility redirector: `/Game/Ms_footstep`

Backups are stored in:

`Saved/FoundationCleanupBackup/PreConsolidation/`

### Duplicate navigation data

The authoritative Mansion contained three Recast navigation external actors. One active navigation package was retained:

`/Game/__ExternalActors__/Mansion_GameStart/E/EB/PI1K1YT4724OFL7HR5Q6VT`

The duplicate packages removed from the active World Partition set were:

- `/Game/__ExternalActors__/Mansion_GameStart/5/UW/E0IW6T1MA10RCDNSQR6ZPS`
- `/Game/__ExternalActors__/Mansion_GameStart/C/52/LMS3EA2D17FSDB91VDZI2W`

Recoverable copies are in:

`Saved/FoundationCleanupBackup/DuplicateNav/`

### Physics and collision cleanup

The zero-scale Mansion actor was repaired:

- Actor: `StaticMeshActor_UAID_047C16DF651DE39702_1628194859`
- Label: `wallHall_Box1222`
- Mesh: `/Game/Victorian_Hotel/meshes/SM_wallHall_Box919`
- Location: `X=5209.604723, Y=8042.227884, Z=473`
- Scale: `(0,0,0)` → `(1,1,1)`

The Forest also contained a flashlight item with compounded tiny scales:

- Actor: `BP_item_C_UAID_047C16DF651DEBE402_1243482624`
- Mesh: `/Game/Inventory/Models/flashlight/StaticMeshes/flashlight`
- Actor scale retained: approximately `(0.005,0.006,0.006)`
- Duplicate mesh-component scale: `(0.005,0.005,0.005)` → `(1,1,1)`
- Visual mesh collision changed to `NoCollision`
- Separate `interaction Box` overlap component was left unchanged

UE still emits one non-blocking “nearly zero” body initialization warning while constructing the shared Blueprint component template before applying the instance collision override. Removing it fully would require changing the shared `BP_item` template or rescaling its interaction hierarchy, which was outside this foundation-only scope.

### Localized Mansion duplicate

The packaged New Game test revealed that Unreal localization automatically replaced the approved `/Game/Mansion_GameStart` with `/Game/L10N/en/Mansion_GameStart`. The localized map was stale and therefore violated the single-production-Mansion decision.

The localized duplicate and its external packages were moved out of `Content` into the recoverable quarantine:

`Saved/FoundationCleanupQuarantine/LocalizedMansionDuplicate/`

This includes:

- `Mansion_GameStart.umap`
- 1,952 localized external actor packages
- 10 localized external object packages

The final packaged New Game run now loads exactly:

`/Game/Mansion_GameStart`

### Invalid sample-map quarantine

These two 141-byte files were not valid Unreal packages and caused Asset Registry cook errors even though they were not in `MapsToCook`:

- `Content/SuperPowers.umap`
- `Content/SuperPowers/SuperPowers.umap`

They were not deleted. They were moved to:

`Saved/FoundationCleanupQuarantine/InvalidSampleMaps/`

### Renderer compatibility

The project used forward shading, Pixel Depth Offset marketplace foliage, and virtual-textured lightmaps. UE 5.6 generated an invalid forward-base-pass permutation for the inherited `HillTree` material:

`ManualDepthTestEqual` was referenced but not declared.

The source `HillTree` asset was left byte-for-byte unchanged. Virtual-textured lightmaps were disabled in `Config/DefaultEngine.ini`:

`r.VirtualTexturedLightmaps=False`

Ordinary virtual-texture support, forward shading, authored materials, scene layout, and textures remain enabled. This removes the invalid permutation while changing lightmap storage rather than the visible asset design.

The invalid always-cook entry `/MetaHumanCoreTech` was removed from `Config/DefaultGame.ini`.

## Blueprint compile audit

The recursive dependency closure of the three production maps contained approximately 4,649 packages and 177 Blueprint assets.

- 173 Blueprints compiled without blocker errors.
- Previously surfaced `WBP_KeyBinder` and save-slot compile issues completed cleanly after their dependencies were loaded.
- Two inherited Menu System Pro widgets still report UE 5.6 delegate-signature errors involving `ShowDecisionDialogFromMenu`:
  - `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Ingame/WBP_IngameMenuGeneral`
  - `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/TitleScreen/WBP_TitleScreenMenu`

These two assets require a deliberate Menu System Pro API migration. They were not redesigned during this architecture milestone. They did not prevent the Development cook, package, title screen, Play selection, or New Game travel.

The final cook reported zero Blueprint/cook errors.

## Development launcher

An editor-only launcher was added:

- `Content/Python/SOTM_DevLauncher.py`
- `Content/Python/init_unreal.py`

It registers an **SOTM Dev** menu with direct editor-open actions for:

- Main Menu
- Mansion
- Forest

The launcher is Python/editor-only and does not add a runtime shipping menu or gameplay code.

## Packaging configuration

`Config/DefaultGame.ini` explicitly lists only:

- `/Game/Main_Menu_Map`
- `/Game/Mansion_GameStart`
- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

Only these three maps were passed explicitly to UAT. Their transitive dependency closure still produced approximately 2,199 cooked packages because the active menu configuration and production maps hard-reference marketplace content.

The full project C++ Development target compiled successfully. A direct cook of the original project descriptor could not load the freshly built `MetaHumanCore` editor module under this workstation’s Windows Application Control policy. To separate host policy from content validity, the package smoke test used an isolated content-only diagnostic descriptor under:

`Saved/FoundationPackageProject/`

Its `Content` points to the real project content, its config is a copy, and the production C++ module was audited as an empty bootstrap with no referenced `/Script/SOTM1` gameplay types in the three-map closure. No production source file was changed.

Two successful Development archives were produced:

- SM6 diagnostic archive: `Build/FoundationCleanupPackage/`
- SM5 renderable archive: `Build/FoundationCleanupPackage_SM5/`

The SM5 archive was required for this Intel adapter, which UE reports as supporting D3D12 SM5 but not the packaged SM6 target.

Final UAT result:

- Cook: success, 0 errors
- Stage: success
- Pak/IoStore: success
- Archive: success
- UAT: `ExitCode=0`
- Result: `BUILD SUCCESSFUL`

Primary test executable:

`Build/FoundationCleanupPackage_SM5/Windows/SOTM1_FoundationPackage.exe`

Final packaging log:

`Saved/FoundationCleanup/UAT_SM5_AuthoritativeMansion.log`

## Packaged smoke test

### Interactive Main Menu → New Game → Mansion

Passed.

Runtime evidence:

- Startup browse: `/Game/Main_Menu_Map`
- Main Menu GameMode: `BP_MenuLevelGameMode_C`
- Title screen rendered and accepted input
- Play/New Game selected interactively
- New Game browse: `/Game/Mansion_GameStart`
- Mansion GameMode: `BP_PlayLevelGameMode_C`
- Mansion world reached `Load map complete`

The first run displayed a Windows Firewall prompt. It was dismissed with **Cancel**; no network permission was granted.

### Mansion → Forest

The existing Mansion overlap trigger and its explicit CH1 target were verified in PIE in the preceding foundation milestone. In this packaged milestone, the handoff was exercised through the Development console after New Game loaded the authoritative Mansion:

`open /Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

Runtime evidence:

- Forest browse and `LoadMap` succeeded
- Forest GameMode: `BP_PlayLevelGameMode_C`
- Forest world reached `Load map complete`

This proves that the packaged reference and cooked destination are valid. It does not claim that the player physically walked into the overlap volume during the automated packaged test.

## Clean flow diagram

```mermaid
flowchart LR
    Menu["Main Menu<br/>/Game/Main_Menu_Map"]
    Mansion["Mansion<br/>/Game/Mansion_GameStart"]
    Forest["Forest<br/>/Game/MenuSystemPro/.../Levels/CH1"]
    Boss["Boss Arena<br/>TBD"]
    Ending["Ending + Credits<br/>TBD"]
    Return["Mansion Return<br/>/Game/Mansion_GameStart"]

    Menu -->|"Play / New Game — verified"| Mansion
    Mansion -->|"Existing overlap target; packaged destination verified"| Forest
    Forest -.->|"Future work"| Boss
    Boss -.->|"Future work"| Ending
    Ending -.->|"Future work"| Return
```

Dashed stages remain deliberately unimplemented.

## Remaining warnings and risks

### Runtime blockers to repair next

1. Menu System Pro prints a runtime fatal-status message before New Game:
   - “Level Name is not added in the Level Meta Data list.”
   - Travel still succeeds, but the Mansion/Forest metadata entries should be corrected before save/checkpoint work.
2. Forest media source `Isabella_Jumpscare` points to a developer-local absolute path:
   - `C:/Users/Marcus Gonzalez/Downloads/Isabella Jumpscare.mp4`
   - The Forest loads, but the media asset is invalid in the package.
3. The two Menu System Pro widget compiler errors listed above remain.

### Non-blocking cook warnings

1. The Forest flashlight shared component template still logs a nearly-zero body initialization warning before the instance `NoCollision` override is applied.
2. `r.Streaming.PoolSize=5000` overrides the lower-priority scalability value.
3. `SilencePlayLevel` loads `/Game/MenuSystemPro/ExampleContent/Levels/MoonTown` as a hidden dependency.
4. Marketplace assets cause severe cook-memory pressure. Examples observed include:
   - `SM_Iceland_Mountain_02`
   - `SM_shelf`
   - `SM_HillTree_P2`
   - `SM_car_scene_v0`
   - several textures with 1.1–2.3 GB build-memory estimates
5. CH1 reports a small number of Chaos bad-triangle warnings.
6. `/Game/AI/CruelDoll/Meshes/SK_CruelDoll` reports a zero-length normal.
7. No stable SM5 PSO pipeline-cache file is currently supplied.
8. Missing profiling DLL messages for PIX/VTune are development-machine diagnostics and are not gameplay failures.

## Backups and recoverability

Material and map backups are under:

- `Saved/FoundationCleanupBackup/`
- `Saved/FoundationCleanupQuarantine/`

No quarantined sample or localized package was destroyed.

## Recommended next milestone

The safest next milestone is **Menu Metadata and Runtime Contract Stabilization**, still before gameplay feature implementation:

1. Add the authoritative Mansion and Forest to the active Menu System Pro Level Metadata list.
2. Migrate the two delegate-signature-broken Menu System Pro widgets for UE 5.6.
3. Remove or replace the absolute-path `Isabella_Jumpscare` media reference with a project-owned source.
4. Audit the Mansion PlayerController/menu-component contract in a clean packaged session.
5. Re-run the same three-map SM5/SM6 package tests.
6. Add a small automated travel smoke test that invokes New Game and the Mansion forest overlap without requiring a full playthrough.

Gameplay systems should remain deferred until these runtime contract errors are clean.
