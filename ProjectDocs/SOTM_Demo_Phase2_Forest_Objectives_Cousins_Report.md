# SOTM Client Demo Phase 2 — Forest Objectives, HUD, and Cousin AI

Date: 17 August 2026  
Project: The Secrets of the Mansion — Chapter 1  
Production Forest: `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

## Executive result

Phase 2 implements the first Forest gameplay loop without advancing into Phase 3. The approved HUD direction is functional and data-driven; the Forest Coin objective uses persistent lifetime/unique collection state; one reusable Cruel Doll cousin architecture patrols, detects, chases, catches, and performs a gated lethal jump scare; and death, lives, respawn, Game Over, Retry, and Coin persistence remain owned by the existing Player System.

The Editor target builds successfully. Selected production Blueprints compile with zero errors and the Main Menu, Mansion, and CH1 maps load in read-only validation. Focused runtime acceptance passed the natural five-Coin route, natural AI detection/chase/catch, one-life death and respawn, persistence reload, duplicate-catch rejection, Game Over/Retry, and the authoritative 329-to-330 completion boundary.

No package was produced. Nothing was staged or committed. No Forest map, lighting, exposure, post-process, fog, sky, material, or renderer asset was modified.

## 1. Approved HUD reference used

`Phase2HUD.png` was used as the approved visual specification. The implementation follows its hierarchy and visual language: a dark translucent gothic objective panel at upper left, gold objective/Coin accents, purple locked-progression accents, red danger messaging, health/lives presentation, and protected screen-center gameplay space.

The PNG was not pasted into the viewport. Every displayed value is a real UMG text/progress element driven by production state.

## 2. HUD implementation architecture

The existing production native parent `USOTMIngameUIWidget` was extended, so the existing `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI` remains the production widget. The widget builds the Phase 2 presentation on its existing Canvas root and binds to authoritative delegates.

Implemented presentation:

- left objective panel with the active `Collect All Coins` objective and live `N / 330` progress;
- dimmed, locked future objectives: Find the Chest, Obtain the Gate Key, and Reach the Gate;
- authoritative top-right Coin total;
- existing Player System health bar and lives integration;
- `SPEED BOOST / LOCKED` presentation only;
- `GATE KEY / NOT ACQUIRED` presentation only;
- a brief right-side `COUSIN SPOTTED!` warning;
- HUD suppression/restoration during the existing death and Game Over flows.

Updates are delegate-driven. No Event Tick polling was added for Coins, objectives, health, lives, or warnings.

## 3. Objective System architecture

`USOTMObjectiveSubsystem` is a Game Instance subsystem and is the authority for Phase 2 objective presentation. Its objective record contains:

- objective ID;
- display name;
- current progress;
- required progress;
- state (`Locked`, `Active`, or `Completed`).

The production objective is:

- ID: `CollectAllForestCoins`
- Display name: `Collect All Coins`
- Required progress: `330`

The subsystem activates this objective only in CH1 and binds to the existing Player State Coin-change event. The UI observes the Objective subsystem; it does not own or calculate gameplay progress.

## 4. Exact verified CH1 Coin count

A read-only audit of the current production CH1 map found:

- placed Coin actors: **330**;
- valid persistent Coin IDs: **330**;
- unique persistent Coin IDs: **330**;
- duplicate IDs: **0**;
- invalid IDs: **0**;
- map dirtied by the audit: **no**.

No Coins were spawned, deleted, or moved to force this number.

## 5. Why lifetime/unique collection is authoritative

Coins are both collectible progress and future spendable currency. A current balance can decrease at Timmy's future Upgrade Station, so `AvailableCoins >= 330` would incorrectly undo an already completed objective after spending.

The objective therefore derives progress from the greater authoritative persistent value represented by `LifetimeCoinsCollected` and the persistent unique collected-Coin ID set, clamped to 330. Completion remains true after future spending because it represents collection history, not current purchasing power.

## 6. Collect All Coins implementation

The existing successful Coin path remains authoritative:

`placed Coin overlap -> TryCollectCoin -> Player State updates unique/lifetime state -> existing autosave -> OnCoinsChanged -> Objective subsystem -> HUD delegate refresh`

The Phase 2 code does not award Coins independently and does not maintain a second widget-only count. At 330 authoritative unique/lifetime Coins, the objective changes to `Completed` and displays the completed state. Speed Boost remains locked and no later objective is activated.

## 7. Persistence behavior

Focused acceptance produced the following verified results:

- five real placed Coin actor overlap events increased both the Coin HUD and objective from 0 to 5;
- each successful pickup used the existing autosave path;
- after lethal cousin catch and respawn, Coins, lifetime count, unique-ID count, and objective progress remained at 5;
- an explicit production Slot 1 reload restored Coins/lifetime/unique/objective progress and the decremented lives state;
- the 329-to-330 completion test used authoritative `TryCollectCoin` calls for setup and a real placed Coin overlap for the final pickup;
- at 329 the objective was Active; the final valid unique pickup changed it to 330/330 and Completed;
- test save files were backed up before validation and restored byte-for-byte afterward (23 files, zero hash differences).

Limitation: the persistence run exercised the production Slot 1 load API directly. A fresh manual relaunch followed by clicking the production Continue UI was not repeated in this Phase 2 pass. The underlying save/load result passed, but the exact user click route remains a manual acceptance item.

## 8. Cousin assets used

The cousin implementation reuses existing project content:

- skeletal mesh: `/Game/AI/CruelDoll/Meshes/SK_CruelDoll`;
- Animation Blueprint: `/Game/AI/CruelDoll/Meshes/SKL_CruelDoll_AnimBlueprint`;
- catch montage: `/Game/AI/AS_CruelDoll_Attack03_Montage`;
- scream: `/Game/AI/Nightmare_scream_jumpscare_SFX`.

No marketplace download or new imported art/audio was added. The main Isabella is not used as the normal cousin pawn.

## 9. Cousin AI architecture

All production cousins share `ASOTMCousinCharacter` and `ASOTMCousinAIController`. The controller is AI-Perception and timer driven, with the states:

`Idle -> Patrol -> Chase -> Catch -> Disabled/death cleanup`

If sight is lost beyond the grace period, the controller clears its target and returns to patrol. Navigation failure queues a delayed patrol retry rather than recursively calling `MoveTo`, preventing a stack overflow on an invalid or unavailable path.

At CH1 initialization, the Phase 2 World subsystem disables the 33 legacy normal-enemy stacks for that runtime session and spawns eight transient production cousins at sampled legacy positions. This avoids editing the production map and avoids running two enemy architectures simultaneously. The actors are transient runtime instances; no CH1 asset was resaved.

## 10. Cousin variants and presentation

Eight cousins share the same behavior and use three restrained presentation variants. Variant differences are limited to safe scale and movement-speed multipliers (slower stalking, faster aggressive, and smaller/default-tempo presentation). They do not duplicate AI or gameplay logic.

The variants retain the existing Cruel Doll locomotion/attack asset family. No custom cousin content was fabricated.

## 11. Detection behavior

Detection uses AI Perception sight rather than per-frame actor searches:

- sight radius: 1250 units;
- lose-sight radius: 1550 units;
- peripheral half-angle: 65 degrees;
- lost-target grace: 3 seconds;
- targets must be the valid living production player;
- dead, respawning, Game Over, or invalid pawns are rejected.

The runtime test observed a patrolling cousin perceive the production player and transition naturally to Chase. The warning displayed once for 1.8 seconds during the encounter.

## 12. Chase behavior

Chase uses navigation `MoveToActor`, a base chase speed of 390, and variant speed multipliers. Catch requires both range and line of sight. Movement stops when Catch is accepted.

Patrol, detection, and natural chase-to-catch were runtime verified. The dedicated escape/search route was not visually exercised in this pass; the timer-driven lost-sight grace and return-to-patrol code compiled and is present, but this specific behavior remains a manual acceptance test.

## 13. Catch behavior

A catch is requested only when the living player is within the 180-unit catch range and the controller has valid line of sight. The World subsystem owns a single global encounter gate, so nearby cousins cannot start overlapping catches.

On acceptance it:

1. suspends all cousin pursuit;
2. acquires the Player System `JumpScare` input lock;
3. stops movement;
4. creates a transient close camera and frames the catching cousin;
5. plays the Cruel Doll attack montage and scream;
6. applies one authoritative lethal hit at the timed impact;
7. releases cinematic state through Player System death/respawn/Game Over callbacks.

There is no cousin multi-hit combat loop.

## 14. Jump-scare implementation

The catch presentation reuses the proven architectural principles of the existing Isabella jump scare while remaining cousin-specific. Its camera is transient and removed on completion or world cleanup. Input locking uses the existing Player State lock reason, the normal HUD is suppressed during the death presentation, and the camera is restored on respawn or Game Over.

The impact is timed 0.68 seconds after catch start to align with the existing attack montage presentation. Runtime logs and screenshots verified the montage/cinematic state and impact sequence. Audio playback was invoked successfully by the runtime path; audible quality was not independently evaluated by the available automated tools and requires a human listening pass.

No persistent Forest post-process or lighting state is changed.

## 15. Instant-death integration

At impact, lethal damage is sent through `USOTMPlayerBlueprintLibrary::ApplyPlayerDamage`. The cousin does not write health, lives, death, respawn, checkpoint, or Game Over variables directly. The existing Player Vital component and Player State subsystem remain responsible for the result.

The runtime acceptance result was one lethal damage request, health reaching the normal death path, lives changing from 5 to 4 once, followed by the existing respawn with restored living state.

## 16. Duplicate-catch protection

Protection exists at both the controller/actor encounter state and the Phase 2 World subsystem's authoritative global catch gate. During final-life testing, the first catch request was accepted and a simultaneous second request was rejected. Only one lethal impact occurred, only one death was raised, and no camera or input lock was stacked.

## 17. Lives, death, respawn, and Game Over results

Verified:

- normal catch: lives 5 -> 4 exactly once;
- Player System death became active;
- cousins suspended during death/respawn;
- existing respawn completed and restored a living player;
- health restoration remained owned by the Player System;
- Coin/objective progress remained at 5/330 after respawn;
- final-life test: lives 1 -> 0, dead true, Game Over true;
- catch state/camera was cleared at Game Over;
- existing `RetryFromGameOver` returned true;
- Retry restored lives to 5, dead false, Game Over false, and cousins to patrol.

The requested second natural 4-to-3 catch was not separately replayed. Exact-one decrement was verified on a normal catch and again on the final-life path, with duplicate-catch rejection tested explicitly.

## 18. Phase 1 regression result

Read-only production validation loaded all three approved maps:

- `/Game/Main_Menu_Map`;
- `/Game/Mansion_GameStart`;
- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`.

The relevant production Blueprints compiled, no maps became dirty, and Phase 2 changes are CH1-gated and do not modify Phase 1 Main Menu/Mansion assets or logic.

Limitation: the complete interactive `Main Menu -> four slots -> New Game -> Mansion locked cinematic -> Timmy -> Isabella -> dragging -> CH1` route was not replayed end-to-end during this Phase 2 pass. It passed the preceding Phase 1 work; this pass provides structural/map/compile regression coverage, not a new complete click-through claim.

## 19. HUD 1920x1080 result

An actual 1920x1080 runtime screenshot was captured and visually inspected. Objective progress, Coin value, health/lives, Speed Boost locked state, Gate Key state, and gameplay center were readable with no major clipping.

The Development capture contained an Unreal `Preparing Shaders` overlay near the upper-left objective area. This overlay is not part of the HUD; a clean human screenshot after shader preparation is recommended for client presentation.

## 20. HUD 1280x720 result

Actual 1280x720 runtime screenshots were captured and inspected. The layout remained readable, the warning did not obscure the center, and no major clipping occurred. At the 330/330 completed state, one objective line wrapped but remained readable. This is acceptable functionally; a final UI polish pass may reduce that wrap if desired without changing Phase 2 gameplay.

Generated verification screenshots are stored under `Saved/Screenshots/WindowsEditor/Phase2/` and are intentionally excluded from Git.

## 21. Errors found and fixed

Fixed during Phase 2:

- patrol path failure could synchronously call another patrol request and recurse until stack overflow; it now schedules bounded delayed retries;
- legacy normal-enemy stacks would otherwise coexist with cousins; the 33 legacy instances are disabled only at CH1 runtime while eight transient cousins provide the production Phase 2 behavior;
- duplicate catch/death/camera/input-lock risk was resolved through one global encounter gate;
- objective completion was decoupled from spendable Coin balance;
- HUD state is event driven and avoids Event Tick polling.

Final validation:

- SOTM1 Editor Development target: successful;
- selected production Blueprint compile: success, 0 errors, 0 warnings;
- selected production Blueprint/map validation: success, 0 errors, 2 warnings;
- production maps loaded and remained clean.

Remaining pre-existing warnings/issues:

- navigation data reports a serialized `maxTiles` size mismatch and rebuilds its in-memory `dtNavMesh` instance;
- Crowd Manager can report no RecastNavMesh during map teardown/commandlet validation;
- CH1 has a pre-existing zero-scale physics/static-mesh actor warning;
- `/Game/.../Isabella_Jumpscare` references an external developer path (`C:/Users/Marcus Gonzalez/Downloads/Isabella Jumpscare.mp4`) and fails media validation. Phase 2 uses the existing in-project Cruel Doll scream and does not depend on that file.

No new production Blueprint runtime error, `Accessed None`, or C++ compile error was observed in the tested Phase 2 route.

## 22. Exact Phase 2 files created or modified

Created:

- `Source/SOTM1/Public/Objective/SOTMObjectiveSubsystem.h`
- `Source/SOTM1/Private/Objective/SOTMObjectiveSubsystem.cpp`
- `Source/SOTM1/Public/AI/SOTMCousinTypes.h`
- `Source/SOTM1/Public/AI/SOTMCousinCharacter.h`
- `Source/SOTM1/Private/AI/SOTMCousinCharacter.cpp`
- `Source/SOTM1/Public/AI/SOTMCousinAIController.h`
- `Source/SOTM1/Private/AI/SOTMCousinAIController.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase2WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp`
- `ProjectDocs/SOTM_Demo_Phase2_Forest_Objectives_Cousins_Report.md`

Modified:

- `Source/SOTM1/Public/UI/SOTMIngameUIWidget.h`
- `Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp`

Reference input already present and untracked before implementation:

- `Phase2HUD.png`

No `.uasset`, `.umap`, Config file, Forest lighting asset, or Build.cs file was modified for Phase 2.

## 23. Exact Git allowlist

Commit Phase 1 first because its changes are still present in the working tree. For a separate Phase 2 commit, stage only:

```text
Source/SOTM1/Public/Objective/SOTMObjectiveSubsystem.h
Source/SOTM1/Private/Objective/SOTMObjectiveSubsystem.cpp
Source/SOTM1/Public/AI/SOTMCousinTypes.h
Source/SOTM1/Public/AI/SOTMCousinCharacter.h
Source/SOTM1/Private/AI/SOTMCousinCharacter.cpp
Source/SOTM1/Public/AI/SOTMCousinAIController.h
Source/SOTM1/Private/AI/SOTMCousinAIController.cpp
Source/SOTM1/Public/Demo/SOTMDemoPhase2WorldSubsystem.h
Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp
Source/SOTM1/Public/UI/SOTMIngameUIWidget.h
Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp
ProjectDocs/SOTM_Demo_Phase2_Forest_Objectives_Cousins_Report.md
Phase2HUD.png
```

`Phase2HUD.png` is included as the approved design reference. It may be omitted only if the repository intentionally excludes client reference art; doing so does not affect runtime behavior.

Do not include generated `Saved`, `Intermediate`, `DerivedDataCache`, logs, screenshots, acceptance scripts, test save backups, or packaged output. Do not include the still-uncommitted Phase 1 files in the Phase 2 commit.

## 24. Suggested commit title

`Implement Phase 2 Forest objectives HUD and cousin encounters`

## 25. Suggested commit description

```text
- add authoritative Collect All Forest Coins objective using persistent lifetime/unique Coin state
- extend the production gameplay HUD from the approved Phase 2 mockup
- add reusable Cruel Doll cousin pawn and perception/navigation AI
- add gated cousin catch cinematic and lethal Player System integration
- preserve Coin progress across death, respawn, load, Game Over, and Retry
- add Development-only acceptance routes for Phase 2 verification
- document runtime, responsive-layout, compile, and regression results
```

## 26. Short client update

Phase 2 now provides the playable CH1 Forest loop: the approved functional HUD displays the live 0/330 Coin objective, existing health/lives/Coin state, and locked future progression; all 330 placed Coins are uniquely tracked and complete the objective without tying completion to future spendable balance; and reusable Cruel Doll cousins patrol, detect, chase, warn, and trigger one cinematic instant-death catch through the existing Player System. Natural Coin pickup, AI catch, life loss, respawn, persistence, 330 completion, duplicate-catch protection, Game Over/Retry, 1080p/720p layout, C++ build, and production Blueprint/map validation were completed. No Phase 3 ability, Coin spending, chest/key/gate, boss, sword, or Forest-lighting work was added.

## Manual acceptance still recommended

Before presenting to the client, perform these short human checks in PIE:

1. Run the complete Phase 1 New Game click route into CH1 once.
2. Confirm exactly four slots and the complete locked Mansion cinematic sequence.
3. In CH1, let a cousin detect the player, then break pursuit long enough to observe return to patrol.
4. Listen to the catch scream and confirm the montage/camera timing feels appropriately scary.
5. Wait for shader preparation to finish, then capture a clean 1920x1080 HUD screenshot.
6. Relaunch the Editor and click Continue to visually confirm the restored objective count through the user-facing menu.

These limitations are reported explicitly; they are not claimed as automated passes.
