# SOTM Client Demo — Phase 4 Chest, Key, Gate and Demo Ending Report

Date: 2026-08-19  
Engine: Unreal Engine 5.6  
Target: `SOTM1Editor Win64 Development`

## 1. Phase 4 architecture

Phase 4 extends the existing Phase 1–3 architecture. It does not create a second coin, objective, upgrade, input, save, or HUD system.

- `USOTMObjectiveSubsystem` remains the authority for the Chapter 1 objective chain and all chest/gate prerequisite decisions.
- `USOTMPlayerStateSubsystem` remains the persistent story-state authority and performs transactional autosaves.
- `USOTMDemoPhase4WorldSubsystem` is a CH1-only runtime coordinator. It locates the approved placed art, creates transient overlap anchors, binds the existing Interact input, and owns presentation timers.
- `ASOTMPhase4Interactable` is a transient overlap-only actor. It contains no progression state.
- `USOTMIngameUIWidget` remains the single gameplay HUD and responds to delegates rather than polling.
- `USOTMDemoCompleteWidget` is a native full-screen presentation. It contains no progression authority.

No Event Tick was added. State changes use overlap events, Enhanced Input, delegates, one-shot timers, and montage-independent presentation timers.

## 2. Chest asset and location

The existing production chest was reused:

- Mesh: `/Game/Chest_Keys/chest.chest`
- Placed in CH1 at approximately `X=9450.011, Y=56610.008, Z=-2138.084`
- Rotation: approximately `Yaw=60`
- Scale: `1.0, 1.0, 1.5`

No Forest map or external-actor asset was edited. The runtime subsystem discovers this placed actor and adds only transient interaction/presentation support.

The source chest is a single static mesh with no usable lid animation. The safe presentation therefore uses a short lift/tilt of the existing chest and reveals the existing Gate Key mesh. No replacement asset was introduced.

## 3. Chest interaction implementation

The subsystem binds the existing production `IA_Interact` action to the production player's `UEnhancedInputComponent`. A transient overlap anchor around the placed chest emits enter/exit events and shows:

- `[E] OPEN CHEST` when `FindChest` is active;
- `COMPLETE PREVIOUS OBJECTIVES` when prerequisites are incomplete;
- `CHEST OPENED` after completion.

The interaction calls `USOTMObjectiveSubsystem::TryOpenPhase4Chest()`. It validates all-coins completion, Speed Boost ownership, and one-shot chest state. On success, chest-open and Gate Key ownership are committed atomically and autosaved. A second interaction returns `AlreadyCompleted` and cannot award another key.

## 4. Objective progression architecture

The authoritative Chapter 1 chain is now:

1. `CollectAllForestCoins`
2. `UnlockSpeedBoost`
3. `FindChest`
4. `ObtainGateKey`
5. `ReachGate`
6. `DemoComplete`

The active/completed/locked states are derived from persistent coin, Speed Boost, chest, key, gate, and completion state. Widgets do not own objective state.

The Phase 3 handoff is enforced: `FindChest` becomes active only when lifetime coin completion is valid and Speed Boost is unlocked. Spending 250 coins does not invalidate the 330-lifetime-coin prerequisite.

After chest success, `FindChest` and `ObtainGateKey` are complete and `ReachGate` becomes active immediately through delegates.

## 5. Gate Key persistence

The existing save payload was extended backward-compatibly to schema version 4 by appending a namespaced record:

`SOTM_PHASE4|ChestOpened|HasGateKey|GateUnlocked|DemoCompleted`

This avoids modifying the inherited Blueprint SaveGame class or duplicating save objects. Older saves default Phase 4 flags to false. New Game resets all four flags. Each successful story transition rolls back its in-memory value if the save fails.

Autosaves occur after:

- chest open + key award;
- gate unlock;
- demo completion.

## 6. Gate Key HUD

The existing production HUD now displays event-driven Gate Key state:

- `GATE KEY / NOT ACQUIRED`
- `GATE KEY / ACQUIRED`

Successful chest interaction also shows a readable gothic notification:

- `GATE KEY ACQUIRED`
- `OBJECTIVE UPDATED - REACH THE GATE`

No duplicate HUD was created.

## 7. Gate requirements

`TryUnlockPhase4Gate()` validates all three requirements:

1. lifetime coin objective completed;
2. Speed Boost unlocked;
3. Gate Key acquired.

It deliberately does not require 330 spendable coins. The accepted state `Lifetime=330, Available=80, Speed Boost owned` is valid.

Locked feedback lists player-facing requirement status and does not expose internal property names.

## 8. Gate implementation

The existing production gate mesh was reused:

- `/Game/Fab/Main_gate_entrance/main_gate_entrance/StaticMeshes/main_gate_entrance.main_gate_entrance`

The runtime subsystem chooses the placed gate segment nearest the approved chest/final progression area and attaches a transient interaction anchor. No random replacement door was created and no map asset was resaved.

Successful interaction commits `GateUnlocked`, autosaves, clears the prompt, disables gate collision, and starts the one-shot opening presentation. Duplicate interaction cannot restart it.

## 9. Gate animation and presentation

The selected gate segment lifts smoothly by 520 Unreal units using a short ease-in/out timer. Its component becomes transiently Movable before animation and collision is disabled at unlock. This removed the initial runtime mobility-warning spam without altering the saved map.

The presentation shows:

- `GATE UNLOCKED`
- `FINAL DEMO GOAL ACHIEVED`

After a controlled delay, the demo completes. No boss map or unfinished boss content is loaded.

## 10. Demo Complete architecture

Gate completion calls the Objective System, persists `DemoCompleted`, then creates `USOTMDemoCompleteWidget`. A Player System custom input lock is acquired while the completion screen is visible.

The completion UI is a dark gothic full-screen presentation with purple accents, gold hierarchy, and the approved text:

- `THE SECRETS OF THE MANSION`
- `DEMO COMPLETE`
- `THE STORY CONTINUES...`
- `BUY THE FULL GAME`
- `COMING SOON`
- `RETURN TO MAIN MENU`

An initial native-UMG lifecycle defect caused an empty placeholder root to render. The tree is now created in `NativeOnInitialized`, before Slate rebuilds it. Both 1280×720 and 1920×1080 captures confirm the corrected screen.

## 11. Buy Full Game UI

No store URL or release date was supplied, so none was invented. The button is presentation-only and changes its status to `STORE LINK COMING SOON`. Actual store integration remains dependent on the client supplying a production URL.

## 12. Continue after completion behavior

The completed state is slot-persistent. When CH1 initializes with completed Phase 4 state, the chest and gate presentation are restored and the Demo Complete presentation is scheduled again. The player is never sent into boss content.

Automated save/load verified all four Phase 4 flags in a completed slot. A fresh-process click through the actual Main Menu Continue UI was not manually replayed during this focused implementation session and remains a final human acceptance check.

## 13. New Game reset

`ResetRuntimeStateForNewGame()` clears:

- chest opened;
- Gate Key ownership;
- gate unlocked;
- demo completed.

The final data acceptance reported `newGameReset=1`.

## 14. Save-slot isolation

Development data acceptance used two independent temporary slots:

- Slot A: `Available=80`, `Lifetime=330`, Speed Boost owned, chest opened, key acquired, gate unlocked, demo completed.
- Slot B: zero coin/upgrade progression and all Phase 4 flags false.

Load results reported `stateA=1` and `stateB=1`, with no state leakage. The project's four production slots continue to use the existing Menu System Pro manager.

## 15. Death and Key persistence

Phase 4 flags are not modified by Player System death, respawn, or Game Over code. They are written into the same active story save and restored independently of lives/health. The accepted Cousin instant-death path was not changed.

Slot reload proved Key/chest ownership persistence. A live Cousin catch after acquiring the key was not manually replayed in this session; it remains a focused human regression check.

## 16. Forest Health presentation decision

The underlying Player Vital, health, death, lives, respawn, and future boss-health architecture remains intact. During normal CH1 demo play, widgets whose production name identifies them as Health presentation are hidden so the HUD does not imply multi-hit Forest combat. Lives, coins, objectives, Speed Boost, and Gate Key remain visible.

No health logic was removed and Cousin catch remains an instant-death route.

## 17. Phase 1 regression

Read-only production validation loaded:

- `/Game/Main_Menu_Map`
- `/Game/Mansion_GameStart`
- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

The final Phase 4 acceptance route also returned successfully to `/Game/Main_Menu_Map` in 0.79 seconds through the production Player State route. No Phase 1 source or asset was modified.

The complete Main Menu → Mansion cinematic → knockout → CH1 route was not manually replayed from the first menu click in this session; its previously accepted implementation was not rebuilt.

## 18. Phase 2 regression

The following production Blueprints compiled during read-only validation:

- `BP_MenuSystemCharacter`
- `Coin`
- `WBP_IngameUI`
- `WBP_HealthBar`
- `BP_ForestPortal`
- `BP_AI`

The Phase 2 coin authority and Cousin instant-death implementation were not changed. Full live patrol/chase/jump-scare/death replay was not repeated in this session.

## 19. Phase 3 regression

Phase 4 consumes the existing persistent Speed Boost ownership and does not change Timmy, purchase cost, movement multiplier, duration, cooldown, or Q binding. Rendered acceptance showed `SPEED BOOST [Q] READY` after Phase 4 state setup. Data acceptance verified lifetime/available separation at `330/80` and Speed Boost persistence.

A complete live READY → ACTIVE → COOLDOWN → READY cycle was not replayed in this focused Phase 4 session.

## 20. Full demo playthrough result

The Phase 4 segment was executed as a rendered production-CH1 route:

`Find Chest → Open Chest → Key acquired → Reach Gate → Unlock Gate → Demo Complete → Main Menu`

All logged Phase 4 checkpoints passed. The earlier Phase 1–3 route was validated by affected Blueprint compilation, production map loading, and the previously accepted Phase 1–3 reports. One uninterrupted human-controlled Main Menu-to-Demo-Complete playthrough was not recorded during this session, so final client acceptance should still include that playthrough before packaging.

## 21. Resolution tests

Rendered evidence passed at:

- 1280×720: objective panel, prompt, key state, notifications, and Demo Complete layout visible without clipping.
- 1920×1080: the same elements remained readable and unclipped.

Evidence locations (generated; do not commit unless intentionally desired):

- `Saved/Screenshots/WindowsEditor/Phase4_1280x720/`
- `Saved/Screenshots/WindowsEditor/Phase4/` (final 1920×1080 pass)

Captured images cover Find Chest/prompt, Key acquired, Reach Gate/gate progression, and Demo Complete/Buy Full Game. Separate images for every requested intermediate animation frame, locked-combination panel, Continue screen, and Main Menu return were not captured.

## 22. Errors found and fixed

Fixed during Phase 4:

- native Demo Complete Widget existed but rendered empty because its tree was built too late;
- forced UI focus targeted a non-focusable root and logged an input-mode error;
- active objective was duplicated in the objective list;
- chest/key/gate presentation attempted to move Static components and produced repeated mobility warnings.

Final Editor target build: **Succeeded**.  
Final read-only production validation: **0 errors, 2 warnings**.  
Final Phase 4 data acceptance:

`PASS=1 requirements=1 oneShot=1 saveA=1 loadA=1 stateA=1 saveB=1 loadB=1 stateB=1 newGameReset=1`

Final rendered acceptance:

- `CHEST pass=1`
- `COMPLETE pass=1`
- production return browse/load to `/Game/Main_Menu_Map` succeeded.

Known pre-existing warnings/issues not caused by Phase 4:

- CH1 Recast NavMesh serialized `maxTiles` mismatch warning;
- Crowd Manager warning while loading a map without an active RecastNavMesh instance;
- missing absolute media path `C:/Users/Marcus Gonzalez/Downloads/Isabella Jumpscare.mp4`.

## 23. Exact files intentionally modified or created for Phase 4

Modified:

- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/Objective/SOTMObjectiveSubsystem.h`
- `Source/SOTM1/Private/Objective/SOTMObjectiveSubsystem.cpp`
- `Source/SOTM1/Public/UI/SOTMIngameUIWidget.h`
- `Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp`

Created:

- `Source/SOTM1/Public/Demo/SOTMPhase4Types.h`
- `Source/SOTM1/Public/Demo/SOTMPhase4Interactable.h`
- `Source/SOTM1/Private/Demo/SOTMPhase4Interactable.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase4WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase4WorldSubsystem.cpp`
- `Source/SOTM1/Public/UI/SOTMDemoCompleteWidget.h`
- `Source/SOTM1/Private/UI/SOTMDemoCompleteWidget.cpp`
- `ProjectDocs/SOTM_Demo_Phase4_Chest_Key_Gate_Ending_Report.md`

No `.uasset`, `.umap`, Config, Forest lighting, Build.cs, package, or archive file was intentionally modified for Phase 4.

Pre-existing unstaged Phase 2 files and reports were preserved and are not part of this Phase 4 change list.

## 24. Exact Git allowlist

Stage only the 14 paths listed in Section 23 for a Phase 4 commit. Do not stage `Saved/`, `Intermediate/`, screenshots, logs, temporary test saves, or the pre-existing Phase 2 working-tree changes/reports.

## 25. Suggested commit title

`feat: complete Phase 4 chest key gate and demo ending`

## 26. Suggested commit description

`Extend the authoritative objective and player-state systems with persistent Phase 4 progression, reuse the production CH1 chest and gate through event-driven interaction, add Gate Key HUD feedback and a safe Demo Complete presentation, and include Development-only acceptance coverage for prerequisites, one-shot behavior, slot isolation, reset, responsive UI, and return to the Main Menu.`

## 27. Client-facing progress update

Phase 4 is implemented on top of the accepted demo architecture. After all 330 coins are collected and Speed Boost is owned, the player can find and open the production chest, obtain a persistent Gate Key, follow the updated objective to the production gate, unlock it once, and reach a polished Demo Complete / Buy the Full Game screen without entering unfinished boss content. The system autosaves key progression, remains isolated per save slot, resets correctly for New Game, preserves the future health system, and returns safely to the Main Menu. Editor builds, production Blueprint/map validation, negative prerequisite checks, one-shot checks, persistence, slot isolation, and 720p/1080p rendered checks passed. A final uninterrupted human-controlled full-demo recording is still recommended before packaging.
