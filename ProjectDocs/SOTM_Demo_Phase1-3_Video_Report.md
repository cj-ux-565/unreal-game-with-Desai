# SOTM Demo Phase 1–3 Video Recording Report

Date: 18 August 2026  
Requested output: `Saved/ClientVideos/SOTM_Demo_Phase1-3_Gameplay.mp4`

## Status

**VIDEO RECORDING BLOCKED: UNREAL RUNTIME/COOK EXHAUSTED AVAILABLE SYSTEM VIRTUAL MEMORY**

No final client MP4 was produced or claimed. Xbox Game Bar audio/video capture itself was verified successfully, but the current machine could not keep the Unreal gameplay session running long enough to record the required Phase 1–3 playthrough. Producing or editing a partial/silent video would not satisfy the supplied success criteria.

## 1. Recording methods inspected

The requested preference order was followed:

1. Unreal-native capture: no configured interactive gameplay-and-game-audio recorder suitable for this playthrough was available.
2. Xbox Game Bar: installed (`Microsoft.XboxGamingOverlay 7.326.7271.0`) and enabled (`GameDVR_Enabled=1`). This was selected.
3. OBS: not installed.
4. System FFmpeg/ffprobe: not installed or available on `PATH`.
5. NVIDIA App overlay: installed, but opening it requested NVIDIA web authentication. It was not used because Game Bar was already installed, enabled, and proven to capture Unreal audio.

No software was downloaded or installed.

## 2. Xbox Game Bar capture proof

An eight-second setup capture of the real Unreal PIE Main Menu was created before attempting the complete recording. Windows' built-in media APIs reported:

- video stream: present;
- video codec: H.264;
- capture resolution: 2400×1440 (native desktop capture resolution);
- measured frame rate: approximately 29.4 FPS;
- audio stream: present;
- audio codec: AAC;
- audio channels: 2 (stereo);
- audio sample rate: 48,000 Hz;
- audio bitrate: 128,000 bps.

The audio track was decoded to PCM with Windows' built-in MediaTranscoder. Waveform measurement returned:

- peak sample amplitude: 28,006;
- RMS amplitude: 1,537.51.

Therefore the Game Bar result contained a real, non-silent audio signal. Audio capture itself was not the blocker.

The setup capture is not a final client deliverable because it contains only the Main Menu.

## 3. Clean build result

Before recording:

- PIE was stopped and Unreal Editor was closed.
- `SOTM1Editor Win64 Development` was built with the latest DLL.
- result: **Succeeded / target up to date**.

When the lower-memory packaged fallback became necessary, `SOTM1 Win64 Development` was built separately with UBA disabled and one compiler action:

- 12/12 actions completed;
- `SOTM1.exe` linked successfully;
- result: **Succeeded**.

## 4. Recording attempt 1 — Standalone Game

The production Main Menu was launched with:

- Unreal Standalone `-game` mode;
- 1920×1080 requested resolution;
- Development configuration;
- production Main Menu map.

Before a useful recording could begin, Unreal crashed in the NVIDIA/D3D12 driver path (`nvwgf2umx.dll` and `D3D12Core.dll`). No renderer setting or Forest-lighting setting was changed to hide this failure.

Result: failed before client footage could be recorded.

## 5. Recording attempt 2 — immersive Editor PIE

The production Main Menu was opened in PIE and switched to immersive mode. Shader preparation was allowed to settle. Xbox Game Bar successfully recorded the Unreal window with non-silent game audio.

During the attempted route, Unreal Editor terminated with:

```text
Ran out of memory allocating 350786 (0.3 MiB) bytes.
The paging file is too small for this operation to complete.
```

The attempted long clip did not progress beyond the menu/chapter presentation and is not valid client evidence.

Result: failed because the Editor exhausted available committed virtual memory.

## 6. Recording attempt 3 — temporary Development package

Packaging was not attempted initially. It was used only after Standalone and PIE both failed, under the specification's “unless absolutely required” exception, because a packaged executable normally has substantially lower memory overhead than Editor PIE.

Steps:

- built the Development game target successfully with one compiler process;
- started a full single-process Windows cook;
- kept the temporary archive under `Saved/ClientVideoBuild`;
- made no production gameplay, map, lighting, or renderer edits.

The cook failed while compiling production shaders/assets:

```text
ShaderCompileWorker failed with out-of-memory (OOM) exception.
AvailablePhysical: 1.68 GiB
AvailableVirtual: 1.08 GiB
```

AutomationTool exited with `Error_UnknownCookFailure` / exit code 25.

Result: no runnable current package was produced.

## 7. Machine resource evidence

Before the package attempt, the machine reported approximately:

- installed RAM: 15.71 GiB;
- free physical RAM: 2.15 GiB;
- configured page file: 49,152 MiB;
- free virtual memory: 9.63 GiB.

During the cook, Unreal's available committed virtual memory dropped to 1.08 GiB and ShaderCompileWorker terminated. The first UAT build attempt also stalled under memory pressure, so it was stopped and replaced by the successful single-process non-UBA build.

No Windows page-file, security, GPU-driver, or renderer settings were modified automatically.

## 8. Final video metadata

No final file exists at:

`Saved/ClientVideos/SOTM_Demo_Phase1-3_Gameplay.mp4`

Consequently there is no final duration, resolution, FPS, codec, channel count, synchronization result, or gameplay-section acceptance result to report. The technical metadata above applies only to the short Game Bar capture-method test.

## 9. Gameplay sections captured

The complete required client sequence was **not captured**. The setup/failed attempts visually reached only:

- production Main Menu;
- Chapter presentation during the aborted PIE route.

The following required sections are not claimed as captured in one client-reviewable video:

- four-slot screen and completed New Game selection;
- full Mansion cinematic, Timmy, Isabella, knockout, and dragging transition;
- Forest entry and normal movement;
- five visible real Coin pickups;
- natural Cousin patrol/chase/jump scare/death/respawn;
- 329/330 to 330/330 completion;
- Timmy Upgrade Station purchase and 330 -> 80 balance change;
- Speed Boost READY/ACTIVE/COOLDOWN gameplay;
- Esc and Tab pause demonstrations.

Previously generated acceptance screenshots/logs are not substituted for the requested single gameplay MP4.

## 10. Development-only setup

No Development progression injection was used in a final recording because no final recording was produced. Existing acceptance mechanisms were inspected as a possible way to prepare 329/330 outside visible footage, but no production gameplay was modified and no debug presentation was passed off as client footage.

## 11. Save safety

Before recording, all existing saves were copied to an isolated backup and hashed.

After the failed attempts:

- all Unreal/cook processes were closed;
- the original save backup was restored to `Saved/SaveGames`;
- restored files: 23;
- restored bytes: 70,532;
- SHA-256 mismatches: 0;
- temporary save-backup folder remaining: no.

The user's original saves were restored byte-for-byte.

## 12. Production-change confirmation

- No production gameplay code or Blueprint was changed for recording.
- No production map or Unreal asset was changed.
- No Forest lighting, exposure, fog, sky, post-process, or renderer setting was changed.
- No Phase 4 work was performed.
- No files were staged or committed.

This report is the only intentional project file created by the recording task.

## 13. Required continuation

To complete the requested video, first make enough committed memory available for Unreal to run reliably. The safest continuation is:

1. restart Windows to clear accumulated committed memory;
2. close nonessential browser/launcher applications;
3. confirm the Windows page file has adequate free disk-backed capacity;
4. rerun the Development cook/package or immersive PIE route;
5. use Xbox Game Bar, whose H.264 + AAC stereo capture and non-silent game audio are already verified;
6. validate the final MP4 with Windows MediaEncodingProfile and decoded-waveform checks before delivery.

The recording must be rerun from the beginning. The current partial clips should not be delivered to the client.
