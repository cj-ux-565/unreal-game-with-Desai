# SOTM Player System Foundation Report

**Project:** The Secrets of the Mansion — Chapter 1  
**Milestone:** Client Task 3 — Player System Foundation  
**Date:** 30 July 2026  
**Git:** Nothing was staged or committed by Codex.

## 1. Existing player systems reused

- Production character: `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter`
- Existing movement, camera, jump, sprint/stamina, pause and interaction graphs were preserved.
- Production GameMode/player contract was preserved.
- Unreal's standard `Apply Damage` path remains supported. Existing attacks that use `Apply Damage` can reach the new health component without AI changes.
- Existing Menu System Pro GameInstance, save manager and custom SaveGame object were retained.
- Existing Main Menu, Mansion and Forest maps and travel paths were not redesigned.
- Legacy character values `HP`, `CurrentHealth` and `isdead?` are synchronized for compatibility; they are no longer the authoritative implementation.
- The inherited death widget was not reused because its Retry and Main Menu buttons route to obsolete maps (`CH1` and `Menu`).

## 2. New architecture

### Player vital component

`USOTMPlayerVitalComponent` is an event-driven, non-ticking Actor Component that owns:

- maximum/current health;
- standard Unreal damage reception;
- healing and reset;
- invulnerability timing;
- dead state;
- duplicate-death prevention;
- health, damage and death delegates.

The production pawn receives a transient `SOTM_PlayerVitals` component at runtime. The large production character Blueprint was not expanded or resaved.

### Persistent player/chapter state

`USOTMPlayerStateSubsystem` is a GameInstance subsystem that owns:

- current/maximum lives;
- persistent health;
- current checkpoint ID, map and transform;
- death, respawn and Game Over state;
- reason-counted input locks;
- SaveGame serialization;
- retry and Main Menu routing.

The subsystem persists across normal map travel. Future coins, abilities, objectives, key and boss progression were intentionally not implemented.

### Runtime integration

`USOTMPlayerFoundationWorldSubsystem` discovers the controlled pawn after each gameplay world starts, attaches/reuses the vital component, binds it to player state, and creates a safe map-entry checkpoint. It uses a short one-shot timer for pawn discovery and has no Event Tick.

### Checkpoints

`ASOTMCheckpoint` is a reusable box-trigger checkpoint actor with:

- activate-once behavior;
- configurable respawn transform;
- checkpoint activation delegate;
- no Tick;
- no required final art/audio.

Mansion and Forest also receive automatic map-entry checkpoints so the foundation is safe before authored checkpoint actors are placed.

### Game Over

The native `USOTMGameOverWidget` provides:

- Game Over presentation;
- Retry from Checkpoint;
- Main Menu.

It is deliberately neutral temporary UI, not a final HUD redesign.

### Input locking

Locks are reference-counted by reason:

- Death
- Respawn
- Cinematic
- Jump Scare
- Pause Menu
- Game Over
- Custom

Releasing one reason cannot clear another reason or an additional lock of the same reason.

### Public API

`USOTMPlayerBlueprintLibrary` exposes stable Blueprint calls for future AI, HUD and cinematic work:

- find player-state subsystem;
- find vital component;
- apply player damage;
- acquire/release an input lock;
- activate a checkpoint.

Standard `Apply Damage` remains compatible.

### Save integration

The existing custom SaveGame object was extended additively with:

- `SOTM_HasPlayerState`
- `SOTM_SaveVersion`
- `SOTM_CurrentLives`
- `SOTM_MaximumLives`
- `SOTM_CurrentHealth`
- `SOTM_CheckpointId`
- `SOTM_CheckpointMap`
- `SOTM_CheckpointTransform`

Version 1 uses `SOTM_HasPlayerState=false` as the safe default. Older slots therefore retain default player state rather than crashing.

Normal `SavePlayerState`/`LoadPlayerState` use the active Menu System Pro save manager. Explicit named-slot APIs were also added for safe testing and future slot-selection integration. The serializer tolerates the existing Blueprint numeric representation of `SOTM_CurrentHealth`.

No save is automatically deleted.

## 3. Temporary configurable defaults

These values are temporary and live under `[/Script/SOTM1.SOTMPlayerSystemSettings]`:

| Setting | Temporary value |
|---|---:|
| Maximum health | 100 |
| Starting lives | 5 |
| Maximum lives | 5 |
| Damage invulnerability | 1 second |
| Respawn delay | 2 seconds |
| Respawn health | Full |
| Respawn safety invulnerability | 1 second |
| Zero lives | Wait at Game Over |
| Retry | Restore five lives at the active checkpoint |
| Save deletion | Never automatic |

## 4. Tests completed

### Automation

Commandlet suite: `SOTM.PlayerFoundation`

Final result: **3/3 passed**

- `SOTM.PlayerFoundation.Vitals`
- `SOTM.PlayerFoundation.InputLocks`
- `SOTM.PlayerFoundation.SaveSchema`

Evidence log: `Saved/Logs/PlayerFoundationAutomationVerified.log` (generated; do not commit).

### Runtime test matrix

| # | Test | Result |
|---:|---|---|
| 1 | Starts with five lives | Passed: 5/5 in Mansion and Forest |
| 2 | Partial damage exactly once | Passed: 100 → 75 |
| 3 | Invulnerability ignores damage | Passed: first 10 accepted, immediate second 10 rejected |
| 4 | Heal clamps to maximum | Passed: 35 restored, result remained 100 |
| 5 | Fatal damage triggers once | Passed |
| 6 | One death removes one life | Passed: 5 → 4 |
| 7 | Respawn at active checkpoint | Passed |
| 8 | Respawn health rule | Passed: restored to 100 |
| 9 | Death input/movement lock and restoration | Passed: Death lock active during death; walking restored and locks empty after respawn |
| 10 | Repeated damage during death | Passed: second fatal call rejected; no second life loss |
| 11 | New checkpoint changes respawn | Passed: respawned at runtime checkpoint `(3612.87, 11038.38, 98.28)` |
| 12 | Mansion checkpoint respawn | Passed |
| 13 | Forest checkpoint respawn | Passed at `MapEntry_CH1`; lives 5 → 4, health restored, movement active |
| 14 | Save, restart and load | Passed using isolated `SOTM_PlayerFoundation_Test`: fresh PIE restored 3 lives and exact Mansion checkpoint, then respawned there |
| 15 | Zero lives Game Over once | Passed: lives 0, Game Over true, Death + GameOver locks |
| 16 | Retry policy | Passed: Retry restored 5 lives, 100 health and cleared all locks |
| 17 | Main Menu route | Passed: Game Over/Main Menu API loaded `/Game/Main_Menu_Map` |
| 18 | Main Menu → Mansion → Forest | Core maps and travel passed. Main Menu title, Play and Create New Game slot screen were exercised interactively; Mansion → Forest travel and player persistence passed. The final existing-slot click was intentionally not performed to avoid overwriting the user's Slot 1. |
| 19 | Existing movement/camera/sprint/stamina | Movement passed after reload and in Forest. No related Blueprint was changed. Synthetic mouse input was captured by editor Slate, so camera feel and sprint/stamina presentation require the short manual check below. |
| 20 | Production Blueprint compile | Passed for the affected save object, production character, both production GameModes, production PlayerController and Forest portal; compiles were run without saving unrelated assets. |

Game Over visual evidence was captured in:

`Saved/Screenshots/SOTM_GameOver_FullEditor.png`

The MCP game-viewport screenshot mode omits Slate/UMG overlays; the full-editor capture visibly contains Game Over and both choices.

## 5. Manual verification still required

Automated tooling cannot reliably prove subjective camera feel, sprint/stamina presentation, or click an existing save slot without risking its contents.

1. Launch PIE or Standalone from `/Game/Main_Menu_Map`.
2. Click through the title, then **Play → New Game**.
3. Use a known disposable/empty save slot.
4. Confirm `/Game/Mansion_GameStart` loads.
5. Walk, look, jump and hold Sprint; confirm camera feel and stamina presentation are unchanged.
6. Use the existing Mansion-to-Forest portal and confirm `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1` loads.
7. Repeat walk/look/sprint briefly in the Forest.

Development-only console helpers:

- `SOTM.Player.Damage 25`
- `SOTM.Player.Heal 25`
- `SOTM.Player.SetLives 1`
- `SOTM.Player.CheckpointHere TestCheckpoint`
- `SOTM.Player.Respawn`
- `SOTM.Player.Save`
- `SOTM.Player.Load`
- `SOTM.Player.Retry`

## 6. Production Blueprint compile verification

Compiled successfully without saving unrelated assets:

- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject`
- `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter`
- `/Game/MenuSystemPro/Blueprints/GameFramework/BP_PlayLevelGameMode`
- `/Game/MenuSystemPro/Blueprints/GameFramework/BP_MenuLevelGameMode`
- `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemPlayerController`
- `/Game/BP_ForestPortal`

No new production Blueprint compile error was introduced.

## 7. Remaining risks

- Authored Mansion/Forest checkpoint placement and final audiovisual feedback remain future content work.
- Retry policy and life/health rules are temporary pending client confirmation.
- The existing save manager must have an active SaveGame object for the default save API. Explicit named-slot APIs remain available for testing and future slot UI.
- Current health is serialized through a tolerant numeric adapter because the Blueprint's runtime numeric representation differs from its reported metadata.
- Existing AI stacks and 330 coin actors were not touched.
- Existing HUD health presentation is not authoritative yet; future HUD work should subscribe to the new delegates.
- Active player abilities are prevented from receiving gameplay input on death, but a future ability-system milestone should explicitly cancel any long-running ability tasks.
- The project still emits unrelated environment warnings for UDP messaging and legacy iOS configuration during commandlet startup.

## 8. Client questions

1. Confirm final maximum health and damage balance.
2. Confirm whether checkpoints restore full health, partial health or saved health.
3. Confirm whether checkpoints ever restore lives.
4. Confirm the final zero-life policy: checkpoint, Forest restart, Chapter restart or save-slot selection.
5. Confirm whether Retry should restore all five lives or another amount.
6. Confirm whether death should play a character animation, jump scare, fade or dedicated screen.
7. Confirm whether checkpoint activation should always autosave.

## 9. Exact files intentionally created or modified

### Modified

- `Config/DefaultGame.ini`
- `Content/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject.uasset`
- `Source/SOTM1/SOTM1.Build.cs`

### Created

- `Source/SOTM1/Public/SOTMPlayerSystemSettings.h`
- `Source/SOTM1/Private/SOTMPlayerSystemSettings.cpp`
- `Source/SOTM1/Public/SOTMPlayerVitalComponent.h`
- `Source/SOTM1/Private/SOTMPlayerVitalComponent.cpp`
- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp`
- `Source/SOTM1/Public/SOTMCheckpoint.h`
- `Source/SOTM1/Private/SOTMCheckpoint.cpp`
- `Source/SOTM1/Public/SOTMPlayerBlueprintLibrary.h`
- `Source/SOTM1/Private/SOTMPlayerBlueprintLibrary.cpp`
- `Source/SOTM1/Public/SOTMGameOverWidget.h`
- `Source/SOTM1/Private/SOTMGameOverWidget.cpp`
- `Source/SOTM1/Private/SOTMPlayerDebugCommands.cpp`
- `Source/SOTM1/Private/Tests/SOTMPlayerFoundationTests.cpp`
- `ProjectDocs/SOTM_Player_System_Foundation_Report.md`

## 10. Exact Git allowlist

Commit only the 20 files listed in section 9. Exclude:

- `Saved/`
- `Intermediate/`
- `DerivedDataCache/`
- `Binaries/`
- screenshots, logs and test `.sav` files

No coins, objectives, HUD redesign, AI, boss, skills, Timmy, chest/key or cutscene asset was intentionally modified.
