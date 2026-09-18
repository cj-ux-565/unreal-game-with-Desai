# SOTM CH1 Persistent Forest Lighting Fix Report

**Date:** 13 August 2026  
**Scope:** CH1 Forest persistent PIE rendering-state correction only

## 1. BAD video observations

The supplied current recording (`SOTM1 - Unreal Editor 2026-08-13 20-47-47.mp4`) begins with normal dark presentation, but later CH1 and Mansion gameplay become clipped white/blue false-colour images. Terrain, trees, walls, the player, and highlights lose normal tonal separation. This is not a small uniform exposure increase: dark outlines remain while most lit surfaces saturate near white.

Eight sampled BAD frames measured mean luminance from 20.0 to 104.5 and near-white coverage as high as 24.2%. The high values occur during the visibly contaminated gameplay frames.

## 2. GOOD video target observations

The task text names an August 12 target, but the second video actually attached to this request is `SOTM1 - Unreal Editor 2026-08-13 20-16-20.mp4`. It was therefore used as the available GOOD reference. Its Forest retains a dark blue/grey horror tone, readable tree silhouettes, terrain separation, fog depth, and controlled emissive highlights.

Eight sampled GOOD frames measured mean luminance from 23.4 to 42.3 with near-white coverage no higher than 0.2%.

## 3. Exact recurring root cause

The root cause is persisted PIE viewport state, not a changed CH1 light or a save-game value.

Evidence:

- `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` contains a persisted `GameShowFlagsString` entry with `PostProcessing=0`.
- The August 12 Editor log records the exact diagnostic command that created the bad state: `showflag.PostProcessing 0`.
- The BAD recording's clipped raw-HDR-like appearance affects both CH1 and Mansion after travel, which rules out a Forest-only light intensity change.
- No Coin, Continue, SaveGame, camera asset, or CH1 environment actor contains a rendering-state mutation responsible for the transition.

## 4. Why the previous fix did not remain stable

The prior C++ normalization ran in `OnWorldBeginPlay` and replaced the complete viewport flag set with `FEngineShowFlags(ESFIM_Game)`. PIE can apply its persisted `GameShowFlagsString` after world BeginPlay, so the saved `PostProcessing=0` could overwrite that early repair. Replacing the whole flag set was also broader than required.

## 5. Rendering/show-flag diagnostics

- Intended gameplay view: `VMI_Lit`.
- Persisted contamination found: `PostProcessing=0`.
- Persisted debug visualization names: empty.
- Required normal state: Post Processing, Tonemapper, and Eye Adaptation enabled; HDR, Buffer, Nanite, Lumen, Substrate, Groom, and Virtual Shadow Map visualizers disabled.
- The new repair executes once after the production pawn is possessed and the PIE game viewport exists.
- It is compiled only with `WITH_EDITOR`, applies only to PIE worlds whose normalized package ends in `/CH1`, and does not run in Shipping.
- No Event Tick or repeated enforcement was introduced.

## 6. PostProcessing state

The authored CH1 post-process settings remain unchanged. The existing fixed Forest exposure and zero-bloom tuning were not numerically retuned. The permanent correction restores the ability for those authored settings to execute by enabling the Post Processing/Tonemapper presentation flags at the correct PIE lifecycle boundary.

## 7. Camera override findings

The production camera remains the existing `BP_MenuSystemCharacter` FollowCamera. Camera position, SpringArm length, startup camera normalization, post-process blend settings, movement, sprint, and jump were not modified. The camera did not cause the cross-map false-colour state.

## 8. PostProcessVolume findings

The existing CH1 unbound volume remains authoritative and unchanged. Its previously approved fixed exposure and bloom-zero settings were preserved. No additional PostProcessVolume was created, moved, or edited.

## 9. Environment light/FX findings

Controlled light/FX isolation was unnecessary after the show-flag cause was proven. Directional Light, Skylight, Exponential Height Fog, volumetric fog, `P_Fog`, `P_Fog2`, particles, materials, foliage, terrain, and emissive assets were not modified.

## 10. Exact permanent fix

`USOTMPlayerFoundationWorldSubsystem` now performs a narrowly scoped, one-time `NormalizeCH1PIERenderState` after player possession:

- only in Editor PIE;
- only for production CH1;
- enables Post Processing, Tonemapper, and Eye Adaptation;
- clears known diagnostic visualization flags and modes;
- selects normal Lit view;
- logs one confirmation per PIE world.

The old broad BeginPlay `ESFIM_Game` assignment was removed. No map or asset save is required for this fix.

## 11. Direct PIE repeated-run results

- Run 1: **Pass**. Log confirmation at 15:55:04; no clipped white/blue presentation.
- Run 2 after Stop/Play: **Pass**. Log confirmation at 15:59:04; captured `CH1_Run2.png`.
- Run 3 after Stop/Play: **Pass**. Log confirmation at 15:59:33; captured `CH1_Run3.png`.
- A representative Forest route capture at the previous fixed test location is `CH1_Forest_Run1_TargetView.png`. It shows dark blue/grey fog, readable trees and terrain, and no scene-wide white-out.

## 12. Fresh Editor relaunch result

The first Editor session was exited normally after the three passes. A second process was launched for the mandatory relaunch check, but this machine stalled during Editor window creation under severe memory/commit pressure. The relaunch visual test therefore remains **unable to verify in this run**; it is not reported as passed.

## 13. Mansion to CH1 result

Not replayed interactively in this constrained verification run. Static analysis and the BAD recording confirm the issue was viewport state carried across maps; the fix deliberately applies when the resulting CH1 PIE world binds its player, independent of whether CH1 was entered directly or through travel.

## 14. Continue to CH1 result

Not replayed interactively in this lighting-only verification run. No Continue, New Game, save-slot, or Player State code was changed by this fix. Lighting remains uncoupled from saved gameplay data.

## 15. Before/after visual evidence

- BAD reference: supplied `SOTM1 - Unreal Editor 2026-08-13 20-47-47.mp4`.
- Corrected direct PIE: `Saved/Screenshots/WindowsEditor/CH1_Run1.png`.
- Corrected repeated PIE: `Saved/Screenshots/WindowsEditor/CH1_Run2.png` and `CH1_Run3.png`.
- Corrected Forest route view: `Saved/Screenshots/WindowsEditor/CH1_Forest_Run1_TargetView.png`.
- Fresh-relaunch capture: unavailable because the second Editor launch stalled before an interactive window was created.

Screenshots under `Saved/` are generated evidence and are not Git candidates.

## 16. Coin/Save regression confirmation

No Coin actor, Coin GUID, pickup, autosave, HUD, currency value, SaveGame, Continue, New Game, or persistence source was modified. No Coin migration ran and none of the 330 Coin external actor packages was touched. Because the relaunch Editor stalled, the requested one-Coin smoke test was not executed in this run and is not claimed as passed.

## 17. Exact files modified

Lighting task files only:

- `Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h`
- `ProjectDocs/SOTM_CH1_Persistent_Forest_Lighting_Fix_Report.md`

Pre-existing Coin/Save/HUD working-tree changes were preserved and are outside this task.

## 18. Exact Git allowlist

```text
Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp
Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h
ProjectDocs/SOTM_CH1_Persistent_Forest_Lighting_Fix_Report.md
```

Do not stage `Saved/`, screenshots, logs, binaries, intermediates, CH1 external actors, or unrelated Coin/Save/HUD files.

## 19. Suggested commit title

`fix(ch1): restore persistent PIE post-processing state`

## 20. Short client update

The recurring CH1 white/washed-out presentation was traced to an Editor PIE viewport setting persisted after a diagnostic `showflag.PostProcessing 0` command. The correction now runs once at the correct post-possession PIE boundary, restores normal Lit/PostProcess rendering, and clears diagnostic visualizers without changing Forest exposure values, lights, fog, camera, coins, saves, or gameplay. The Editor target compiles successfully and three consecutive direct CH1 PIE runs pass. A final fresh-relaunch visual capture remains pending because the verification Editor stalled under machine memory pressure.

