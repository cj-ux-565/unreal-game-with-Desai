# SOTM Client Demo — Phase 2 + Phase 3 Final Acceptance Report

Date: 18 August 2026  
Engine: Unreal Engine 5.6.1  
Scope: self-debugging, regression, and final acceptance of the already implemented Phase 2 Forest loop and Phase 3 Timmy/Speed Boost loop

## 1. Acceptance status

The tested Phase 2 and Phase 3 gameplay paths are stable and compile successfully. The final pass found one defect in a Development-only Phase 2 acceptance route, repaired that test harness, and reran the affected route successfully. It did not require a production catch, Player System, Coin, objective, AI, Timmy, or Speed Boost redesign.

The following are verified by captured runtime logs and/or screenshots:

- actual Main Menu navigation to Single Player and an exactly four-slot New Game screen;
- actual New Game travel to `Mansion_GameStart`, the locked Mansion intro/zero-damage knockout sequence, and travel to production CH1;
- 330 valid, unique placed CH1 Coins and objective completion at 330/330;
- natural Coin pickup, autosave, reload persistence, death persistence, Game Over, and Retry;
- eight production Cousins, natural perception/chase/catch, one lethal impact, exactly one life loss, respawn, and duplicate-catch rejection;
- Timmy upgrade requirements, 250-Coin purchase, duplicate-purchase rejection, available/lifetime Coin separation, and per-slot/relaunch persistence;
- production `Q` input, 1.40x Speed Boost, 3-second active duration, 10-second cooldown, no stacking, pause/resume stability, and death cleanup;
- upgrade UI repeated open/close behavior and production Esc/Tab pause behavior;
- Editor Development build and selected production Blueprint/map validation;
- original user save files restored byte-for-byte after testing.

This is not a Phase 4 implementation. No chest, key, gate, ending, boss, dark magic, lightning, sword, Forest-lighting, or later gameplay work was performed.

## 2. Source request limitation

The supplied acceptance specification file contains 1,162 lines and ends in the middle of Section 44 after `movement baseline = correct normal speed`. No text after that point exists in the attachment. This report covers every complete section available (Sections 1–43) and the available part of Section 44. It does not invent missing requirements, success criteria, report naming, or a later phase.

## 3. Clean restart and build

- Closed the active Editor test process before the clean build.
- Initial high-parallel UBA compilation reached genuine host-memory pressure and was cancelled rather than treated as a product failure.
- Rebuilt the Editor Development target with `-NoUBA -MaxParallelActions=2`.
- Full clean result: **355 actions succeeded**; total parallel executor time 1,132.24 seconds; overall build time 1,133.76 seconds.
- After the Development-only acceptance-route fix, an incremental build ran UnrealHeaderTool and completed **7 compile/link actions successfully**. UHT reported zero generated-file rewrites.
- No C++ compile errors were present.
- Existing deprecation/include-order warnings remain (`StructUtils` and the older UE 5.3 include order); neither is a runtime acceptance blocker.

## 4. Production Blueprint and map validation

Read-only validation compiled:

- `BP_MenuSystemCharacter`
- `Coin`
- `WBP_IngameUI`
- `WBP_HealthBar`
- `BP_ForestPortal`
- `BP_AI`

It also loaded:

- `/Game/Main_Menu_Map`
- `/Game/Mansion_GameStart`
- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

Result: no production Blueprint compile errors, no commandlet errors, and `dirty_maps=[]`. The two emitted navigation/crowd warnings are listed under Known Remaining Warnings.

## 5. Phase 1 regression and actual UI route

The production UI was driven through the real menu path rather than calling map travel directly:

`Main Menu -> Play -> Single Player -> New Game -> Save Slot screen -> Slot 1 confirmation -> Mansion_GameStart -> Mansion intro -> CH1`

Verified:

- the save UI normalized to exactly four story slots;
- selecting Slot 1 opened the expected overwrite confirmation for the copied test save;
- New Game travelled to the approved Mansion map;
- the Mansion intro acquired its cinematic lock;
- the scripted zero-damage knockout presentation occurred;
- the Mansion intro completed and travelled to the approved CH1 Forest;
- CH1 initialized the production `BP_MenuSystemCharacter`, Phase 2 HUD, eight Cousins, and Phase 3 Timmy station.

Evidence:

- `Saved/Logs/Phase1FinalWithCopiedSaves.log`
- `Saved/Screenshots/WindowsEditor/FinalAcceptance/Phase1_FourSlots.png`

The automation-launched Editor stopped exposing a capturable top-level window handle after the final travel, so a new CH1 screenshot could not be captured from this specific click-through. The successful travel and subsystem initialization are recorded in the runtime log. Player movement and CH1 interaction were independently exercised in the Phase 2/3 runtime routes.

## 6. Pause regression

Native Windows key events were sent to the real Unreal game window, and the resulting production CommonUI activation was observed.

- Esc during ACTIVE opened and closed the existing pause UI.
- Tab during COOLDOWN opened and closed the existing pause UI.
- Exactly two pause activations occurred; no duplicate pause widget was observed.
- Active/cooldown timers remained valid and returned to READY.
- Input and movement recovered after each resume.

Evidence:

- `Saved/Logs/Phase3PauseSynchronized.log`
- `Saved/Screenshots/WindowsEditor/FinalAcceptance/Phase3_ESC_Pause_Active.png`
- `Saved/Screenshots/WindowsEditor/FinalAcceptance/Phase3_TAB_Pause_Cooldown.png`

## 7. Phase 2 HUD and objective authority

The production `WBP_IngameUI` remained data-driven and showed the approved Phase 2 objective/Coin/health/lives presentation. Objective authority remains in `USOTMObjectiveSubsystem`; the widget does not calculate progression.

Read-only CH1 audit:

- placed Coin actors: **330**
- valid persistent Coin IDs: **330**
- unique IDs: **330**
- duplicate IDs: **0**
- invalid IDs: **0**
- map dirtied by audit: **no**

The objective uses lifetime/unique collection history, not spendable balance. Therefore spending 250 Coins in Phase 3 leaves Collect All Coins complete at 330/330.

## 8. Phase 2 Coin, persistence, death, and completion results

Final runtime results:

- five real placed Coin overlap pickups changed available/lifetime/unique/objective state from 0 to 5;
- autosave ran on successful collection;
- one natural Cousin catch changed lives from 5 to 4 exactly once;
- after respawn, all five Coins and 5/330 objective progress remained;
- explicit Slot 1 reload restored available 5, lifetime 5, unique 5, and lives 4;
- completion setup reached 329 through the authoritative collection API, then a real placed Coin overlap supplied the final unique Coin;
- the final pickup produced 330/330 and `Completed`;
- a separate process immediately restored 330 available, 330 lifetime, 330 unique, Completed, and lives 5;
- final-life catch produced lives 0, dead true, and Game Over true;
- existing Retry restored lives 5, dead false, and Game Over false.

Evidence:

- `Saved/Logs/Phase2FinalAcceptanceRerun.log`
- `Saved/Logs/Phase2FinalPersistence5.log`
- `Saved/Logs/Phase2FinalCompletion.log`
- `Saved/Logs/Phase2FinalCompletionPersistenceSetup.log`
- `Saved/Logs/Phase2FinalPersistence330Immediate.log`
- `Saved/Logs/Phase2FinalGameOver.log`

The pass did not physically walk through all 330 placements. Coverage consists of a static audit of every placed Coin, five natural overlap pickups, authoritative unique-collection setup to 329, a natural final overlap, and separate-process persistence. This distinction is intentional and is not represented as a 330-Coin manual traversal.

## 9. Phase 2 Cousin encounter results

- Eight Cruel Doll Cousins initialized and began patrol.
- A Cousin detected the living production player through AI Perception.
- Natural chase reached the normal public catch gate.
- The catch used the existing in-engine montage/camera/audio path.
- Exactly one lethal Player System damage request occurred.
- The Player System, not Cousin code, owned health, death, life loss, respawn, Game Over, and Retry.
- Cousins stopped attacking during death/respawn and could resume their normal runtime behavior afterward.
- Simultaneous/duplicate catch was rejected.

The dedicated visual escape/lost-sight/search/return-to-patrol route was not newly replayed in this final pass. Its timer-driven code and prior Phase 2 implementation remain intact, but final client observation of that behavior is still recommended. Audible jump-scare quality and subjective camera composition also require a human review; automation verified invocation and state transitions, not artistic approval.

## 10. Defect found and repaired

The first final Phase 2 acceptance run completed a real natural catch and normal respawn. Because respawn correctly cleared `bCatchActive`, the Development-only fallback timer later misinterpreted the cleared flag as “no catch occurred” and invoked the public catch gate a second time. This caused a second legitimate Player System death in the test route even though production catch protection was correct.

Fix:

- added Development-only `bDevelopmentAcceptanceCatchObserved` state;
- reset it when the acceptance route starts;
- set it when any catch is accepted;
- allow the fallback only when no catch has ever been observed during that route.

The corrected rerun produced one natural catch, one lethal impact, and exactly one life decrement (5 -> 4). The flag is enclosed in `#if !UE_BUILD_SHIPPING`; production Shipping gameplay is unaffected.

## 11. Phase 3 Timmy purchase acceptance

Verified states and transactions:

- objective incomplete, 120/330: Speed Boost locked;
- objective complete but only 100 available Coins: purchase rejected as `NotEnoughCoins`;
- objective complete and 330 available Coins: purchase ready;
- first purchase succeeded;
- available Coins changed 330 -> 80;
- lifetime Coins remained 330 and objective remained Completed;
- immediate duplicate purchase was rejected as `AlreadyOwned`;
- Level 1 ownership persisted through death, respawn, slot load, and process relaunch;
- test Slot 1 restored unlocked Level 1/80 available, while test Slot 2 remained locked/0, proving slot isolation.

The station uses the real Timmy-adjacent CH1 placement and reuses production `IA_Interact` (`E`). Purchase authority remains in `USOTMPlayerStateSubsystem`; the UI does not directly mutate Coins or ability state.

## 12. Upgrade UI stress result

The real `E` input and Escape were exercised for five open/close cycles during cooldown.

- each `E` generated the production character interaction log;
- each cycle created one expected open event and closed normally;
- no stuck cursor, input lock, duplicate persistent widget, or timer corruption was observed;
- the route continued through READY, death/respawn cleanup, and its final state.

Evidence: `Saved/Logs/Phase3UpgradeUIStressFinal.log` (`CONFIRMED_E_ESCAPE_CYCLES=5`).

## 13. Speed Boost input and values

The final input proof used native `WM_KEYDOWN/WM_KEYUP` for `Q` against the real Unreal game window. The event travelled through the production Enhanced Input binding; the validation did not directly call `TryActivateSpeedBoost()`.

- base `MaxWalkSpeed`: **600**
- multiplier: **1.40x**
- active speed: **840**
- active duration: **3.0 seconds**
- cooldown: **10.0 seconds**
- immediate stacking attempt: rejected
- cooldown reactivation attempt: rejected
- after cooldown: state READY, speed 600

Evidence: `Saved/Logs/Phase3PhysicalQPostMessage.log`.

## 14. Pause, chase, death, respawn, and persistence with Boost

- Natural Cousin perception/chase remained functional while Boost was ACTIVE.
- Boost did not grant invulnerability.
- A lethal hit during ACTIVE changed lives 5 -> 4 exactly once.
- Death terminated ACTIVE, cleared timers/presentation, and restored movement speed 840 -> 600.
- After respawn, Speed Boost remained owned at Level 1 and returned READY with speed 600.
- Separate-process Continue/load restored available 80, lifetime 330, objective Complete, unlocked Level 1, READY, speed 600, and living state.

The automated route demonstrated AI functionality and the speed delta, but did not produce an objective physical-distance comparison suitable for final balancing. Whether 1.40x feels sufficiently useful while keeping the Cousin threatening remains a client play-feel decision.

## 15. Resolution and visual evidence

The Phase 3 UI was exercised at 1280x720 through the runtime route. An exact 1920x1080 capture was produced using forced resolution after Windows desktop scaling initially yielded a 1423x889 client area.

Key evidence:

- `Saved/Screenshots/WindowsEditor/Phase3/14_1920x1080_Continue_READY.png`
- `Saved/Screenshots/WindowsEditor/Phase3/01_1280x720_Timmy_Prompt.png`
- `Saved/Screenshots/WindowsEditor/Phase3/06_SpeedBoost_ACTIVE.png`
- `Saved/Screenshots/WindowsEditor/Phase3/07_SpeedBoost_COOLDOWN.png`
- `Saved/Screenshots/WindowsEditor/Phase3/09_Natural_Cousin_Chase_Boost.png`
- `Saved/Screenshots/WindowsEditor/Phase3/12_Post_Respawn_READY.png`

These are generated verification artifacts under `Saved/` and must not be committed.

## 16. Save-file safety

Before acceptance, the existing `Saved/SaveGames` folder was moved to an isolated backup and hashed. Tests used a fresh/copy-only test folder. At the end:

- the test SaveGames folder was removed;
- the original backup was restored to `Saved/SaveGames`;
- restored files: **23**;
- restored bytes: **70,532**;
- SHA-256 mismatches: **0**;
- temporary backup folder remaining: **no**.

No user save was intentionally changed by this acceptance pass.

## 17. Known remaining warnings and manual acceptance items

Known pre-existing/runtime warnings:

1. `/Game/.../Isabella_Jumpscare` still points to `C:/Users/Marcus Gonzalez/Downloads/Isabella Jumpscare.mp4` and fails media validation. The Phase 2 Cousin catch uses an in-project montage/camera/scream path and does not depend on this external file.
2. CH1 navigation reports a serialized `maxTiles` mismatch; commandlet teardown may also report that Crowd Manager has no RecastNavMesh.
3. CH1 still reports one pre-existing near-zero-scale physics/static-mesh actor.
4. First pause-menu loading can emit a Niagara `PostSystemTick_GameThread` warning for the Forest portal during PostLoad. The tested pause routes completed successfully.
5. Marketplace MenuSystemPro emits enum-name collision/invalid maximum-value warnings during the actual menu route. Four-slot normalization and production travel still succeeded.
6. The Mansion intro has intentional minimum-delay/loading latency.

Recommended short human checks before recording the client demo:

- watch and listen to the complete Cousin jump scare for artistic timing/audio approval;
- break line of sight during Cousin chase and observe return to patrol;
- judge whether 1.40x for 3 seconds feels useful but not invincible;
- perform one normal keyboard/mouse click-through on the development PC while screen recording.

These items are not concealed as automated passes.

## 18. Files intentionally modified by this final acceptance pass

- `Source/SOTM1/Public/Demo/SOTMDemoPhase2WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp`
- `ProjectDocs/SOTM_Demo_Phase2_Phase3_Final_Acceptance_Report.md`

The remaining dirty/untracked Phase 3 files listed below existed as the uncommitted Phase 3 implementation before this final acceptance pass. They are required for Phase 3 and should be included in the eventual Phase 3/acceptance commit.

No `.uasset`, `.umap`, Config, Forest lighting, environment, chest, key, gate, boss, weapon, or later-ability file was modified during this pass.

## 19. Exact Git allowlist for the current intended Phase 3 + final acceptance commit

Stage only:

```text
Phase3HUD.png
ProjectDocs/SOTM_Demo_Phase3_Timmy_SpeedBoost_Report.md
ProjectDocs/SOTM_Demo_Phase2_Phase3_Final_Acceptance_Report.md
Source/SOTM1/SOTM1.Build.cs
Source/SOTM1/Public/SOTMPlayerStateSubsystem.h
Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp
Source/SOTM1/Public/UI/SOTMIngameUIWidget.h
Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp
Source/SOTM1/Public/Ability/SOTMSpeedBoostTypes.h
Source/SOTM1/Public/Ability/SOTMPhase3Settings.h
Source/SOTM1/Public/Ability/SOTMTimmyUpgradeStation.h
Source/SOTM1/Private/Ability/SOTMTimmyUpgradeStation.cpp
Source/SOTM1/Public/Demo/SOTMDemoPhase3WorldSubsystem.h
Source/SOTM1/Private/Demo/SOTMDemoPhase3WorldSubsystem.cpp
Source/SOTM1/Public/UI/SOTMUpgradeStationWidget.h
Source/SOTM1/Private/UI/SOTMUpgradeStationWidget.cpp
Source/SOTM1/Public/Demo/SOTMDemoPhase2WorldSubsystem.h
Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp
```

Do not stage `Saved/`, screenshots, logs, test saves, binaries, intermediates, DerivedDataCache, package output, or unrelated project content. Nothing was staged or committed automatically.

## 20. Suggested commit

Title:

`feat: finalize demo phase 3 and phase 2 regression acceptance`

Description:

```text
- add the Timmy Level 1 Speed Boost purchase and persistent available-Coin economy
- add functional Locked, Ready, Active, and Cooldown HUD/station states
- preserve objective completion, save-slot isolation, death/respawn, pause, and Cousin behavior
- prevent the Development Phase 2 fallback from replaying after a successful natural catch
- document clean build, runtime regression, persistence, input, and responsive UI acceptance
```

## 21. Client update

Phase 2 and Phase 3 have completed their technical acceptance pass. The Forest has 330 uniquely tracked Coins, persistent objective progress, eight patrol/chase/catch Cousins, one-life jump-scare deaths, respawn, Game Over, and Retry. Timmy now sells persistent Speed Boost Level 1 for 250 spendable Coins after Collect All Coins is complete; the ability activates from Q at 1.40x for 3 seconds with a 10-second cooldown and cleans up safely on pause, death, respawn, and reload. The real Main Menu -> four slots -> Mansion intro -> Forest route was also rerun. No Forest lighting or Phase 4 gameplay was changed.
