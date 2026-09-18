# SOTM Isabel AI Phase 3 — Cinematic Jump Scare Report

**Project:** The Secrets of the Mansion — Chapter 1  
**Milestone:** Client Task 2, Phase 3 — Cinematic Jump Scare  
**Date:** 2026-08-06  
**Verification environment:** Unreal Engine 5.6 Editor PIE only; no package was produced

## 1. Outcome

Phase 3 adds the remaining Isabel gameplay step to the existing Phase 1/2 controller:

`Patrol → Detect → Chase → Normal Attack → JumpScare → Player System death → Respawn`

The implementation reuses the Phase 1 state machine, Phase 2 attack/notify architecture, the production Isabel pawn and Animation Blueprint, and the completed Player System. It does not add a second AI controller or duplicate health, lives, death, checkpoint, respawn, or Game Over logic.

## 2. Existing assets and systems reviewed

- Phase 1 controller and report.
- Phase 2 controller, normal attack montage/notify, and report.
- Production Isabel Blueprint `/Game/AI/BP_AI`.
- Production Animation Blueprint `/Game/AI/ABP_AI`.
- Isabel skeleton `/Game/AI/CruelDoll/Meshes/SKL_CruelDoll`.
- Existing compatible scream animation `/Game/AI/CruelDoll/Animations/AS_CruelDoll_Scream`.
- Existing audio `/Game/AI/Nightmare_scream_jumpscare_SFX`.
- Legacy `/Game/AI/Jumpscare` and `/Game/JumpscareSequnce1` sequences.
- Legacy media source `/Game/d_oll/DEATH_ANIME/Isabella_Jumpscare`.
- Completed Player System: `USOTMPlayerVitalComponent`, `USOTMPlayerStateSubsystem`, and `USOTMPlayerBlueprintLibrary`.

The legacy jump-scare sequences were not used as the production solution. They are tied to fixed world actors/media, and the legacy media source points to a missing developer-local file. The legacy `BP_AI` jump path also contained direct input/fade/travel behavior and an animation from a different character asset. Reusing it would have bypassed or duplicated completed systems.

## 3. Jump-scare implementation

- Added `JumpScare` to the existing Isabel state enum.
- The trigger is configurable and enabled by default at **25 health**.
- The controller selects the cinematic when a valid living production player is at/below the threshold, or immediately before a normal attack would become lethal at that threshold.
- `bJumpScareTriggeredThisLife`, `bJumpScareInProgress`, and `bJumpScareDamageApplied` prevent duplicate starts and duplicate lethal damage.
- Normal attack movement/montage state is cleanly stopped before the cinematic.
- Isabel faces the current player target and navigation movement is stopped.
- Perception loss caused by the death transition cannot tear down an active cinematic.
- The jump-scare state is callback/timer driven. No Event Tick was added.

## 4. Camera implementation

- A transient runtime `ACameraActor` is created only for the active sequence.
- Camera placement uses Isabel-to-player direction, a configurable side offset, head-bone focus, and a visibility trace to avoid placing the camera through blocking geometry.
- The player view blends in over **0.35 seconds** and back over **0.25 seconds**, using cubic blending.
- During the 1.97-second montage, a bounded 30 Hz timer smoothly tracks Isabel's `head` bone; it is not a permanent tick.
- Default framing values are configurable: distance 70 cm, side offset 70 cm, focus-height fallback 88 cm, FOV 60, tracking speed 12.
- The timer is cleared, the original player pawn is restored as view target, and the transient camera is destroyed on completion, abort, unpossess, or end play.
- Development world-space AI debug text is suppressed only while the cinematic is active so it does not appear in the shot.

Captured PIE frames used for framing review are under `Saved/Screenshots/SOTM_Phase3_Cinematic_Approved_*.png`. `Saved/` is generated evidence and is not part of the Git allowlist.

## 5. Animation and audio

Created `/Game/AI/AM_Isabel_JumpScare_Phase3` from the existing compatible `AS_CruelDoll_Scream` animation.

Verified montage metadata:

- Asset type: Anim Montage
- Skeleton: `/Game/AI/CruelDoll/Meshes/SKL_CruelDoll`
- Duration: 1.9667 seconds
- Sections: 1
- Slots: 1
- Notifies: 1

The dedicated `USOTMIsabelJumpScareImpactNotify` is placed at 1.25 seconds and calls the active Isabel controller once at the impact moment. Existing `/Game/AI/Nightmare_scream_jumpscare_SFX` is played with the montage. Automated tooling confirmed that the audio call executed, but cannot judge audible mix quality; a human listening pass remains recommended.

Patrol, chase, and Phase 2 normal attack assets were not replaced or redesigned.

## 6. Player System integration

- The cinematic acquires only `ESOTMInputLockReason::JumpScare` through `USOTMPlayerBlueprintLibrary`.
- That existing lock disables player movement/camera through the completed Player System; no controller variables or input modes are edited directly.
- The impact notify applies lethal damage through `USOTMPlayerBlueprintLibrary::ApplyPlayerDamage`.
- Current health, legacy HP, lives, dead state, checkpoint, respawn, and Game Over variables are never edited by Isabel.
- If damage is rejected by invulnerability, the one-shot flag is released so a later valid attempt is possible.
- Once Player System death is confirmed, Isabel clears the target and returns safely to patrol.
- The Player System's `OnPlayerRespawned` event resets Phase 3 per-life state and perception so Isabel can reacquire normally.
- Camera and jump-scare input lock cleanup is defensive on every exit path.

## 7. Verification performed

### Build and compile

| Check | Result |
|---|---|
| `SOTM1Editor Win64 Development` after final source change | **Passed** |
| `/Game/AI/BP_AI` compile, without saving | **Passed** |
| `/Game/AI/ABP_AI` compile, without saving | **Passed** |
| `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter` compile, without saving | **Passed** |
| New C++ compile errors | **None** |
| New Blueprint compile errors | **None** |

### Focused Mansion PIE evidence

Repeated captured runs recorded the following sequence:

1. `Patrol → Chase` after valid sight acquisition.
2. `Chase → Attack` in valid attack range.
3. Jump scare selected before a lethal normal hit at 25 health.
4. `Attack → JumpScare`.
5. Runtime camera created and JumpScare input lock held.
6. 1.97-second animation and existing scream audio started.
7. Dedicated impact notify applied exactly 25 lethal damage through the Player System.
8. `JumpScare → ReturnToPatrol → Patrol`.
9. Camera restored/transient camera removed and lock released.
10. Player System completed death/respawn and emitted the Phase 3 per-life reset.

Multiple runs produced the same one-shot impact and cleanup logs. Still-frame inspection confirmed Isabel remains visible during the opening and impact poses and that the player/Isabel composition does not use the broken media sequence.

### Production-map smoke checks after the final build

| Area | Result |
|---|---|
| Main Menu direct PIE | **Passed** — `/Game/Main_Menu_Map`, `BP_MenuLevelGameMode_C`, production menu PlayerController |
| Mansion direct PIE | **Passed** — `/Game/Mansion_GameStart`, production `BP_MenuSystemCharacter_C`, health 100, lives 5, no initial input locks |
| Forest direct PIE | **Passed** — production CH1 map, `BP_PlayLevelGameMode_C`, production `BP_MenuSystemCharacter_C` |
| Forest movement input | **Passed** — move input was not locked and the character transform changed in PIE |
| Existing health/Player System | **Passed** — Phase 3 used the public damage API and Player System owned death/respawn |

The Main Menu's **New Game** button and the Mansion overlap portal were not clicked end-to-end again during the final Phase 3 evidence pass. Their destinations were already verified by the completed Foundation milestone, and Phase 3 changes no maps, menu assets, portal assets, GameModes, or startup settings. Direct PIE loading of all three production maps passed after the final build. This distinction is intentional; the report does not claim a newly captured end-to-end UI transition.

## 8. Known warnings and limitations

1. The inherited production `BP_AI` Event Graph logs an `Accessed None` warning while trying to set a Blackboard bool when no Blackboard exists. This warning was present in the Phase 3 PIE evidence, is unrelated to the new native state machine, and was not introduced or expanded in this milestone. The Blueprint itself compiles successfully. It should be isolated in a later legacy-Blueprint cleanup rather than mixed into the cinematic change.
2. Automated screenshots validate framing at sampled moments but cannot prove subjective camera smoothness as reliably as watching the sequence in real time. A short human PIE viewing pass is recommended before client capture.
3. Automated tools cannot hear the audio mix. The asset load/play path executed; volume and mix quality require a listening pass.
4. Final-life Game Over and Retry remain owned by the unchanged Player System. Phase 3 uses the same lethal API as normal gameplay and does not add a competing final-life branch. A dedicated final-life client demonstration can be performed with the existing Development Player System panel if desired.
5. No Windows package was built, as explicitly required for this phase.

## 9. Manual verification steps

1. Open `/Game/Mansion_GameStart` and start PIE as the player.
2. Confirm Isabel patrols before acquisition.
3. Enter sight range, allow Isabel to chase, and allow normal attacks to reduce health to 25.
4. Confirm the cinematic triggers once, movement and camera are locked, and Isabel faces the player.
5. Watch for smooth blend-in/head tracking and no wall clipping.
6. Listen for the existing scream and confirm impact aligns with the animation.
7. Confirm death consumes one life and Player System respawns at the active checkpoint.
8. Confirm normal player camera/input returns after respawn and Isabel can reacquire later.
9. With the Development Player System panel, set lives to 1 and repeat to confirm the unchanged Game Over/Retry flow.
10. From the Main Menu, click New Game and use the existing Mansion portal once for a final human end-to-end acceptance pass.

## 10. Scope compliance

No HUD, health, lives, checkpoint, respawn, Game Over, menu, map, portal, Forest, coin, objective, boss, Purple Burst, skill tree, Timmy, chest/key, cutscene, leaderboard, or tournament asset was modified. No Event Tick was added. No package was produced.

## 11. Exact files created or modified

- `Source/SOTM1/Public/AI/SOTMIsabelAITypes.h`
- `Source/SOTM1/Public/AI/SOTMIsabelAIController.h`
- `Source/SOTM1/Private/AI/SOTMIsabelAIController.cpp`
- `Source/SOTM1/Public/AI/SOTMIsabelJumpScareImpactNotify.h` (new)
- `Source/SOTM1/Private/AI/SOTMIsabelJumpScareImpactNotify.cpp` (new)
- `Content/AI/AM_Isabel_JumpScare_Phase3.uasset` (new)
- `ProjectDocs/SOTM_Isabel_AI_Phase3_Report.md` (new)

## 12. Exact Git allowlist

Stage only the seven paths listed in section 11. Do not stage `Saved/`, `Intermediate/`, `DerivedDataCache/`, binaries, screenshots, logs, or unrelated assets.

## 13. Suggested commit

**Title**

`Implement Isabel cinematic jump scare and Player System handoff`

**Description**

`Extend the existing Isabel state machine with a one-shot cinematic jump scare, collision-aware blended camera, compatible scream montage and notify-timed lethal damage through the completed Player System. Preserve Phase 1/2 patrol, chase and attack behavior, and cleanly restore camera/input across death and respawn.`

## 14. Suggested client update

Isabel's final gameplay phase is implemented. She continues to patrol, detect, chase, and use her normal attack; at the configured low-health threshold she now transitions into a one-shot cinematic jump scare with a smooth tracked camera, compatible animation, and existing scream audio. The impact uses the completed Player System, so life loss, checkpoint respawn, and Game Over remain authoritative and unchanged. The Editor target and affected production Blueprints compile successfully, repeated Mansion PIE runs completed the jump-scare/death/respawn path, and direct production Main Menu/Mansion/Forest smoke checks passed. A short human listening and end-to-end menu/portal acceptance pass is still recommended before recording the client demonstration.
