# SOTM CH1 Coin Phase 1, Lighting and Camera Final Report

Date: 2026-08-12

## Outcome

- Coin Phase 1 is integrated with the 330 existing CH1 Coin actors and the production `WBP_IngameUI`.
- The requested Editor target builds successfully.
- Fresh CH1 PIE starts in third person immediately; the active camera was measured 212.60 uu from the pawn on the first playable frame.
- The current clean PIE lighting is the dark blue/grey, fog-readable presentation shown in the supplied second video. No CH1 light or post-process asset was changed in this task.
- No package, staging, or commit was performed.

## 1. Why the Coin HUD was previously not visible

The earlier run had created native state, pickup, and HUD classes but had not completed the two asset integrations. `/Game/Blueprints/Coin` still used its legacy overlap chain and `WBP_IngameUI` did not inherit the native event-driven HUD class, so there was no production widget listening to the authoritative Coin delegate.

## 2. C++ build completion

`SOTM1Editor Win64 Development` was built with UnrealBuildTool after closing the Editor. The final build completed successfully, including UHT reflection, the SOTM1 module, and `UnrealEditor-SOTM1.dll`. Live Coding was not used and no package was created.

## 3. Coin Blueprint integration

- Reparented `/Game/Blueprints/Coin` to `/Script/SOTM1.SOTMCoinPickup`.
- Removed the legacy Blueprint overlap / sound / Destroy Actor chain so it cannot race or duplicate the native transaction.
- Preserved the existing mesh/components, placed instances, and pickup sound.
- Assigned the existing pickup sound `/Game/Sound_FX/ScreenRecording_10-16-2025_01-55-20_1` to `PickupSound`.
- Native pickup behavior validates the Player System's bound production pawn, calls the authoritative `TryCollectCoin`, disables all collision/overlap immediately, hides the actor, plays its sound, and destroys it.
- `bCollectionInProgress` plus immediate collision shutdown prevents a single runtime Coin from awarding twice.

## 4. HUD integration

- Reparented `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI` to `/Script/SOTM1.SOTMIngameUIWidget`.
- The native widget adds one minimal gold `Coins: N` TextBlock to the existing Canvas root.
- It initializes from `USOTMPlayerStateSubsystem::AvailableCoins` and listens to `OnCoinsChanged`.
- There is no Event Tick, polling, per-frame binding, or duplicated widget Coin state.
- The existing health widget hierarchy remains present and unchanged.

## 5. Coin pickup PIE results

Fresh CH1 PIE began with 330 existing Coin actors and `Coins: 0`. Five existing instances (`Coin_C_1`, `_2`, `_3`, `_5`, `_6`) were traversed through their real overlap components. Recorded results:

- authoritative log sequence: `available=1`, `2`, `3`, `4`, `5`;
- HUD sequence ended at `Coins: 5` and updated through the delegate after each overlap;
- Coin actor count changed `330 -> 325`;
- each tested actor disappeared and produced one transaction only;
- `PickupSound` was valid and the real `PlaySound2D` pickup path executed on every successful collection; the PIE audio device was active;
- no second award occurred from any tested pickup.

Evidence: `Saved/Screenshots/WindowsEditor/SOTM_CH1_Final_Coins5_ThirdPerson.png` and `Saved/Logs/SOTM1.log` (`SOTM Coin: collected ... available=1..5`).

## 6. Current versus reference lighting diagnosis

The supplied bad video shows a close/inside-character view followed by a transient white/cyan clipped frame. The desired second video shows the authored dark blue/grey forest, readable silhouettes/terrain, white lamps, and fog.

Fresh PIE after the clean Editor restart reproduced the desired dark presentation. Effective production values remained the authored CH1 values: unbound `PostProcessVolume_1`, exposure min/max `0.25`, bias `0`, bloom `0`; Directional Light intensity `3.773698`; and the existing two skylights/fog. The previously approved bloom and grounding fixes are still present.

## 7. Lighting/runtime override root cause

No persistent runtime light, Blueprint, or second post-process override was found. Controlled PIE inspection showed the saved CH1 volume and lights were active and unchanged. The extreme bright state from the bad capture was transient Editor render-state contamination, consistent with the previous controlled diagnosis; restarting the Editor cleared it. The inherited player camera does contain its own post-process settings, but disabling their blend did not correct an already contaminated white frame and was therefore not retained.

No arbitrary exposure or light-intensity edit was made. This preserves the second-video look without downgrading or flattening the production lighting.

## 8. PIE visual comparison

- Fresh first-frame interior: dark, readable house and mage.
- Forest route after five pickups: blue/grey forest depth, readable tree silhouettes, readable path highlights, fog, and controlled lamps.
- No persistent white-out was present during the final clean run.

Evidence: `Saved/Screenshots/WindowsEditor/SOTM_CH1_Final_FirstFrame_ThirdPerson.png` and `Saved/Screenshots/WindowsEditor/SOTM_CH1_Final_Coins5_ThirdPerson.png`.

## 9. Initial close-camera root cause

The correct production pawn and camera were already active; this was not possession delay or a wrong camera. The inherited Camera component was attached to `CameraBoom` without resolving to the spring-arm endpoint, and the boom's Camera-channel probe collapsed against CH1 floor/landscape from the capsule-origin pivot—even in open forest. Runtime measurement before correction was about 89 uu from the pawn and could collapse to 0 uu.

## 10. Third-person startup fix

The existing `USOTMPlayerFoundationWorldSubsystem` now performs one camera normalization when it binds the production pawn:

- reattaches the existing Camera to `USpringArmComponent::SocketName` (`SpringEndpoint`);
- disables only that malformed spring-arm camera probe;
- leaves PlayerStart, pawn collision, movement, input, CameraBoom arm length, and all global player feel unchanged.

The change is event-driven and runs once per bound pawn, with no Tick or delay.

## 11. Direct CH1 test

Passed in fresh PIE:

- production `BP_MenuSystemCharacter_C` possessed immediately;
- first playable frame measured camera distance `212.60 uu`;
- mage visible immediately in the first-frame screenshot;
- camera parent `CameraBoom`, socket `SpringEndpoint`;
- movement input worked;
- player settled in `MOVE_WALKING` mode;
- Coin HUD initialized and five pickups passed.

## 12. Mansion to CH1 test

The supplied bad video confirms the production Mansion portal opens CH1 and uses the same production pawn. The final corrected DLL was verified by direct CH1 PIE, which exercises the same CH1 spawn/bind path. A second interactive Mansion-to-CH1 transition was not performed in the final run, so that exact end-to-end transition is not claimed as newly reverified here.

## 13. Grounding-fix regression

The approved `SM_Iceland_Mountain_7.StaticMeshComponent0` `IgnoreOnlyPawn` external-actor package was not modified. The final player settled at route elevation and reported `MOVE_WALKING`, not an elevated collision hull or falling state.

## 14. Exact files/assets intentionally modified

- `Content/Blueprints/Coin.uasset`
- `Content/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI.uasset`
- `Source/SOTM1/Public/Coin/SOTMCoinPickup.h`
- `Source/SOTM1/Private/Coin/SOTMCoinPickup.cpp`
- `Source/SOTM1/Public/UI/SOTMIngameUIWidget.h`
- `Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp`
- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerBlueprintLibrary.h`
- `Source/SOTM1/Private/SOTMPlayerBlueprintLibrary.cpp`
- `Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp`
- `ProjectDocs/SOTM_CH1_Coin_Phase1_Lighting_Camera_Final_Report.md`

No production map, external actor, Coin placement, light, fog, material, collision asset, or packaged output was modified in this task.

## 15. Exact Git allowlist

Commit only the files listed in section 14. Exclude `.codex_mcp_session`, `Saved/`, `Intermediate/`, `DerivedDataCache/`, packaged output, and unrelated pre-existing worktree changes. `ProjectDocs/SOTM_Coin_System_Implementation_Plan.md` predates this completion pass and is intentionally not in this task's allowlist.

## 16. Suggested commit title

`feat(ch1): complete coin phase 1 and normalize entry camera`

## 17. Suggested commit description

`Integrate existing CH1 Coin actors with authoritative runtime currency and the production HUD, preserving their visuals and sound. Normalize the inherited player camera to the spring-arm endpoint at pawn bind so CH1 starts in third person. Verify five existing pickups, clean dark CH1 lighting, grounded traversal, Blueprint compilation, and a successful SOTM1Editor Development build.`

## 18. Client-facing update

Coin Phase 1 is now demonstrable in CH1: existing Coins award the authoritative total once, play their pickup sound, disappear, and update the normal gameplay HUD immediately. CH1 starts with the mage visible in third person, and the final clean PIE run preserves the intended dark blue/grey forest atmosphere and fog. No new Coins, save persistence, gameplay features, map placements, package, stage, or commit were added.

