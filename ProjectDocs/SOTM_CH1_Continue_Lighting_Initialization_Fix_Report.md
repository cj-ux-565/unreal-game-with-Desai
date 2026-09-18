# SOTM CH1 Continue Lighting Initialization Fix Report

**Date:** 13 August 2026  
**Scope:** shared CH1 render-state initialization for New Game, Continue, direct PIE, and Development game worlds

## 1. New Game entry-path behavior

The supplied requirement confirms the production New Game route is the working baseline:

`New Game -> Mansion -> CH1`

The Mansion/menu path establishes a normal gameplay viewport before CH1 travel, so the authored CH1 PostProcessVolume produces the approved dark blue/grey Forest presentation.

## 2. Continue entry-path behavior

The broken route is:

`Main Menu -> Continue -> direct CH1`

Continue loads the saved CH1 map without first visiting Mansion. The prior render correction was Editor-PIE-only, so a standalone Development/game world loaded directly by Continue could begin CH1 without the common normalization.

## 3. Runtime state difference found

The world subsystem itself supports both `EWorldType::Game` and `EWorldType::PIE`. The actual correction did not:

- it was declared and implemented behind `#if WITH_EDITOR`;
- it explicitly required `World.WorldType == EWorldType::PIE`;
- therefore a direct Continue load in a Development game world skipped it entirely;
- the Mansion route did not expose that gap because its viewport was already initialized before CH1 travel.

Historical runtime evidence also confirms the production menu applies display CVars on Main Menu startup (`gamma`, HDR output, bloom quality, and related settings), after which map travel occurs. Lighting initialization must consequently be performed by CH1 itself rather than relying on which map ran first.

## 4. Missing/incorrect initialization callback

No separate Continue-only callback should own Forest lighting. The existing common possession boundary in `USOTMPlayerFoundationWorldSubsystem::TryBindPlayer` runs when the CH1 world, PlayerController, pawn, camera, and game viewport are all valid.

The problem was the PIE-only guard around the callback, not missing Coin/Save restoration. Continue SaveGame synchronization runs independently through `USOTMPlayerStateSubsystem` and was not changed.

## 5. Exact fix

The PIE-only `NormalizeCH1PIERenderState` was replaced with a single `NormalizeCH1RenderState` initializer:

- called from the existing CH1 player-possession path;
- active for both PIE and Development `Game` worlds;
- restricted to the normalized production `/CH1` package;
- runs once per CH1 world;
- enables normal Post Processing, Tonemapper, and Eye Adaptation presentation flags;
- selects normal Lit view;
- clears HDR/Buffer/Nanite/Lumen/Substrate/Groom/Virtual Shadow Map diagnostic visualizers;
- leaves all authored Forest post-process, exposure, bloom, fog, light, material, and Lumen values untouched;
- adds a detailed one-time log containing map, world type, controller, pawn, view target, and final show-flag state.

No Event Tick or route-specific duplicated lighting implementation was introduced.

## 6. New Game regression result

The previous verification already established three consecutive direct CH1 PIE passes with the authored dark presentation. The current change preserves the same exact flag operations and only broadens their valid world type.

The full interactive `New Game -> Mansion -> CH1` route still requires post-build replay after the currently open Unreal Editor is closed and the Editor target reloads the new module. It is not falsely reported as passed in this report revision.

## 7. Continue result

Source tracing confirms Continue and New Game now converge on the same CH1 possession initializer. The non-packaged `SOTM1 Win64 Development` target compiled successfully, proving the initializer is present in Development game code rather than being compiled out with Editor-only code.

Interactive Continue visual equivalence remains pending until the open Editor can be closed, the Editor target compiled, and the production menu route replayed. No package was created.

## 8. Direct PIE result

The previous scoped implementation passed three repeated direct CH1 PIE sessions. The new implementation performs the same operations at the same possession boundary for PIE; final post-build replay remains pending because the currently running Editor has the previous DLL loaded.

## 9. Fresh-relaunch result

Pending. A true fresh-relaunch acceptance pass requires closing all Unreal Editor processes, compiling `SOTM1Editor`, reopening the project, and using the production Continue button. The requirement is not marked complete until that visual check is performed.

## 10. Exact files modified

Lighting task files:

- `Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h`
- `ProjectDocs/SOTM_CH1_Continue_Lighting_Initialization_Fix_Report.md`

The preceding persistent-lighting report remains a separate untracked report from the immediately prior task. No CH1 map, external actor, Coin, SaveGame, HUD, player, camera, collision, portal, or AI file was modified.

## 11. Git allowlist

```text
Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp
Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h
ProjectDocs/SOTM_CH1_Continue_Lighting_Initialization_Fix_Report.md
```

If the preceding lighting report is also intended for the same commit, review and add it deliberately; do not use `git add -A`.

## 12. Suggested commit message

`fix(ch1): initialize render state for direct Continue loads`

No files were staged or committed.

