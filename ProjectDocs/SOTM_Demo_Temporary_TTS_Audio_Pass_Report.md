# SOTM Demo — Temporary TTS, Cinematic and Gameplay Audio Pass

Date: 20 August 2026  
Engine: Unreal Engine 5.6.1  
Branch inspected: `main`  
Starting checkpoint: `7bd8d819` (`World System`)  
Source-control action: none — nothing was staged or committed.

## Result

The temporary audio implementation is complete and the Windows Development package builds successfully. The packaged executable reaches the production Main Menu, the Mansion introduction plays all nine connected Timmy/Isabella/Forest-arrival lines in order, timing advances from each SoundWave's real duration, the route reaches CH1, Forest ambience is normalized, and cooked Cousin voices play with matching duration-bound subtitles.

The generated speech is explicitly temporary placeholder VO. It uses locally installed Microsoft Windows system voices and does not clone or imitate a performer. Source WAVs, manifests, generation scripts and Unreal SoundWave assets are isolated under clearly named `Temporary` directories for clean replacement.

Subjective listening approval remains manual: the automation could prove that the physical audio device initialized, the expected SoundWaves played, subtitles matched, and no audio asset was missing, but it cannot personally hear the speakers or judge tone, intelligibility, loudness balance or taste. Those items are not estimated or falsely marked passed.

## Sources reviewed

- `ProjectDocs/CHAPTER 1 — Voice Actor Lines.pdf` — all 2 pages, text extraction and rendered-page inspection.
- `ProjectDocs/Mansions Quests_ Chapter 1.pdf` — all 8 pages, text extraction and rendered-page inspection.
- `ProjectDocs/Client_Requirements.txt`.
- `ProjectDocs/SOTM_Client_Demo_Phase1_Report.md`.
- `ProjectDocs/SOTM_Final_Demo_Acceptance_Report.md`.
- Current production audio assets, maps and C++ orchestration for Demo Phases 1–4.

Client wording was preserved in every generated spoken line. No new story dialogue was invented.

## Temporary voice generation

- Timmy: `Microsoft David Desktop`.
- Isabella and Cousins: `Microsoft Zira Desktop`.
- Format: 48 kHz, 16-bit PCM, mono WAV.
- Dialogue files: 21.
- Imported SoundWave assets: 21.
- Total source dialogue size: 8,397,558 bytes.
- Per-file text, voice, rate, format and SHA-256 are recorded in `SourceAudio/Dialogue/Chapter1/Temporary/TemporaryDialogueManifest.json`.
- Regeneration and replacement instructions are in `SourceAudio/Dialogue/Chapter1/Temporary/README.txt`.

Connected dialogue:

- Mansion: Timmy introduction, warning and danger line.
- Mansion: Isabella arrival, taunts and dragging line.
- Forest transition: Timmy Forest Domain and collectible explanation.
- Forest: five occasional Cousin whispers and four detection calls.

Timmy upgrade/chest source lines were generated and imported for approved future use, but were not injected into the current station/chest flow because those flows did not have an approved matching dialogue/subtitle beat. Their required interaction feedback is supplied by SFX. This avoids adding unrequested story timing or playing spoken VO without subtitles.

## Temporary SFX generation

Ten project-owned procedural placeholder SFX were generated locally from deterministic mathematical waveforms/noise:

- Knockout impact
- Respawn
- Station open
- Denied action
- Upgrade success
- Chest open
- Key acquired
- Gate locked
- Gate open
- Demo complete

They contain no downloaded recording, commercial music, performer voice or third-party source audio. Source and imported assets are isolated under `SourceAudio/SFX/Temporary` and `/Game/Audio/SFX/Temporary`.

Existing suitable project audio was reused where available:

- Mansion bed: `Abyssal_Cue`, at a deliberately low level.
- Isabella jump-scare: existing `Nightmare_scream_jumpscare_SFX` preserved.
- Dragging: existing footsteps cue.
- Speed Boost: existing `WindGust_Cue`.
- Main Menu/UI audio and coin pickup audio were already implemented and were not duplicated.

## Cinematic integration

The existing `LS_SOTM_MansionIntro` sequence and production Phase 1 flow were retained. The old fixed timeline was replaced with a duration-driven dialogue sequence:

1. Matching speaker and subtitle appear.
2. Temporary SoundWave starts.
3. `UAudioComponent::OnAudioFinished` advances to the next beat.
4. A short 0.22-second breathing gap separates dialogue beats.
5. Knockout uses a dedicated 0.85-second impact bridge.
6. The Forest transition occurs only after the final Timmy line completes.

Mansion ambience is gently ducked while dialogue plays, faded for the knockout, and cleaned up safely when the world changes. Existing camera, animation, scream, fade, input lock and map travel behavior remain in control.

The non-Shipping command-line switch `-SOTMTempAudioAcceptance` can force the Mansion intro for repeatable Development verification. It is compiled out of Shipping behavior.

## Forest integration

No Forest map or lighting asset was changed or resaved. At runtime, the audio pass normalizes the six existing ambience/music components to prevent the prior stack from masking gameplay:

- Horror ambience: 0.18
- Wind: 0.18
- Sinister whispers: 0.16
- Dark Descent: 0.12
- Dreadful Lullaby: 0.10
- Ambient birds: 0.25

Cousin spoken clips are spatialized. Occasional whispers are scheduled every 12–20 seconds rather than looping constantly. Detection calls are gated by the existing one-warning-per-encounter set. Every Cousin spoken line now creates a temporary bottom-center subtitle overlay with `COUSIN`, exact matching text, and removal from `OnAudioFinished`. The active audio component and overlay are stopped/removed safely on overlap or world teardown.

The existing jump-scare scream remains synchronized through the established catch flow. Respawn now has a short feedback cue. No Cousin sensing, chase, catch, damage, death or respawn mechanics were redesigned.

## Upgrade, chest, key, gate and completion feedback

- Upgrade UI open: subtle station cue.
- Insufficient/denied purchase: restrained denial cue.
- Successful purchase: short dark-fantasy reward cue.
- Speed Boost active: existing wind cue; it fades when boost ends and is stopped on teardown.
- Chest open: spatial chest cue.
- Key acquired: reward cue 0.52 seconds after chest opening.
- Locked chest/gate: spatial locked feedback.
- Gate success: spatial opening cue.
- Demo Complete: short completion cue.

All calls are attached to existing successful/failure state transitions. No objective, currency, ability, save, chest, gate or demo-completion rule was changed.

## Verification

### Source audio

- 21/21 dialogue WAVs validated as 48 kHz, 16-bit PCM, mono.
- 10/10 SFX WAVs validated as 48 kHz, 16-bit PCM, mono.
- 21 dialogue and 10 SFX SoundWave assets imported.
- Final cook contains 21 dialogue and 10 SFX `.uasset` packages plus their cooked bulk/exports.

### Compilation

- Final `SOTM1Editor Win64 Development`: succeeded.
- Final `SOTM1 Win64 Development`: succeeded.
- No Blueprint asset was changed by this pass.
- A full `CompileAllBlueprints` audit still exits with inherited errors in exactly seven untouched assets:
  - `/Game/Blueprints/BP_WEAPON`
  - `/Game/LazyDevAac990ce745b2V1/.../BP_DemoCharacter`
  - `/Game/MenuSystemPro/.../WBP_TitleScreenMenu`
  - `/Game/MenuSystemPro/.../WBP_AudioSettings`
  - `/Game/MenuSystemPro/.../WBP_ControlsSettings`
  - `/Game/MenuSystemPro/.../WBP_GameSettings`
  - `/Game/MenuSystemPro/.../WBP_VideoSettings`

These failures predate and are unrelated to the audio pass. They were not repaired because this task prohibited unrelated gameplay/menu redesign. Therefore this report does not claim a clean global Blueprint compile.

### Editor runtime

Focused Mansion test with the real 48 kHz XAudio2 device:

- Physical device initialized: `Speakers (Realtek(R) Audio)`.
- All nine connected cinematic/transition clips loaded and emitted playback markers.
- Logged durations ranged from 2.04 to 18.65 seconds.
- Each logged subtitle exactly matched the connected spoken text.
- Mansion intro completed and travelled to production CH1.
- CH1 initialized and normalized exactly six ambience/music components.
- No `TEMPORARY PLACEHOLDER VO unavailable`, fatal error or assertion occurred.

Evidence: `Saved/TemporaryAudioPass/EditorRuntimeAudioAcceptance.log`.

### Windows Development package

- `BuildCookRun`: success.
- Cook/stage/archive time on final run: 1 minute 43 seconds.
- Cooked packages: 2,341.
- IoStore chunks: 4,758 project-container chunks.
- Final main container: approximately 2.04 GiB.
- Archive: `Packaged/Demo/Windows`.
- Production Main Menu visibly launched; evidence screenshot: `Saved/TemporaryAudioPass/PackagedMainMenu.png`.
- Packaged Mansion acceptance route loaded all nine clips, completed the intro and reached CH1.
- Final packaged Forest check played two cooked Cousin clips and logged their exact subtitles for their SoundWave durations:
  - `He can’t escape…` — 2.52 seconds.
  - `Let’s play chase…` — 2.34 seconds.
- No missing temporary audio, fatal error or assertion appeared in the focused packaged logs.

Evidence:

- `Saved/TemporaryAudioPass/PackagedAudioAcceptance.stdout.log`
- `Saved/TemporaryAudioPass/PackagedForestAudio.stdout.log`
- `Saved/TemporaryAudioPass/PackagedMainMenu.png`

### Manual listening still required

Automation cannot hear or judge the output. Before client delivery, perform one human pass with headphones/speakers:

1. Launch `Packaged/Demo/Windows/SOTM1.exe`.
2. Start a fresh slot and listen through Mansion → Forest.
3. Confirm dialogue is intelligible, subtitle timing feels natural, ambience does not mask speech, and the knockout/scream is not excessively loud.
4. Trigger a Cousin detection/catch and confirm spatial direction, subtitle readability, jump-scare impact and respawn cue.
5. Open Timmy's station; test denied purchase, success and Speed Boost.
6. Open the chest, acquire the key, test locked/successful gate feedback, and reach Demo Complete.
7. Adjust only local volume constants if subjective balance needs tuning.

## Known warnings and risks

- Cook still reports a nearly-zero-scale `BP_item` physics body in CH1. It predates this audio pass.
- Cook reports hidden dependencies from marketplace menu/bird assets; these affect cook hygiene, not the new audio.
- `StructUtils` is deprecated in the current plugin dependency graph.
- The project contains an unused stale `Isabella_Jumpscare` external-media reference to another developer's Downloads folder. The production audio path does not use it; it was left untouched as instructed.
- The full cook approached the 16 GB machine's free-memory threshold and triggered normal cooker garbage collection, but completed successfully.
- Final actor-relative mix and spatial attenuation require human listening on the intended client hardware.

## Exact intentionally modified files

### Config and source

- `Config/DefaultGame.ini`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase1WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase1WorldSubsystem.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase2WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase3WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase3WorldSubsystem.cpp`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase4WorldSubsystem.cpp`
- `ProjectDocs/SOTM_Demo_Temporary_TTS_Audio_Pass_Report.md`

### Imported dialogue SoundWaves

- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Mansion_001.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Mansion_002.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Mansion_003.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Isabella_Mansion_001.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Isabella_Mansion_002.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Isabella_Mansion_003.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Isabella_Mansion_004.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Forest_001.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Forest_002.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Upgrade_001.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Upgrade_002.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Timmy_Chest_001.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_001.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_002.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_003.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_004.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Whisper_005.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_001.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_002.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_003.uasset`
- `Content/Audio/Dialogue/Chapter1/Temporary/VO_TEMP_Cousin_Detect_004.uasset`

### Imported SFX SoundWaves

- `Content/Audio/SFX/Temporary/SFX_TEMP_ChestOpen.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_DemoComplete.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_Denied.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_GateLocked.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_GateOpen.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_KeyAcquired.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_KnockoutImpact.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_Respawn.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_StationOpen.uasset`
- `Content/Audio/SFX/Temporary/SFX_TEMP_UpgradeSuccess.uasset`

### Dialogue source files

- `SourceAudio/Dialogue/Chapter1/Temporary/GenerateTemporaryDialogue.ps1`
- `SourceAudio/Dialogue/Chapter1/Temporary/README.txt`
- `SourceAudio/Dialogue/Chapter1/Temporary/TemporaryDialogueManifest.json`
- All 21 same-named `VO_TEMP_*.wav` files represented exactly by the 21 imported dialogue assets above.

### SFX source files

- `SourceAudio/SFX/Temporary/GenerateTemporarySFX.ps1`
- `SourceAudio/SFX/Temporary/README.txt`
- `SourceAudio/SFX/Temporary/TemporarySFXManifest.json`
- All 10 same-named `SFX_TEMP_*.wav` files represented exactly by the 10 imported SFX assets above.

Generated directories (`Saved`, `Intermediate`, `DerivedDataCache`) and `Packaged/Demo` are verification output and must not be committed.

## Git allowlist

Stage only:

- `Config/DefaultGame.ini`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase1WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase1WorldSubsystem.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase2WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase2WorldSubsystem.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase3WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase3WorldSubsystem.cpp`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase4WorldSubsystem.cpp`
- `Content/Audio/Dialogue/Chapter1/Temporary/`
- `Content/Audio/SFX/Temporary/`
- `SourceAudio/Dialogue/Chapter1/Temporary/`
- `SourceAudio/SFX/Temporary/`
- `ProjectDocs/SOTM_Demo_Temporary_TTS_Audio_Pass_Report.md`

Do not stage `Saved/`, `Intermediate/`, `DerivedDataCache/` or `Packaged/`.

Suggested commit title:

`Add temporary Chapter 1 TTS and demo audio pass`

Suggested description:

`Generate isolated placeholder VO/SFX, drive Mansion cinematic timing from audio completion, add synchronized subtitles and Forest mix, and connect feedback to existing upgrade, chest, gate and demo-complete flows.`

## Final scope confirmation

- No production map asset was modified or resaved.
- No lighting value or rendering setting was changed.
- No AI detection/chase/catch mechanic was changed.
- No objective, coin, progression, save, ability or gate rule was changed.
- No boss or future Phase 5 content was implemented.
- Nothing was deleted, staged or committed.
