# SOTM Player Death Animation Integration Report

Date: 2026-08-08  
Scope: presentation-only integration of `Anim_Mage_Death_Forward`

## 1. Exact animation asset

- Asset: `/Game/Mage/Animations/Anim_Mage_Death_Forward`
- Type: `UAnimSequence`
- Duration: 3.6667 seconds
- Root motion: disabled
- Root-motion lock setting: reference pose (inactive because root motion is disabled)

The original animation asset was not modified.

## 2. Skeleton compatibility findings

The death animation uses `/Game/Mage/Mesh/SKE_Mage`. The production mesh `/Game/Mage/Mesh/SK_Mage` uses the same skeleton, so the animation is directly compatible and needs no retargeting.

The production character is `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter`. Its `CharacterMesh0` uses `/Game/SuperPowers/Powers/Speedster/Anim/Default/AnimBP/ThirdPerson_AnimBPSpeedster`. That AnimBP is authored for `/Game/SuperPowers/Demo/Character/Mesh/UE4_Mannequin_Skeleton` and exposes no verified Mage-compatible montage slot. A bounded direct sequence presentation is safer than forcing a montage through that AnimBP.

## 3. Integration method

`USOTMPlayerFoundationWorldSubsystem` listens to the existing `USOTMPlayerStateSubsystem::OnPlayerDeathStarted` event. On the first event for the bound production player it:

1. verifies that this death has not already started a presentation;
2. verifies the mesh and sequence skeletons match at runtime;
3. stores the production AnimBP class;
4. plays `Anim_Mage_Death_Forward` once as a non-looping single-node animation;
5. restores the production AnimBP on `OnPlayerRespawned`.

The sequence is preloaded when the player is bound. No permanent Event Tick, repeated actor search, health state, life system, or respawn path was added.

## 4. Montage/AnimBP changes

No montage or Blueprint asset was created or changed. During death only, the mesh enters `ANIMATION_SINGLE_NODE` with `AnimSingleNodeInstance`. On respawn/Retry it returns to `ANIMATION_BLUEPRINT` and `ThirdPerson_AnimBPSpeedster_C`. World teardown also restores it safely.

## 5. Player System death integration

The Player System remains authoritative. `USOTMPlayerVitalComponent` still determines health zero; `USOTMPlayerStateSubsystem::HandlePlayerDeath` still owns the duplicate guard, Death input lock, one-life decrement, respawn, and Game Over. The animation is presentation only.

Repeated damage while dead did not restart the animation, consume another life, or schedule another respawn.

## 6. Death transition timing

The animation lasts 3.67 seconds. The existing respawn window remains 2.00 seconds, showing a useful portion without forcing the full long sequence.

The existing Isabel fade now waits 1.15 seconds after a confirmed lethal normal attack, then fades from 0.0 to 0.85 over 0.30 seconds. A final-life Game Over UI is scheduled after the same 2.00-second presentation interval instead of appearing immediately. The player is already authoritatively dead with zero lives during that interval.

## 7. Respawn restoration

Focused PIE results after normal death and Retry:

- health restored to 100;
- expected remaining lives preserved;
- `PlayerDead=false` and `GameOver=false`;
- no input locks remained;
- animation mode returned to `ANIMATION_BLUEPRINT`;
- AnimBP returned to `ThirdPerson_AnimBPSpeedster_C`;
- movement returned to `MOVE_Walking`;
- move/look ignore flags were false.

The animation triggered again on a second valid death after respawn and restored again afterward.

## 8. Game Over test

With one life, valid damage produced health 0, one decrement to lives 0, `PlayerDead=true`, and the single-node death animation immediately. `GameOver` remained false during the 2.00-second presentation, then became true. The existing Retry API returned true and restored health 100, lives 5, the production AnimBP, walking movement, camera input, and locks.

## 9. Repeat-death test

Six death presentations and six matching AnimBP restorations were logged. Immediate extra damage during ordinary and final-life deaths left the decremented life count unchanged and kept one active presentation. A second death after respawn started the animation exactly once again.

## 10. Regression results

| Check | Result |
|---|---|
| C++ Editor target | Passed: `SOTM1Editor Win64 Development` linked successfully |
| Production Blueprint compile | Passed: `BP_AI`, `ABP_AI`, `BP_MenuSystemCharacter`, `WBP_HealthBar`, `WBP_IngameUI`, `BP_ForestPortal` |
| Isabel catch cinematic | Passed |
| Jump-scare damage | Passed: impact remained `damage=none` |
| Isabel normal attacks | Passed: four 25-damage Player System hits caused the lethal hit |
| Isabel-originated death animation | Passed: sequence started on the fourth normal attack |
| Delayed fade | Passed technically: began after 1.15 seconds |
| One life and duplicate safety | Passed |
| Respawn, second death, Game Over, Retry | Passed |
| AnimBP/movement/input restoration | Passed |
| Mansion to Forest | Passed through real `/Game/BP_ForestPortal`; production player spawned with move/look enabled |
| Package/EXE | Not run, as required |

The inherited `/Game/AI/BP_AI` Event Graph still logs its documented missing-Blackboard `Accessed None` warning. It compiles and this integration did not introduce that legacy warning.

Runtime inspection conclusively verified `AnimSingleNodeInstance`. Screenshots were captured under `Saved/Screenshots/WindowsEditor/`, but final visual readability should be judged manually at normal foreground PIE focus because hidden/background PIE can throttle visible animation progression.

Manual check:

1. Play `Mansion_GameStart`.
2. Let Isabel catch normally and confirm the jump scare remains zero-damage.
3. Remain in attack range until the fourth 25-damage attack reaches health 0.
4. Confirm the player visibly begins `Anim_Mage_Death_Forward` before the fade.
5. Confirm the first 1.15 seconds and overall pacing look appropriate.
6. After respawn, test movement, sprint, jump, and camera.
7. Set one life with the Development panel, repeat, confirm animation before Game Over, then Retry.

## 11. Exact files created/modified

Changed specifically for this task:

- `Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/AI/SOTMIsabelAIController.h`
- `Source/SOTM1/Private/AI/SOTMIsabelAIController.cpp`
- `ProjectDocs/SOTM_Player_Death_Animation_Integration_Report.md`

No Unreal asset, map, AnimBP, source animation, montage, config, package, or EXE was modified.

## 12. Exact Git allowlist

```text
Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h
Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp
Source/SOTM1/Public/SOTMPlayerStateSubsystem.h
Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp
Source/SOTM1/Public/AI/SOTMIsabelAIController.h
Source/SOTM1/Private/AI/SOTMIsabelAIController.cpp
ProjectDocs/SOTM_Player_Death_Animation_Integration_Report.md
```

The Isabel controller files also contain the still-uncommitted approved catch-trigger/final-cinematic changes. If committing all related work together, also use the prior report's notify/report allowlist. Never stage generated `Binaries`, `Intermediate`, `Saved`, screenshots, logs, or unrelated assets. Nothing was staged or committed.

## 13. Suggested commit title and description

Title: `Integrate Mage player death presentation`

Description: `Play Anim_Mage_Death_Forward once from the authoritative Player System death event, preserve a readable pre-fade interval, delay final-life Game Over presentation, and restore the production AnimBP and movement on respawn/Retry.`

## 14. Short client update

The production Mage now plays `Anim_Mage_Death_Forward` exactly once whenever the Player System confirms health zero. Health, death, lives, respawn, Game Over, and Retry remain controlled by the existing system. The fade waits briefly so the death is visible, final-life Game Over follows the presentation interval, and respawn restores the production AnimBP, movement, camera, health, and input. Isabel's catch jump scare remains zero-damage; her normal attacks remain responsible for lethal damage.
