# SOTM Isabel Jump Scare Catch-Trigger Rework Report

Date: 2026-08-08
Project: The Secrets of the Mansion - Chapter 1
Engine: Unreal Engine 5.6
Scope: Client Task 2, catch-based camera jump scare rework only

## 1. Previous 25-health implementation

The committed Phase 3 controller used `JumpScareHealthThreshold = 25.0f`. `HandleAttackImpactNotify()` evaluated `ShouldBeginJumpScare()` both before a lethal normal hit and after a normal hit reduced the player to the threshold. `ShouldBeginJumpScare()` explicitly read `USOTMPlayerVitalComponent::GetCurrentHealth()` and required health to be at or below the threshold.

The montage notify then called `HandleJumpScareImpactNotify()`, which applied all remaining health through `USOTMPlayerBlueprintLibrary::ApplyPlayerDamage()`. A montage-end fallback called the same impact handler when the notify had not fired. The resulting old sequence was normal damage to 25, jump scare, lethal jump-scare damage, Player System death.

## 2. Exact trigger logic removed

Removed from `ASOTMIsabelAIController`:

- `JumpScareHealthThreshold` and all health-based jump-scare decisions.
- The pre-lethal-hit branch in `HandleAttackImpactNotify()`.
- The post-normal-hit threshold branch in `HandleAttackImpactNotify()`.
- All lethal damage from `HandleJumpScareImpactNotify()`.
- The montage-end fallback that could apply hidden lethal damage.
- The per-life and damage-applied flags that represented the old design.

The only remaining `ApplyPlayerDamage` call in the Isabel controller is the existing normal attack path with configurable `AttackDamage`.

## 3. New encounter definition

A chase encounter begins when the existing sight perception acquires the living production player and enters Chase. The encounter remains active while Isabel and the player move between Chase and Attack, including small movements outside `AttackRange`.

`bJumpScarePlayedThisEncounter` is reset only when the encounter genuinely ends:

- target loss and entry into Search Last Known;
- target fully cleared;
- Return to Patrol;
- Patrol entry;
- player death/respawn reset;
- an aborted cinematic that never completed.

Moving from Attack to Chase because the player is just outside attack range does not reset the flag.

## 4. New catch trigger

`EvaluateChase()` now evaluates the catch before `TryBeginAttack()`. A catch jump scare is allowed only when all of the following are true:

- catch jump scares are enabled;
- the current state is Chase;
- the jump scare is not already active;
- it has not played in this encounter;
- the target is the current production player;
- the player is living and not dead, respawning, or at Game Over;
- distance is within the configurable normal `AttackRange`;
- line of sight is valid;
- Isabel is facing the target.

When these conditions pass, the encounter flag is consumed and the existing JumpScare state begins before any normal attack.

## 5. Encounter reset rules

Verified rules:

- Small escape: moving outside attack range did not clear `bJumpScarePlayedThisEncounter`. Isabel chased back and continued normal attacks without another cinematic.
- Genuine escape: moving beyond perception caused `SearchLastKnown`, `CurrentTarget = None`, and `bJumpScarePlayedThisEncounter = false`.
- Death: the normal lethal hit cleared the target and reset the encounter.
- Retry/respawn: the flag, stale target, perception memory, camera lock, and input locks were clear afterward.

## 6. Jump-scare camera flow

The existing camera implementation was preserved:

1. stop navigation and face the player;
2. enter JumpScare;
3. acquire the Player System `JumpScare` input lock;
4. create and blend to the close camera;
5. play the existing 1.97-second Isabel montage and existing scream audio;
6. track Isabel during the cinematic;
7. blend back to the production player camera;
8. release the input lock;
9. only then resume AI evaluation.

PIE capture during the first catch recorded:

- state: `JumpScare`;
- health: `100.0`;
- encounter used: `true`;
- view target: cinematic `CameraActor_11`;
- active input locks: `[JumpScare]`.

A diagnostic post-cinematic screenshot at `Saved/Screenshots/WindowsEditor/SOTM_CatchJumpScare_PIE.png` confirms return to the normal player camera with Isabel nearby. It is generated evidence and must not be committed.

## 7. Jump-scare damage removal

`USOTMIsabelJumpScareImpactNotify` remains in the existing montage as a presentation timing point. It now sets the one-shot `bJumpScareImpactReached` diagnostic flag and logs the synchronized impact. It does not call the Player System damage API and cannot reduce health, consume a life, trigger death, or trigger Game Over.

The notify display name and source documentation now identify it as `Isabel Jump Scare Presentation Impact`.

## 8. Attack flow after the jump scare

`FinishJumpScare()` restores the camera and locomotion mode, then waits for the camera blend-out plus 0.02 seconds. `ResumeAfterJumpScare()` evaluates the live situation:

- living target, sight, line of sight, and in range: Chase immediately evaluates into normal Attack;
- living target outside range: Chase and navigation resume;
- invalid or invisible target: existing Search/Return behavior resumes.

PIE logs recorded this exact order:

`Chase -> JumpScare -> presentation impact -> camera restoration -> Chase -> Attack`

The first normal impact occurred after camera/input restoration and reduced health by 25 through the existing Player System.

## 9. Player hit-reaction asset findings

The production player Blueprint is `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter`. Its active skeletal mesh is `/Game/Mage/Mesh/SK_Mage` with skeleton `/Game/Mage/Mesh/SKE_Mage`.

The only named player-style hit reactions found were:

- `/Game/CombatSystem/Animaitons/HitReact_Left`
- `/Game/CombatSystem/Animaitons/HitReact_Right`

Both use `/Game/Characters/Mannequins/Meshes/SK_Mannequin`, not the production Mage skeleton. They were not integrated because they are not directly production-compatible and no approved retargeted montage exists. No hit reaction is played by the catch cinematic.

## 10. Player death-animation asset findings

`/Game/Mage/Animations/Anim_Mage_Death_Forward` uses the correct Mage skeleton, is 3.67 seconds long, is non-additive, and has root motion disabled. It was not integrated in this rework because the active production Animation Blueprint exposes no verified death state or montage slot for it, and the current Player System respawn timing is shorter than the animation. Forcing the sequence from Isabel would couple AI to player presentation and risk breaking the established death/respawn flow.

A future player-animation task should add an approved death state/montage through the Player System without duplicating death logic.

## 11. Test results

### Build and compile

- `SOTM1Editor Win64 Development`: succeeded.
- UnrealHeaderTool: succeeded.
- Modified controller and notify sources: compiled and linked successfully.
- Editor-only Blueprint compile after PIE stopped: `/Game/AI/BP_AI`, `BP_MenuSystemCharacter`, `WBP_HealthBar`, and `/Game/BP_ForestPortal` completed with no Blueprint compiler errors.
- A handled delegate ensure occurred only when the player Blueprint was initially recompiled while PIE was active. The test was discarded and repeated after PIE stopped; the clean editor-only compile produced no ensure or compiler error.

### Catch and damage tests

| Test | Result | Captured evidence |
|---|---|---|
| First catch at 100 health | Pass | JumpScare state, health 100, cinematic camera active, JumpScare input lock active |
| Jump-scare impact | Pass | Presentation-impact log; health unchanged |
| Animation and scream | Pass for execution | Existing montage reported 1.97 seconds and existing scream call executed; final subjective timing/volume remains a manual visual/audio acceptance item |
| Camera restore | Pass | JumpScare -> Chase after 0.27-second restore delay; view target returned to player and locks became empty |
| Normal attack after scare | Pass | Attack started only after restore; each successful hit applied 25 damage through Player System |
| Remain in range | Pass | Repeated normal attacks; no repeated jump scare in the same encounter |
| Small escape | Pass | Encounter flag remained true; Isabel chased back and attacked without another jump scare |
| Genuine escape | Pass | `SearchLastKnown`, health 100, target none, encounter flag false |
| Rediscovery | Pass | Later valid catch started a new jump scare once |
| Health reaches 25 | Pass | Health sampled at 25 in normal Attack with encounter flag still true; no health-triggered cinematic |
| Lethal normal attack | Pass | Four normal 25-damage hits produced 100 -> 75 -> 50 -> 25 -> 0 |
| Life consumption and Game Over | Pass | Start: health 100/lives 1. End: health 0/lives 0/dead true/Game Over true |
| Retry | Pass | Restored health 100, lives 5, dead false, Game Over false, encounter false, no input locks |
| Post-respawn eligibility | Pass | New sight/catch cycles logged a new one-per-encounter jump scare |

## 12. Regression results

- Patrol, Detection, Chase, Search, Return to Patrol: passed in PIE.
- Normal Attack montage and 25-damage notify: passed.
- Blue health bar data path: health values changed through the Player System; the production widget compiled cleanly.
- Death, lives, respawn, Game Over, Retry: passed.
- Camera takeover, camera restoration, and input lock release: passed.
- Mansion to Forest portal: passed. The production portal loaded `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`, and the production player pawn spawned.
- Forest enemies were not modified.
- No package or EXE was created.
- No files were staged or committed.

Manual visual/audio acceptance still recommended: play normally from a room approach, confirm the close framing does not clip a wall from every encounter angle, confirm scream loudness/timing, and confirm the player body is not the visual focus during the cinematic. Automation verified the state, camera target, input lock, montage/audio call, timing, health, and restoration but cannot judge subjective framing or audio mix.

## 13. Exact files modified

- `Source/SOTM1/Public/AI/SOTMIsabelAIController.h`
- `Source/SOTM1/Private/AI/SOTMIsabelAIController.cpp`
- `Source/SOTM1/Public/AI/SOTMIsabelJumpScareImpactNotify.h`
- `Source/SOTM1/Private/AI/SOTMIsabelJumpScareImpactNotify.cpp`
- `ProjectDocs/SOTM_Isabel_JumpScare_CatchTrigger_Report.md`

No Unreal asset was intentionally modified or saved.

## 14. Exact Git allowlist

Stage only these files for this task:

```text
Source/SOTM1/Public/AI/SOTMIsabelAIController.h
Source/SOTM1/Private/AI/SOTMIsabelAIController.cpp
Source/SOTM1/Public/AI/SOTMIsabelJumpScareImpactNotify.h
Source/SOTM1/Private/AI/SOTMIsabelJumpScareImpactNotify.cpp
ProjectDocs/SOTM_Isabel_JumpScare_CatchTrigger_Report.md
```

Do not stage anything under `Saved`, `Intermediate`, `DerivedDataCache`, `Binaries`, or screenshot output.

## 15. Suggested commit message

Title:

`Rework Isabel jump scare to trigger on first catch`

Description:

`Replace the 25-health lethal jump-scare trigger with a presentation-only camera scare on the first valid catch of each chase encounter. Preserve normal attack damage, reset the encounter only on genuine target loss/death/respawn, and resume Attack/Chase/Search after camera restoration.`

## 16. Short client update

Isabel's jump scare now plays when she first catches the player in a chase, not when health reaches 25. The cinematic itself causes no damage; after the camera and input return, Isabel's normal attacks deal the existing 25 damage and control death, lives, respawn, and Game Over through the Player System. The scare plays once per genuine encounter, does not repeat after a small step out of range, and becomes available again after a real escape or respawn. Editor build, Blueprint compile, PIE combat, Game Over/Retry, and Mansion-to-Forest regression checks passed. Final subjective camera framing and scream mix should receive a short human acceptance pass from several room angles.
