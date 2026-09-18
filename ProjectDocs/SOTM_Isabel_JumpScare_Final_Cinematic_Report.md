# SOTM Isabel Jump Scare — Final Cinematic Polish Report

Date: 2026-08-08  
Project: *The Secrets of the Mansion — Chapter 1*  
Scope: Client Task 2 final client-facing cinematic presentation pass

## 1. Client requirement addressed

This pass polishes the already-approved catch-based Isabel flow. A valid living production player must be in Isabel's existing catch range with line of sight during the active chase encounter. The jump scare remains a zero-damage cinematic, restores gameplay, and hands control back to the existing normal-attack and Player System paths.

No coin, objective, skill, Timmy, chest/key, boss, Purple Burst, unrelated cutscene, Forest-enemy, or HUD-redesign work was performed.

## 2. Existing architecture preserved

The Phase 1 native state machine and Phase 2 normal-attack implementation in `ASOTMIsabelAIController` remain authoritative. The approved `Patrol -> Chase -> JumpScare -> Chase/Attack` route was extended in place; no second AI controller, health model, death model, or camera framework was introduced.

The catch still uses the existing encounter gate, target validation, range and line-of-sight rules. The cinematic still applies no damage. Normal montage impact continues through the Player System, which remains authoritative for health, invulnerability, death, life decrement, respawn, Game Over, and Retry.

## 3. Camera choreography

The transient cinematic camera now has four explicit phases: Catch, Anticipation, Impact, and Recovery. It blends from the player view, begins at a 68-degree anticipation FOV, performs a restrained 16 cm push toward Isabel, punches to a 50-degree impact FOV, then eases back before the existing camera-return blend.

The existing bounded 30 Hz cinematic timer performs interpolation only while the jump scare is active. There is no permanent camera tick and no actor search inside the update.

## 4. Camera effects implemented

At the montage impact notify, the transient camera receives a short 0.24-second effect envelope:

- vignette up to 0.52;
- exposure bias down to -0.40;
- chromatic aberration up to 1.10;
- brief decaying location and rotation impulse;
- synchronized FOV punch.

All values are exposed as editable controller properties for later human tuning. Overrides are cleared when the cinematic completes, aborts, the controller unpossesses, the world ends, or death/respawn cleanup runs.

No persistent post-process volume or global renderer setting was modified.

## 5. Isabel positioning

The existing collision-aware camera placement and Isabel focus point are retained. Before the montage begins, Isabel is smoothly rotated to face the cinematic camera. Rotation adjustment stops once the montage is playing so controller rotation does not fight animation playback.

A captured PIE catch frame confirms the transient CameraActor is active and both player/Isabel are present during the blend. Final face/body composition remains a subjective human acceptance item because a still frame cannot validate motion, perceived threat, or final timing.

## 6. Animation integration

Reused production montage:

- `/Game/AI/AM_Isabel_JumpScare_Phase3`
- duration observed: approximately 1.97 seconds;
- compatible with Isabel's Cruel Doll skeleton.

The montage plays once per approved encounter. Its existing dedicated impact notify remains the synchronization point. The normal attack montage and locomotion Animation Blueprint were not replaced or redesigned.

## 7. Sound design/integration

Reused production sound:

- `/Game/AI/Nightmare_scream_jumpscare_SFX`
- observed asset duration: approximately 6.478 seconds.

The scream previously began at cinematic start. It now starts exactly at the montage impact notify using a transient 2D audio component, aligning the primary sound hit with the FOV punch, post-process envelope, and camera impulse. It receives a short fade-out during cleanup.

No suitable secondary stinger or safe project-wide ducking mix was found. Weapon-impact sounds were rejected as tonally inappropriate. No external audio was imported.

## 8. Player Hit Reaction findings/implementation

No player hit reaction was added. Existing `HitReact_Left` and `HitReact_Right` assets target the Mannequin skeleton, while the production player mesh is `/Game/Mage/Mesh/SK_Mage`. Using them would be incompatible without retargeting and visual approval. Normal damage and the blue health-bar response remain unchanged.

## 9. Player Death animation findings/implementation

`Anim_Mage_Death_Forward` matches the Mage skeleton, but the active production player Animation Blueprint is `/Game/SuperPowers/Powers/Speedster/Anim/Default/AnimBP/ThirdPerson_AnimBPSpeedster`, which targets the inherited mannequin animation setup and has no verified death slot/state integration. Forcing the sequence would risk breaking locomotion or producing an unblended pose.

Therefore no unverified death animation was wired in this pass. This is an asset/integration dependency, not a false completion claim.

## 10. Smooth death-transition implementation

When a normal Isabel attack is confirmed lethal by the existing Player System, the PlayerCameraManager begins a short black fade from 0.0 to 0.85 over 0.30 seconds. The fade bridges into the existing death/respawn or Game Over presentation without changing damage, health, lives, input locks, timers, or state ownership.

On Player System respawn/Retry, the fade returns smoothly over 0.40 seconds. Teardown also clears it safely.

## 11. Respawn/Game Over integration

PIE validation with one remaining life produced health 0, lives 0, `PlayerDead=true`, `GameOver=true`, and the existing `Death` and `GameOver` input locks. Isabel stopped attacking and returned to Idle. Calling the existing Retry path restored health 100, lives 5, cleared death/Game Over and all locks, restored the player view, and left move/look input enabled.

No independent respawn, life, or Game Over logic was added.

## 12. Repeatability tests

Three catch/cinematic executions were exercised in Mansion PIE. Runtime evidence showed each valid execution entering `JumpScare`, acquiring the transient CameraActor, holding the `JumpScare` input lock, reaching the montage impact notify, playing the synchronized effect/audio event, restoring the camera, and returning to Chase/Attack.

Observed repeatability evidence:

| Check | Result |
|---|---|
| Jump scare once in an active encounter | Passed; a second development request during the same encounter was rejected |
| Zero cinematic damage | Passed; health remained 100 through the cinematic impact |
| Normal attack after cinematic | Passed; each successful attack applied 25 through the Player System |
| Escape from attack range | Passed; Attack returned to Chase when separation was created |
| Death and respawn | Passed; one lethal sequence consumed one life and reset the encounter gate |
| New encounter after respawn | Passed; a new catch could trigger again |
| Game Over and Retry | Passed; state, view, input, health, and lives restored through the existing APIs |
| Persistent camera/FOV/effects/input lock | None observed after completion, respawn, Retry, or map travel |

The development-only `ForceDevelopmentCatchForCinematicTest` helper was added solely to make repeatable PIE presentation possible when editor automation cannot emit a fresh AI Perception sight stimulus. It still requires the real living production player, actual attack range, and true line of sight, and calls the same `BeginJumpScare` path. In Shipping it returns false.

## 13. Full regression results

| Area | Result | Evidence/limitation |
|---|---|---|
| Idle, Patrol, Chase, Attack | Passed | State transitions and normal attacks observed in PIE |
| Detection/Search/Return | Preserved | No perception/search code was changed; prior Phase 1/acceptance evidence remains applicable |
| Catch predicates | Passed | Helper refused invalid/repeated encounter; real range and LOS were required |
| Jump-scare animation, impact and sound event | Technically passed | Montage/notify/audio/effect logs observed; perceived synchronization still needs human judgment |
| Jump-scare damage | Passed | Zero damage at cinematic impact |
| Normal attack/player damage | Passed | Four 25-damage attacks reached lethal health in validation |
| Health/lives/death/respawn | Passed | Player System values verified before/after |
| Game Over/Retry | Passed | Existing flow restored health/lives/view/input |
| Main Menu | Partially automated | Production map and title-screen widgets loaded and rendered; the bridge could not reliably activate the real Blueprint Play/New Game button with pointer focus |
| Mansion gameplay | Passed | Production character movement and Isabel encounters exercised |
| Mansion -> Forest | Passed | Real `/Game/BP_ForestPortal` changed PIE to production `CH1`; production player spawned and moved |
| Blueprint compile | Passed | `BP_AI`, `ABP_AI`, production player, health/in-game UI, intro/title/container widgets compiled without compiler errors |
| C++ compile | Passed | `SOTM1Editor Win64 Development` completed successfully after Editor restart |
| Package | Not run | Explicitly excluded by task |

Known inherited runtime warnings remain: legacy `/Game/AI/BP_AI` polls a missing Blackboard value, and `WBP_SaveSlotBase` can access null `AreaTextRef`/`AreaNameTextRef` references. The Blackboard warning is documented in prior Isabel reports; neither warning originates in the new cinematic controller code. They should be handled as separate legacy Blueprint cleanup rather than hidden in this presentation pass.

No instrumented before/after performance capture was requested or collected for this polish pass, so no FPS or frame-time claim is made. The implementation adds only the already-bounded cinematic timer while the sequence is active and no permanent Event Tick work.

## 14. Remaining subjective human checks

Run PIE in `Mansion_GameStart`, reach Isabel normally, and judge one complete encounter at normal play speed:

1. Confirm Isabel's face/body is centered and frightening at the impact moment.
2. Confirm the initial blend and 16 cm push feel smooth.
3. Confirm the impact impulse is strong enough without causing discomfort.
4. Confirm the FOV punch, vignette, exposure, and fringe improve the scare rather than obscuring Isabel.
5. Confirm the scream begins on the visible animation impact and its volume is appropriate.
6. Confirm the camera returns without a visible snap and movement/look resume immediately.
7. Allow normal attacks to reduce the blue health bar and confirm its visual response.
8. Allow a lethal attack; judge whether the short fade is natural without a dedicated death animation.
9. Confirm respawn fades back smoothly and Isabel does not deliver unavoidable damage during protection.
10. Repeat after respawn to confirm the cinematic feels consistent.

Subjective values are intentionally left exposed for one client-guided tuning pass rather than repeatedly changed without human feedback.

## 15. Missing production assets

- No compatible, approved player hit-reaction animation for the Mage production animation setup.
- No verified player death state/slot in the active production Animation Blueprint.
- No suitable secondary horror stinger identified.
- No dedicated CameraShake asset identified; a short transient deterministic impulse is used instead.
- No safe, dedicated ambient-ducking Sound Mix identified.

## 16. Exact files created/modified

Files changed specifically by this final cinematic polish:

- `Source/SOTM1/Public/AI/SOTMIsabelAIController.h`
- `Source/SOTM1/Private/AI/SOTMIsabelAIController.cpp`
- `ProjectDocs/SOTM_Isabel_JumpScare_Final_Cinematic_Report.md`

Pre-existing uncommitted catch-trigger work remains in the working tree and was not created by this pass:

- `Source/SOTM1/Public/AI/SOTMIsabelJumpScareImpactNotify.h`
- `Source/SOTM1/Private/AI/SOTMIsabelJumpScareImpactNotify.cpp`
- `ProjectDocs/SOTM_Isabel_JumpScare_CatchTrigger_Report.md`

Generated screenshots/logs under `Saved/` are evidence only and must not be committed.

## 17. Exact Git allowlist

For one combined commit containing the approved catch-trigger rework plus final cinematic polish, allow only:

```text
Source/SOTM1/Public/AI/SOTMIsabelAIController.h
Source/SOTM1/Private/AI/SOTMIsabelAIController.cpp
Source/SOTM1/Public/AI/SOTMIsabelJumpScareImpactNotify.h
Source/SOTM1/Private/AI/SOTMIsabelJumpScareImpactNotify.cpp
ProjectDocs/SOTM_Isabel_JumpScare_CatchTrigger_Report.md
ProjectDocs/SOTM_Isabel_JumpScare_Final_Cinematic_Report.md
```

Do not stage `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, screenshots, logs, or any other modified asset. Nothing was staged or committed automatically.

## 18. Suggested commit title

`Polish Isabel catch jump-scare cinematic`

## 19. Suggested commit description

`Preserve the catch-based Isabel AI flow while adding phased camera choreography, impact-synchronized scream and transient camera effects, controlled lethal-attack fade integration, repeatable Development-only validation, and final PIE verification documentation.`

## 20. Short client-facing completion update

Isabel's approved catch-based jump scare now has a smoother camera takeover, anticipation push, synchronized animation/scream impact, restrained horror effects, safe camera/input restoration, and a smoother transition from lethal normal attacks into the existing Player System death flow. The cinematic remains zero-damage and once per encounter; normal attacks still own health loss. Death, one-life loss, respawn, Game Over, Retry, and the Mansion-to-Forest route were verified in PIE. Final framing, intensity, and sound-volume preference require one brief human visual review before client sign-off.
