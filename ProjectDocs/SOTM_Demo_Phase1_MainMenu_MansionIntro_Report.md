# SOTM Client Demo Phase 1 Report

Date: 2026-08-15  
Project: The Secrets of the Mansion - Chapter 1  
Engine: Unreal Engine 5.6

## Outcome

Demo Phase 1 is implemented without adding any later-phase gameplay systems. The production New Game route now presents exactly four story save slots, opens the Mansion as a locked cinematic introduction, presents Timmy and Isabella, performs a zero-damage scripted knockout/dragging transition, and then opens the production CH1 Forest with normal third-person control and HUD restored.

The Editor target builds successfully and the affected save-slot widget compiles successfully. A final clean four-slot PIE rerun produced no Blueprint runtime error or `Accessed None` after the slot screen initialized.

Two pause-input acceptance checks remain manual: the Escape and Tab mappings are present on the same production pause action, but Editor/Slate focus intercepted automated keystrokes during PIE, so a reliable visual proof of both keys and repeated no-stacking behavior was not captured. No packaged build was made because packaging was explicitly prohibited.

## 1. Main Menu cleanup

- Kept the approved Design Silence production menu instead of redesigning it.
- Preserved the existing PLAY, Continue/load, options, and story-save architecture.
- The clean title/menu and save-slot viewport no longer display production-facing red runtime messages.
- Unreal logging remains enabled; `DisableAllScreenMessages` was not used.

## 2. Root cause of visible error text

The production `WBP_SaveSlot` child did not initialize two text-widget references expected by `WBP_SaveSlotBase`: `AreaNameTextRef` and `AreaTextRef`. Loading save data therefore caused repeated `Accessed None` Blueprint runtime errors, which Unreal exposed as visible red PIE messages.

The child PreConstruct graph now assigns both inherited references to the child widget's existing hidden `EmptyText` element before calling the parent PreConstruct. This preserves the approved slot layout while giving the base class valid non-rendered targets for its optional area data.

Verification: after the final four-slot normalization log entry, the remaining 25 log lines contain no `Error:`, Blueprint runtime error, `Accessed None`, fatal error, or failed ensure.

## 3. Four-save-slot implementation

- `WBP_SaveGameMenu` maximum displayed story slots changed from 20 to exactly 4.
- The Phase 1 world subsystem normalizes the user-facing labels to:
  - SAVE SLOT 1
  - SAVE SLOT 2
  - SAVE SLOT 3
  - SAVE SLOT 4
- The existing MenuSystemPro save backend remains the authority for empty slots, New Game, existing saves, Continue/load, overwrite, and delete behavior.
- No parallel SaveGame implementation was introduced.

## 4. Pause Escape/Tab implementation

- Added Tab to the same `IA_InGameMenu` mapping already used by Escape/P and the gamepad menu button in `IMC_AlwaysAllowed`.
- No second pause widget or alternate pause system was created.
- Static mapping verification passed: P, Escape, Gamepad Special Right, and Tab all target the same production in-game-menu input action.
- Automated visual acceptance in PIE is unresolved because Unreal Editor/Slate consumed Escape as Stop PIE and intermittently consumed Tab as an editor-focus command. This is a test-environment focus limitation, not evidence that the runtime mapping is broken.
- Manual acceptance still required: focus the actual game viewport, press Escape, Resume, press Tab, Resume, then alternate both keys several times and confirm only one pause menu exists.

## 5. Mansion cinematic architecture

- Added `USOTMDemoPhase1WorldSubsystem`, active only for Game/PIE worlds.
- Added `LS_SOTM_MansionIntro` and a dedicated `SequenceCamera` actor in `Mansion_GameStart`.
- New Game sets a transient pending-intro flag in the existing Player State subsystem. Mansion consumes it exactly once.
- Continue and direct development map loads do not replay the intro.
- Sequencer/timers drive the phases; no Event Tick cinematic state machine was added.
- The sequence uses controlled camera framing while the world subsystem coordinates the story actors, subtitles, fades, input locks, and final map travel.

## 6. Timmy asset used

- Production skeletal mesh: `/Game/HorrorBear/Mesh/SKM_HorrorBear`
- Existing HorrorBear idle animation is retained.
- Runtime lookup is asset-based so the cinematic uses the actual HorrorBear skeletal mesh rather than relying on a fragile actor label.
- Timmy is established before Isabella's close presentation. Temporary subtitle timing is isolated so final recorded voice can replace it later.

## 7. Isabella asset and animation used

- Production Mansion actor: `Isabel_Phase1` / existing `BP_AI`-derived Isabel.
- Existing montage: `/Game/AI/AM_Isabel_JumpScare_Phase3`
- Existing scream/audio reference is reused where available.
- Her normal AI gameplay is suspended for this scripted opening; it is not allowed to run Forest combat behavior during the cinematic.
- Existing controller/legacy timers are cleared safely for the cinematic actor without creating a second AI or combat system.

## 8. Knockout and jump-scare implementation

- Isabella is framed close to the controlled cinematic camera and plays the existing suitable montage once.
- The impact is presented with a restrained fade/blackout and subtitle/audio cue.
- This is a dedicated zero-damage story knockout. It does not reduce health, consume a life, invoke respawn, or bypass the Player System.
- Runtime evidence includes: `SOTM Demo Phase 1: scripted zero-damage knockout presented.`

## 9. Dragging transition

- After blackout, the camera moves to a low/body-level presentation intended to read as the unconscious player being moved.
- Temporary dragging narration/subtitle bridges the abstracted movement.
- A final controlled fade hides the map transition.
- No interactive walking or final full-body dragging animation was fabricated.

## 10. Forest transition

- The sequence ends by opening `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`.
- Runtime log confirms Mansion intro completion followed by travel to the production CH1 map.
- The captured first playable frame shows the production third-person character, gameplay HUD, 5/5 lives, and Chapter Status coin display.
- No Forest lighting, Forest post process, coin placement, coin GUID, objective, AI, upgrade, chest, gate, boss, or ending asset was changed.

## 11. Input lock and unlock behavior

- The cinematic uses the existing Player State subsystem input-lock architecture.
- Movement, look, and interaction remain locked during the Mansion story presentation.
- The player is not required to walk through the intro.
- The lock is released before/while transitioning to CH1; the Forest frame confirms normal gameplay presentation is restored.
- World cleanup removes the temporary viewport subtitle overlay and timers if PIE stops or the world changes.

## 12. Continue regression

- Continue does not set the pending Mansion-intro flag and therefore resumes the saved route instead of replaying the opening.
- A Continue run loaded the existing saved CH1 state during acceptance testing.
- The original local save folder was backed up before slot testing and restored afterward.

## 13. Save-slot regression

- The backend was preserved; only the visible slot count, labels, and missing widget references were changed.
- Original local save data was restored after the acceptance run.
- Hash comparison between the backup and restored save set reported `HashMismatches=0` across the 23 restored files.
- Future checkpoints can continue writing through the selected MenuSystemPro slot. Full checkpoint UX redesign remains out of scope.

## 14. Coin-persistence regression

- Coin subsystem code, coin saves, GUIDs, actors, maps, and placements were not modified.
- Continue successfully reached the saved CH1 route.
- The pre-test save files were restored byte-for-byte, preventing the temporary New Game acceptance slot from replacing the user's retained coin state.
- A new packaged-process relaunch persistence test was not run because packaging was explicitly prohibited in this phase.

## 15. Errors found and fixed

Fixed:

- `WBP_SaveSlotBase` `AreaNameTextRef` Accessed None through the production child.
- `WBP_SaveSlotBase` `AreaTextRef` Accessed None through the production child.
- Excess story-slot creation (20 entries) reduced to the approved four.
- Fragile Timmy actor-label dependency replaced with production HorrorBear mesh identification.
- Isabel cinematic setup avoids the earlier invalid blackboard/timer noise caused by leaving inherited gameplay timers active during the scripted presentation.

Known unrelated/environment messages not expanded in this phase:

- UDP Messaging cannot bind one configured network interface.
- Legacy iOS runtime setting import warning.
- An unused media source references a previous developer's local `Isabella Jumpscare.mp4` path. The Phase 1 cinematic does not depend on that media file.
- Async asset compilation reported low available-memory estimates while the Editor was processing marketplace assets.

## 16. Acceptance results

| Requirement | Result | Evidence / note |
|---|---|---|
| Fresh Editor and production Main Menu | Pass | Main Menu loaded from a fresh rebuilt Editor session. |
| No production-facing red error text | Pass | Clean title and final four-slot captures; no errors after final slot initialization. |
| Exactly four slots | Pass | Final clean four-slot capture and runtime normalization log. |
| New Game uses a selected slot | Pass | User-facing route used; not direct Mansion loading. |
| Mansion intro begins | Pass | Runtime intro-start log and cinematic captures. |
| Movement/look unavailable during intro | Pass | Existing input lock held for the sequence duration. |
| Timmy presented | Pass | Production HorrorBear asset and Timmy framing capture. |
| Isabella appears and animates | Pass | Existing Isabel actor/montage and captures. |
| Scripted knockout | Pass | Zero-damage knockout runtime log and visual sequence. |
| Dragging presentation | Pass | Low/body-level controlled shot captured. |
| CH1 Forest loads | Pass | Runtime map travel and first playable frame. |
| Third person, camera, HUD restored | Pass | First playable Forest capture. |
| Movement restored | Pass | Player returned to normal CH1 gameplay state after lock release. |
| Escape opens/resumes same Pause Menu | Manual check required | Mapping verified; automated PIE keystroke was intercepted by Editor Stop PIE behavior. |
| Tab opens/resumes same Pause Menu | Manual check required | Mapping verified; automated PIE keystroke was intercepted by Slate/editor focus. |
| Repeated Esc/Tab does not stack | Manual check required | Existing single action/menu architecture retained; requires a focused Standalone/PIE manual stress test. |
| Continue route | Pass | Existing save resumed CH1; intro did not replay. |
| Coin persistence | Pass with scope limitation | No coin/save implementation changed; original 23 save files restored byte-identically. No new package/relaunch test. |
| No Forest lighting changes | Pass | No CH1/Forest lighting or post-process file in the change list. |
| No new C++ compile errors | Pass | Development Editor target built successfully after final C++ change. |
| No new production Blueprint errors | Pass for affected route rerun | `WBP_SaveSlot` compiled/saved; final clean slot rerun has no runtime error after initialization. |

Because the two physical pause-key tests and repeated-key stress test could not be captured reliably under Editor automation, final client acceptance should remain conditional until those short manual checks are performed.

## 17. Screenshots and visual evidence

Generated evidence is under `Saved/Screenshots` and is intentionally not part of the Git allowlist:

- Clean Main Menu: `DemoPhase1_Acceptance_Title2.png`
- Four slots, final clean run: `DemoPhase1_Accepted_FourSlotsClean.png`
- Timmy/HorrorBear framing: `DemoPhase1_Accepted_TimmyBear.png`
- Timmy introduction subtitle: `DemoPhase1_Accepted_Timmy.png`
- Isabella appearance: `DemoPhase1_Accepted_Isabella.png`
- Knockout/close presentation: `DemoPhase1_Acceptance_MansionSequence.png`
- Dragging shot: `DemoPhase1_Accepted_Dragging.png`
- First playable Forest frame: `DemoPhase1_Acceptance_ForestEntry.png`

Files named `Pause_Escape`, `PauseTab`, or `Pause_Tab` are diagnostic attempts and are not valid acceptance evidence because Editor focus/Stop PIE behavior prevented a conclusive pause-menu frame.

## 18. Exact files intentionally modified or created

Modified:

- `Content/Mansion_GameStart.umap`
- `Content/MenuSystemPro/Blueprints/Input/AlwaysAllowed/IMC_AlwaysAllowed.uasset`
- `Content/MenuSystemPro/ExampleContent/Designs/Design_Silence/Config/DA_GlobalMenuConfig.uasset`
- `Content/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/SaveGame/WBP_SaveGameMenu.uasset`
- `Content/MenuSystemPro/ExampleContent/Designs/Design_Silence/Widgets/SaveGame/WBP_SaveSlot.uasset`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/SOTM1.Build.cs`

Created:

- `Content/Cinematics/LS_SOTM_MansionIntro.uasset`
- `Content/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/MetaData/DA_Mansion_GameStart.uasset`
- `Content/__ExternalActors__/Mansion_GameStart/3/56/FTRVJ6YHA5PF15Z6F6C6F7.uasset`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase1WorldSubsystem.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase1WorldSubsystem.h`
- `ProjectDocs/SOTM_Demo_Phase1_MainMenu_MansionIntro_Report.md`

## 19. Exact Git allowlist

When the manual pause checks have passed, stage only the 14 paths listed in Section 18. Do not stage `Saved`, `Intermediate`, `DerivedDataCache`, Binaries, screenshots, acceptance save backups, logs, or packaged output.

Nothing was staged or committed automatically.

## 20. Suggested commit

Title:

`feat(demo): add four-slot menu and Mansion intro flow`

Description:

`Limit the story UI to four save slots, repair save-slot widget references, map Tab to the existing pause action, route New Game through a locked Timmy/Isabella Mansion introduction, and restore normal third-person gameplay on CH1 travel without changing Forest lighting or later gameplay systems.`

## 21. Suggested client update

Client Demo Phase 1 is implemented. New Game now uses one of four story save slots, plays a locked Mansion introduction with Timmy and Isabella, presents a zero-damage knockout/dragging transition, and starts normal third-person gameplay in the production Forest with the HUD restored. The visible save-screen runtime error was fixed at its widget-reference source, and Continue plus existing save/coin data were preserved. Before final sign-off, Escape and Tab should each be pressed manually in a focused game viewport to capture the same Pause Menu and confirm repeated toggling does not stack widgets.

## 22. Manual completion steps

1. Restart Unreal Editor so the final Editor DLL and assets load cleanly.
2. Open `Main_Menu_Map` and start PIE, clicking once inside the game viewport so it owns keyboard focus.
3. Use Continue or complete New Game through the four-slot screen and intro until CH1 is playable.
4. Press Escape, capture the Pause Menu, select Resume.
5. Click the game viewport again, press Tab, capture the same Pause Menu, select Resume.
6. Repeat Escape/Resume and Tab/Resume three times, then press Escape followed by Tab while the menu is open. Confirm only one pause widget exists and gameplay input/cursor state restores after Resume.
7. If Escape still stops PIE rather than opening Pause, use Play > Standalone Game (not packaging) and repeat the test there; this removes the Editor's Escape-to-stop shortcut from the input path.

