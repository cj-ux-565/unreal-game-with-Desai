# SOTM Client Demo — Final Acceptance and Packaging Report

Date: 2026-08-20  
Project: The Secrets of the Mansion — Chapter 1  
Engine: Unreal Engine 5.6  
Final status: **CLIENT DEMO FINAL ACCEPTANCE: CONDITIONAL**

This report records only observed results. Automated Development-only acceptance routes were used to prepare the 330-coin state and exercise isolated systems; it does not claim that all 330 coins were collected manually.

## 1. Clean build result

- `SOTM1Editor Win64 Development`: successful.
- `SOTM1 Win64 Development`: successful.
- Final `BuildCookRun`: successful, AutomationTool exit code `0`.
- Final incremental build/cook/stage/archive duration: 1 minute 35 seconds.
- Final IoStore: 2,310 packages in `SOTM1-Windows`; the production cook list remains limited to the approved Main Menu, Mansion, and CH1 maps.
- No Blueprint or C++ compile failure was introduced by this pass.

## 2. Phase 1 regression

Observed production route evidence:

- Main Menu loaded.
- Play and Single Player opened the save selection screen.
- Runtime normalization reported exactly four story save slots.
- New Game loaded `/Game/Mansion_GameStart`.
- The Mansion intro acquired its cinematic input lock.
- Timmy and Isabella presentation ran through the existing Phase 1 sequence.
- The scripted knockout used zero damage and did not consume a life.
- The Mansion transitioned to the production CH1 map.
- CH1 initialized the production `BP_MenuSystemCharacter`, third-person view, HUD, eight Cousins, Timmy station, and Phase 4 bridge.

Evidence: `Saved/FinalAcceptanceAudit/RuntimeLogs/Phase1_RealRoute_1080_PreFix.log` and `Saved/FinalAcceptanceAudit/Phase1/`.

The save-slot Area label was presentation-only normalized from prototype boss wording to `CHAPTER 1`; authoritative save data is not changed.

Esc/Tab Pause/Resume was not freshly completed in the final packaged run because the automation-launched desktop window did not remain reliably interactive. No duplicate Pause widget was observed in the completed earlier UI acceptance, but this exact final packaged interaction remains part of the conditional manual pass.

## 3. Opening cinematic visual review

The full opening route was watched during the interactive 1080p acceptance run. Camera progression, Timmy/Isabella presentation, blackout, zero-damage knockout, dragging transition, and restoration into CH1 were readable and completed without a life loss or input-lock leak.

The final package initially omitted `/Game/Cinematics/LS_SOTM_MansionIntro` because it is loaded by a C++ string path. `Config/DefaultGame.ini` now explicitly cooks `/Game/Cinematics`; UnrealPak listing confirms `LS_SOTM_MansionIntro.uasset` exists in the final IoStore.

The inherited Main Menu character pose/look was not redesigned in this pass.

## 4. Final voice/audio status

**Final client voice recordings were not present in the project and were not integrated.**

- 95 project-contained audio/media assets were inventoried.
- Existing project ambience, scream, and gameplay audio were preserved.
- The packaged XAudio2 device initialized at 48 kHz, stereo output.
- The project-contained Cousin scream remains `/Game/AI/Nightmare_scream_jumpscare_SFX`.
- A legacy media asset still contains `file://C:/Users/Marcus Gonzalez/Downloads/Isabella Jumpscare.mp4`. CH1 logs a failed validation for that unused external media source. No external file was copied, fabricated, or treated as approved client audio.

## 5. Phase 2 acceptance

Passed through focused Development acceptance:

- 330 unique production Forest Coin components validated.
- Five real overlap pickups increased available/lifetime/unique counts from 0 to 5.
- HUD and Collect All Coins objective updated on each pickup.
- Every successful pickup autosaved.
- Separate-process reload restored partial progress.
- Development-safe preparation reached 330/330; this was not represented as manual collection.
- Eight production Cousins initialized; 33 legacy normal enemy stacks remained disabled by the existing production bridge.
- Patrol, perception-driven detection, chase, lethal catch, jump scare impact, instant death, exactly one life loss, respawn, Game Over, and Retry paths were exercised.
- The acceptance guard prevents a scheduled fallback catch after a natural catch has already succeeded.

Evidence: `Phase2_Corrected_720.log` and the Phase 2 screenshots under generated `Saved` evidence.

## 6. Cousin visual/audio review

- Natural detection/chase and the catch transition were observed in the focused route.
- Catch used the existing global gate, applied authoritative lethal Player System damage once, and returned all Cousins to patrol after respawn.
- The project-contained scream reference is present.
- Screenshots confirmed the camera/HUD state before and after catch.

A fresh, uninterrupted client-style review of the complete chase → break line of sight → search/return → catch sequence with audible output was not captured in the final packaged session. This is included in the conditional manual/video blocker rather than falsely marked as a new automated pass.

## 7. Phase 3 acceptance

Passed:

- Timmy station initialized and exposed `E` interaction and `Q` boost input.
- Locked objective state and insufficient-funds result were exercised.
- Purchase cost: 250.
- Available Coins: 330 → 80.
- Lifetime Coins remained 330 and Collect All Coins remained completed.
- Duplicate purchase was rejected.
- Slot 1/Slot 2 state isolation was exercised.
- Speed Boost ownership persisted through death/respawn and separate-process Continue.

Evidence: `Saved/FinalAcceptanceAudit/RuntimeLogs/Phase3_720.log`.

## 8. Speed Boost play-feel result

Measured runtime values:

- Base `MaxWalkSpeed`: 600.
- Active: 840.
- Multiplier: 1.40x.
- Duration: 3 seconds.
- Cooldown: 10 seconds.
- A second activation while active was rejected.
- Cooldown reactivation was rejected.
- Normal speed restored to 600 and READY returned after cooldown.
- Activation was exercised near a natural Cousin chase; it did not grant invulnerability.

No tuning was changed.

## 9. Phase 4 acceptance

Passed in Editor acceptance and again in the corrected packaged Development build:

- Find Chest active.
- Chest interaction and opened state.
- Gate Key acquired and notification displayed.
- Reach Gate active.
- Gate requirement validation, unlock, and open.
- Demo Complete overlay.
- Buy Full Game presentation.
- Return to Main Menu through the production Player State route.
- No boss map or boss gameplay loaded.

Corrected packaged log markers: `START`, `CHEST pass=1`, `COMPLETE pass=1`, and `RETURN_TO_MAIN_MENU`.

## 10. Mandatory death-after-key result

Passed using a real production Cousin catch through the public Phase 2 API:

- Preconditions: Chest opened, Gate Key owned, Speed Boost owned, Reach Gate active.
- Catch started successfully.
- Lethal damage was applied once through the Player System.
- Lives decreased from 5 to 4 exactly once.
- Respawn completed.
- Chest, key, boost, 80 available Coins, 330 lifetime Coins, and Reach Gate state were preserved.

Evidence: `Saved/FinalAcceptanceAudit/RuntimeLogs/Phase4_DeathAfterKey_720.log`, final marker `PASS=1`.

The verification route is guarded by `#if !UE_BUILD_SHIPPING` and is not Shipping gameplay.

## 11. Gate ending result

The current gate ending satisfies the demo scope:

Gate unlock → Demo Complete promotional overlay → Buy Full Game / Coming Soon → Return to Main Menu.

No unfinished boss, combat, Spider Mother, credits, or Phase 5 content is exposed.

## 12. Separate-process Continue

Passed for four fresh-process states:

- A: partial Forest progress restored 5/330, five unique pickups, four lives.
- B: Speed Boost owned restored with 80 available / 330 lifetime Coins.
- C: Chest opened and Gate Key owned restored with Reach Gate active.
- D: Gate unlocked and Demo Complete restored without entering boss content.

Evidence: `Continue_A_Partial.log`, `Continue_B_SpeedBoost.log`, `Continue_C_GateKey.log`, and `Continue_D_Completed.log`.

## 13. Save-slot isolation

Two-slot Development data acceptance passed. Advanced state in Slot 1 did not leak into the fresh Slot 2 state. The presentation fix changes only visible legacy Area text, not stored Coins, objectives, boost, key, gate, or completion data.

## 14. 1080p / 720p UI result

- 1920×1080: Main Menu, four save slots, opening route, and CH1 HUD were captured/reviewed.
- 1280×720: Phase 2 HUD, Phase 3 station/status states, Gate Key notification, gate goal, and Demo Complete overlay were captured/reviewed.
- No major clipping was observed in reviewed screenshots.
- The corrected packaged Phase 4 run generated real 1280×720 screenshots for Find Chest, Gate Key, Reach Gate, and Demo Complete.

## 15. Uninterrupted Main Menu → Demo Complete result

The production Main Menu → four slots → New Game → Mansion → CH1 portion was run interactively. Phase 2, Phase 3, Phase 4, death-after-key, Continue, and packaged Phase 4 were then completed in focused routes.

One uninterrupted Main Menu → Demo Complete client-style playthrough was **not** completed in a single process. Development-safe state preparation supplied the 330-coin prerequisite in focused acceptance. This remains an exact conditional acceptance blocker.

## 16. Errors found and fixes

Fixed:

1. A natural Cousin catch could be followed by the acceptance fallback catch. Added a Development-only observed-catch guard.
2. Existing save slots exposed prototype `BOSS FIGHT / FIND THE KEY` Area text. Normalized presentation to `CHAPTER 1` without mutating saves.
3. Added a Development-only mandatory death-after-key acceptance route using existing public APIs.
4. Packaged builds omitted the existing Mansion intro sequence and Mage death animation because they were only string-loaded. Added their directories to `DirectoriesToAlwaysCook`. UnrealPak listing confirms both assets in the final IoStore.

Known non-blocking/inherited warnings:

- Mansion emits one handled texture-streaming build-data ensure: `LevelStreamingTextures.IsValidIndex(BuildInfo.TextureLevelIndex)`. It did not crash the prior Mansion → CH1 run, but the map should have streaming data rebuilt in a future environment cleanup.
- CH1 contains one nearly-zero-scale `BP_item` physics component warning.
- Legacy marketplace save enum collision warnings remain.
- The stale external Isabella media source fails validation; the current production Cousin presentation uses the project-contained system instead.
- `StructUtils` deprecation and older include-order warnings remain.
- First launch may show a Windows Firewall prompt even though the demo does not require network play.

## 17. Packaging result

**PASS.** Windows Development build, cook, stage, Pak/IoStore, prerequisites, and archive all completed with exit code 0.

Final package contains the approved production maps:

- `/Game/Main_Menu_Map`
- `/Game/Mansion_GameStart`
- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

Marketplace asset dependencies referenced by those maps were cooked, but unrelated showcase maps were not added to `MapsToCook`.

## 18. Packaged executable path

`C:\UE Projects\CURRENT GAME\CURRENT GAME 5.6\Packaged\Demo\Windows\SOTM1.exe`

Runtime binary:

`C:\UE Projects\CURRENT GAME\CURRENT GAME 5.6\Packaged\Demo\Windows\SOTM1\Binaries\Win64\SOTM1.exe`

## 19. Packaged smoke-test result

Verified:

- Executable launched with D3D12/SM6 on the RTX 5060 Laptop GPU.
- Main Menu map loaded and rendered.
- Corrected package contains both formerly missing string-loaded assets.
- Mansion map loaded and initialized Isabel/player subsystems.
- Mansion production route previously completed into CH1.
- Corrected packaged CH1 initialized HUD, Timmy/Phase 3, Cousin/Phase 2, and Phase 4.
- Corrected packaged Phase 4 passed Chest → Key → Gate → Demo Complete → Main Menu.
- Audio device initialized successfully.

Not freshly verified through interactive packaged input: four-slot clicks, Esc/Tab Pause/Resume, natural Coin pickup, and audible human review. The tool-launched desktop was intermittently non-interactive and also surfaced OS dialogs. These are included in the single conditional manual packaged-playthrough blocker.

## 20. Gameplay video path

No compliant client gameplay MP4 was produced.

## 21. Video/audio stream verification

**VIDEO RECORDING: BLOCKED BY AUDIO CAPTURE**

Because no clean gameplay MP4 was produced, there is no video/audio stream, duration, resolution, FPS, codec, or channel result to report. Generated packaged screenshots are retained under `Saved/FinalAcceptanceAudit/PackagedBuild/` and `PackagedBuildCorrected/`; they are evidence, not a substitute falsely labeled as the required video.

## 22. Original save restoration/hash result

- Original backup contained 23 files.
- Restored folder: `Saved/SaveGames`.
- SHA-256 comparison: 23/23 matched.
- Mismatches: 0.
- Extra files: 0.
- Temporary Editor acceptance saves removed.
- Packaged-runtime test saves/logs/screenshots were removed from the deliverable after evidence was copied to ignored `Saved/FinalAcceptanceAudit`.

## 23. Known remaining non-blocking issues

- Rebuild Mansion texture-streaming data to remove the handled ensure.
- Remove or disconnect the unused stale external Isabella File Media Source in an Editor asset-maintenance pass.
- Repair the CH1 zero-scale `BP_item` component.
- Clean legacy marketplace enum-collision warnings.
- Consider suppressing the Windows Firewall prompt by removing unused network-enabled dependencies/configuration after confirming no menu feature needs them.

## 24. Exact production files modified

1. `Config/DefaultGame.ini`
2. `Source/SOTM1/Private/Demo/SOTMDemoPhase1WorldSubsystem.cpp`
3. `Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp`
4. `Source/SOTM1/Private/Demo/SOTMDemoPhase4WorldSubsystem.cpp`
5. `Source/SOTM1/Public/Demo/SOTMDemoPhase4WorldSubsystem.h`
6. `ProjectDocs/SOTM_Demo_Final_Acceptance_Packaging_Report.md`

No Unreal `.uasset` or `.umap` was modified in this pass.

## 25. Exact Git allowlist

Stage only:

```text
Config/DefaultGame.ini
Source/SOTM1/Private/Demo/SOTMDemoPhase1WorldSubsystem.cpp
Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp
Source/SOTM1/Private/Demo/SOTMDemoPhase4WorldSubsystem.cpp
Source/SOTM1/Public/Demo/SOTMDemoPhase4WorldSubsystem.h
ProjectDocs/SOTM_Demo_Final_Acceptance_Packaging_Report.md
```

Do not stage `Saved`, `Packaged`, `Binaries`, `Intermediate`, `DerivedDataCache`, generated logs/screenshots, or test saves.

## 26. Suggested commit title

`Finalize Phase 1-4 demo acceptance and package required cinematic assets`

Suggested description:

```text
- normalize four-slot Chapter 1 presentation
- prevent duplicate Development acceptance catches
- add Development-only death-after-key regression coverage
- guarantee Mansion intro and player death assets are cooked
- document build, persistence, package, and remaining manual acceptance
```

## 27. Suggested demo tag

`demo-phase4-client-review-v1`

Apply the tag only after the remaining manual packaged playthrough and audio-bearing client video are accepted.

## 28. Short client update

The Phase 1-4 demo systems compile, package, and pass focused acceptance, including four-slot isolation, Coin persistence, Cousin lethal catch, Speed Boost purchase/use, Chest/Key/Gate progression, death-after-key persistence, Demo Complete, and return to Main Menu. The final Windows Development package launches, and missing string-loaded cinematic/death assets were added to the cook. Final client voice recordings were not present and were not fabricated. Delivery remains conditional only on one uninterrupted manual packaged playthrough and a clean gameplay recording with captured game audio.

## Actual remaining blockers

1. Complete one uninterrupted interactive packaged playthrough from Main Menu through Demo Complete, including four-slot clicks and Esc/Tab Pause/Resume.
2. Record and validate a clean client gameplay MP4 with game audio covering the required route.

**CLIENT DEMO FINAL ACCEPTANCE: CONDITIONAL**
