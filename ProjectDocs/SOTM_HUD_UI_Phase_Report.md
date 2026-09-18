# SOTM HUD / UI Phase Report

Date: 2026-08-14  
Project: The Secrets of the Mansion - Chapter 1  
Scope: Client Task 7 - Main Menu, Pause Menu, Gameplay HUD, and Objective Panel

## 1. Result

The production gameplay HUD has been extended without creating a second HUD or duplicating gameplay state. The existing `WBP_IngameUI` remains the single production HUD and its native parent, `USOTMIngameUIWidget`, now supplies a coherent Chapter Status panel, authoritative coin and lives displays, future-facing presentation APIs, and Development-only preview commands.

The existing production Main Menu and Pause Menu systems were retained. No map, menu Blueprint, save asset, gameplay system, or packaged build was modified or created.

The C++ Editor target builds successfully. The affected production UI Blueprints compile successfully in commandlet validation. A standalone Development-editor runtime smoke test rendered the completed HUD at 1280x720 and logged no Blueprint or script runtime error.

Interactive PIE acceptance is still required after restarting Unreal Editor because the Editor process was already running while the native HUD DLL was rebuilt. This report does not claim that mouse-driven Main Menu/Pause interaction, live damage/death transitions, coin pickup animation, or Isabel regression passed in that stale Editor process.

## 2. Source requirements limitation

The supplied task attachment contains exactly 1006 lines and ends mid-sentence in section 31, immediately after `death animation -> Resp`. Every accessible requirement was reviewed. Any requirements that may have existed after that truncation were not available and are not claimed as reviewed or implemented.

## 3. Existing production architecture reused

- Production boot map: `/Game/Main_Menu_Map`.
- Production Main Menu: Menu System Pro using the Design Silence presentation and `BP_MenuSystemActor` configuration.
- Production Pause Menu: `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameMenu` and `WBP_IngameMenuGeneral`.
- Production Gameplay HUD: `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI`.
- Authoritative health display: existing blue `/Game/UI/WBP_HealthBar`, parented to `USOTMPlayerHealthBarWidget`.
- Authoritative runtime player state: `USOTMPlayerStateSubsystem` and `USOTMPlayerVitalComponent`.
- Authoritative coin state: `USOTMPlayerStateSubsystem::OnCoinsChanged`.
- Authoritative lives state: `USOTMPlayerStateSubsystem::OnLivesChanged`.

The unrelated legacy `/Game/UI/PauseMenu` sample route was not connected or modified.

## 4. Gameplay HUD implementation

The native parent adds presentation-only children to the existing HUD root canvas:

- Top-left dark translucent `CHAPTER STATUS` panel.
- Current objective row, hidden until an objective is supplied.
- Always-visible authoritative `COINS COLLECTED` value.
- Required-coin progress row, hidden until requested.
- Upgrade progress bar and detail, hidden until requested.
- Reusable mission-task rows, hidden until populated.
- Bottom-left authoritative `LIVES current / maximum` display.
- Short visual lives pulse when a player death starts.
- Top-center boss name and health bar, hidden until requested.

The existing blue health bar was not redesigned or replaced.

The layout uses canvas anchors, constrained widths, wrapping text, and scalable UMG containers. The captured 1280x720 result contains no visible clipping or overlap between the lives display and the existing health bar.

## 5. Authoritative data integration

The HUD binds once to these existing subsystem delegates and removes every binding in `NativeDestruct`:

- `OnCoinsChanged`
- `OnLivesChanged`
- `OnPlayerDeathStarted`
- `OnPlayerRespawned`
- `OnGameOver`
- `OnInputLocksChanged`

No Event Tick, recurring actor search, or duplicate gameplay variable was added.

The HUD is suppressed during these existing input-lock states:

- Death
- Respawn
- Cinematic
- Jump Scare
- Game Over

It restores itself when those locks clear. Player health, coins, lives, objectives, and boss health remain owned by their gameplay systems.

## 6. Public presentation API

The following Blueprint-callable functions are available for later gameplay milestones:

- `SetCurrentObjective` / `ClearCurrentObjective`
- `SetRequiredCoins` / `ClearRequiredCoins`
- `SetUpgradeProgress` / `ClearUpgradeProgress`
- `SetBossProgress` / `ClearBossProgress`
- `SetMissionTask` / `RemoveMissionTask` / `ClearMissionTasks`
- `SetHUDPresentationVisible`

These functions update presentation only. They do not award coins, complete objectives, unlock upgrades, damage the boss, or alter save data.

## 7. Default visibility and future systems

- Coins and lives are visible because their authoritative systems already exist.
- Objective text, required coins, upgrade progress, mission tasks, and boss health are hidden by default.
- Future systems can reveal those elements by calling the public presentation API.
- No placeholder objective, upgrade, key, boss, or mission state is written into production gameplay.

## 8. Development-only preview tools

The following console commands are compiled only for non-Shipping builds:

- `SOTM.HUD.TestObjective`
- `SOTM.HUD.TestUpgradeProgress`
- `SOTM.HUD.TestBossProgress`
- `SOTM.HUD.ClearPreview`
- `SOTM.HUD.CapturePreview1080`
- `SOTM.HUD.CapturePreview720`

Preview commands populate only HUD presentation. Runtime logs explicitly report `gameplay state unchanged`. They do not persist objective, upgrade, required-coin, or boss data.

## 9. Main Menu and Pause Menu findings

The existing production menu framework already provides the required routing structure:

- Main Menu: Play/New Game, Options, and Quit.
- Pause Menu: Resume, Options, and Main Menu routing.

Those production Blueprints compiled successfully and were deliberately reused rather than replaced. No optional Restart action was added because it was not required to repair a confirmed production issue and could alter checkpoint/retry behavior.

Interactive button acceptance remains a manual PIE check after the Editor restart. No claim is made here that all button paths were clicked during the commandlet/offscreen run.

## 10. Menu metadata and save-safety audit

Read-only inspection found current production references in the menu configuration data, including:

- `/Game/Main_Menu_Map`
- `/Game/Mansion_GameStart`
- Chapter/menu metadata references including `DA_CH1` and `DA_Menu`

Existing save data strings also referenced current production Mansion/CH1 paths. No save was deleted, reset, migrated, or overwritten.

The Blueprint configuration contains private/internal generated properties that the available commandlet Python reflection could not safely enumerate. Therefore no speculative binary asset edit was made. Any remaining warning about an old sample metadata entry must be reproduced interactively before changing the data asset.

## 11. Verification completed

### Build

- Target: `SOTM1Editor Win64 Development`.
- Result: successful UHT, compile, link, and target build.
- New C++ compile errors: none.

### Production UI Blueprint compilation

The following production UI assets were compiled through an Unreal commandlet with no `LogBlueprint` or `LogK2Compiler` errors:

- `WBP_IngameUI`
- `WBP_HealthBar`
- `WBP_TitleScreenMenu`
- Production settings widgets
- `WBP_IngameMenu`
- `WBP_IngameMenuGeneral`

### Runtime HUD smoke test

- Runtime: standalone Development-editor process, not a packaged build.
- Map: `/Game/Mansion_GameStart`.
- Captured resolution: 1280x720.
- HUD preview: objective, coins, required coins, upgrade progress, mission tasks, boss bar, lives, and the existing blue health bar rendered together.
- Log result: preview commands executed; no `Blueprint Runtime Error`, `LogBlueprint: Error`, or `LogScript: Error` was found in the capture log.
- Screenshot: `Saved/Screenshots/WindowsEditor/SOTM_HUD_Preview.png` (generated evidence; intentionally excluded from Git).

### Not yet claimed as verified

- Interactive PIE at 1920x1080.
- Main Menu Play/Options/Quit mouse interaction.
- Pause Resume/Options/Main Menu mouse interaction and repeated open/close behavior.
- Live damage, death, respawn, Game Over, and health/lives transitions with the newly loaded HUD module.
- Live coin pickup and persistence regression.
- Mansion-to-Forest traversal during this UI pass.
- Isabel patrol/detection/chase/attack/jump-scare regression.

These require restarting the already-open Unreal Editor so it loads the rebuilt native module, followed by the manual route in section 12.

## 12. Required manual PIE acceptance route

1. Close Unreal Editor normally and reopen `SOTM1.uproject`.
2. Open `/Game/Main_Menu_Map` and start PIE at 1920x1080.
3. Verify Play/New Game opens Mansion, Options opens and closes safely, and Quit is present. Do not click Quit until the other checks are complete.
4. In Mansion, verify the existing blue health bar, coins, and lives are visible with no overlap.
5. Open the console and run `SOTM.HUD.TestObjective`, `SOTM.HUD.TestUpgradeProgress`, and `SOTM.HUD.TestBossProgress`.
6. Confirm objective wrapping, task rows, upgrade progress, and boss bar presentation. Run `SOTM.HUD.ClearPreview` and confirm future-only elements hide.
7. Use the existing Development Player System Debug Panel (F10) to apply damage, heal, kill, respawn, and retry. Confirm the HUD hides during death/respawn and returns afterward; lives decrement exactly once.
8. Collect one production coin and confirm the coin count updates without duplicate HUD widgets.
9. Press Escape repeatedly and verify Resume, Options, and Main Menu paths, correct input mode, and no stacked pause widgets.
10. Traverse Mansion to Forest and verify the same HUD remains coherent and authoritative.
11. Perform the short Isabel regression route: patrol, detection, chase, normal attack/health reduction, catch jump scare, death animation, and respawn.
12. Repeat the HUD layout check using a smaller 16:9 PIE viewport, preferably 1280x720.

## 13. Packaging and source-control actions

- No package or Development archive was created.
- No Shipping build was run.
- Nothing was staged.
- Nothing was committed.

## 14. Exact intentionally modified files

- `Source/SOTM1/Public/UI/SOTMIngameUIWidget.h`
- `Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp`
- `ProjectDocs/SOTM_HUD_UI_Phase_Report.md`

No `.uasset`, `.umap`, config, save, or generated file was intentionally modified.

## 15. Exact Git allowlist

Only these files are recommended for the technical HUD/UI commit after manual PIE acceptance:

```text
Source/SOTM1/Public/UI/SOTMIngameUIWidget.h
Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp
ProjectDocs/SOTM_HUD_UI_Phase_Report.md
```

Suggested commit title:

```text
Complete production HUD presentation foundation
```

Suggested description:

```text
Extend the existing production HUD with authoritative lives and coin displays,
future objective/upgrade/task/boss presentation APIs, gameplay-state visibility,
and non-Shipping preview tools while preserving existing health and menu systems.
```

## 16. Remaining risks and follow-up

- The final part of the supplied task document was unavailable because the attachment is truncated.
- A running Editor must be restarted before interactive PIE can exercise this native change.
- Exact 1920x1080 interactive layout and mouse navigation remain manual acceptance items.
- Any stale menu-metadata warning needs a reproducible asset/property name before a safe binary data-asset edit.
- Objective, upgrade, key, and boss gameplay systems do not yet exist; the HUD intentionally exposes presentation contracts without inventing their logic.
- Final UI sound, animation, and art polish should follow actual client feedback after the complete interactive route is captured.

