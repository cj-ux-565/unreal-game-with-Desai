# SOTM Performance Audit & Stabilization Report

**Project:** The Secrets of the Mansion — Chapter 1  
**Engine:** Unreal Engine 5.6.1  
**Audit date:** 2026-07-30  
**Status:** **Performance stabilization complete; gameplay-readiness cleanup partially complete because the production Mansion TV path is absent/unverifiable**

This report does not claim that the project is universally “lag free.” The production Main Menu and Mansion now exceed 60 FPS on the development PC in steady-state native-1080p High tests. The Forest remains below 60 FPS because its existing coin and AI actors make it CPU-bound. First-use shader/PSO hitches, inherited Blueprint compile failures, and interactive traversal/TV verification also remain unresolved.

No coins, objectives, HUD, AI, boss, upgrade, Timmy, save, or other gameplay system was implemented.

## 1. Git checkpoint

The clean Foundation Cleanup commit was tagged before optimization:

- Tag: `performance-audit-baseline-2026-07-29`
- Commit: `5921946d`

The pre-existing untracked requirements, PDFs, audit report, and video in `ProjectDocs/` were not modified or staged.

## 2. Test machine and profiling method

### Hardware

- CPU: Intel Core 7 240H
- GPU: NVIDIA GeForce RTX 5060 Laptop GPU
- GPU memory: 8,151 MiB reported by `nvidia-smi`
- System RAM: 16 GB
- NVIDIA driver: 596.08
- OS graphics preference: UnrealEditor set to High Performance / RTX 5060

The repaired driver was verified with `nvidia-smi`. The authoritative RTX captures report D3D12, SM6, 1920×1080, and the NVIDIA adapter.

### Tools and captures

The pass used:

- Unreal CSV Profiler counters corresponding to frame, game, render, GPU, memory, streaming, RHI, scene, actor, tick, Niagara, and animation statistics
- Unreal Insights with CPU, GPU, frame, bookmark, loading, file, and memory trace channels
- World Partition, light, component, Niagara, media, PCG, foliage, and map inventory scripts
- `CompileAllBlueprints` commandlet
- Development cook/package and packaged-map load tests

Authoritative data is under generated/ignored paths:

- `Saved/PerformanceAudit/RTX_Before/`
- `Saved/PerformanceAudit/RTX_After/`
- `Saved/PerformanceAudit/Insights/RTX_After_Forest_Insights.utrace`
- `Saved/Logs/PerformanceAudit_PostOptimization_CompileAllBlueprints.log`

The final Insights trace is 11,892,643 bytes. The trace was captured successfully, but an interactive Unreal Insights session could not be automated, so no per-pass GPU timings are inferred from it.

Each primary CSV test captured 600 frames. Reported steady-state figures exclude the first 20% of frames. Average FPS is `1000 / mean frame time`. The 1% low is derived from the mean of the slowest 1% of included frames.

## 3. Before versus after at native 1920×1080 High

| Map | Average FPS before | Average FPS after | 1% low before | 1% low after | Mean frame ms before | Mean frame ms after |
|---|---:|---:|---:|---:|---:|---:|
| Main Menu | 160.57 | 157.18 | 59.54 | 66.77 | 6.23 | 6.36 |
| Mansion | 16.46 | 64.60 | 12.65 | 5.86 | 60.76 | 15.48 |
| Forest | 16.14 | 43.51 | 7.91 | 31.35 | 61.97 | 22.98 |

The Mansion’s low 1% result is caused by three remaining first-use PSO stalls, including one 345 ms frame. Its 95th-percentile frame is 16.65 ms and median frame is 13.50 ms, so the steady workload is over 60 FPS but shader creation is not yet stutter-free.

### CPU, render, and GPU timing

| Map | Game Thread before → after | Render Thread before → after | GPU before → after | Current bound |
|---|---:|---:|---:|---|
| Main Menu | 2.28 → 2.33 ms | 6.18 → 6.30 ms | 5.34 → 5.37 ms | No persistent bound at 60 FPS |
| Mansion | 60.63 → 8.88 ms | 39.65 → 15.44 ms | 18.50 → 12.68 ms | Render thread / PSO spikes |
| Forest | 61.39 → 19.61 ms | 11.15 → 21.44 ms | 20.42 → 21.64 ms | CPU and GPU/render bound |

### Draw, primitive, and memory counters

| Map | Draw calls before → after | Primitives before → after | Process RAM before → after | Local GPU memory before → after |
|---|---:|---:|---:|---:|
| Main Menu | 75 → 76 | 113,947 → 114,912 | 1,217 → 1,401 MB | 898 → 942 MB |
| Mansion | 441 → 639 | 48,210 → 939,597 | 1,671 → 1,700 MB | 1,991 → 2,029 MB |
| Forest | 5,273 → 5,311 | 4,695,183 → 4,702,484 | 2,250 → 2,474 MB | 1,620 → 1,829 MB |

The Mansion visibility set differs between the old and newly cooked captures, so its draw/primitive delta must not be interpreted as an optimization regression. Actor count changed from 1,974 to 1,975 and the new package renders substantially more visible geometry while still improving frame time.

The available RHI counter reports primitives, not exact triangles. Exact triangle counts are not estimated.

### Actors and ticks

| Map | Runtime actors after | Runtime ticks after |
|---|---:|---:|
| Main Menu | 47 | 37 |
| Mansion | 1,975 | 52 |
| Forest | 906 | 552 |

## 4. Root causes and optimization evidence

### 4.1 CPU Niagara collision was the largest shared CPU cost

Both Mansion and Forest use `/Game/portallevel`, a CPU Niagara system with per-particle collision and no Effect Type/scalability policy.

Before the change:

- Mansion Game Thread Effects: approximately 41.68 ms
- Mansion Effects worker wait: approximately 13.19 ms
- Forest Game Thread Effects: approximately 34.16 ms
- Forest Effects worker wait: approximately 8.15 ms

A diagnostic run with CPU Niagara collision disabled improved Mansion from approximately 15.7 to 45.3 FPS and reduced Game Thread time from approximately 62 ms to 6.45 ms. Visible particles remained active.

Applied change:

`fx.Niagara.Collision.CPUEnabled=0`

This disables the visual-only portal particles’ CPU collision queries without disabling the Niagara systems or gameplay collision.

### 4.2 Mansion movable fill-light shadows were the second bottleneck

Mansion contained 17 low-intensity movable fill lights with:

- Intensity: 50
- Original attenuation radius: 10,000
- Shadow casting enabled on 15
- Approximately 1,876 shadow-caster components in range

Diagnostics measured roughly 31 ms in dynamic shadow initialization. Enlarging the shadow cache did not help. The Medium shadow tier was also slower than High because its smaller Virtual Shadow Map page budget thrashed, producing approximately 24.5 ms of shadow initialization and only 31.97 FPS.

Applied changes:

- Reduced only these 17 fill-light radii from 10,000 to 2,500
- Disabled shadow casting only on the 15 fill lights that cast shadows
- Retained their illumination, intensity, color, position, mobility, and visibility
- Retained key/environment light shadows

Measured result:

- Shadow initialization: approximately 31 ms diagnostic baseline → 0.44 ms
- Shadow wait: approximately 13–24 ms → 0.17 ms
- Mansion: 16.46 → 64.60 average FPS at native 1080p High

Visual before/after screenshots could not be captured through the offscreen automation harness. Therefore the quantitative gain is verified, but an interactive visual sign-off is still required.

### 4.3 Forest is now CPU-bound by existing gameplay actors

Forest inventory:

- 906 runtime actors
- Approximately 552 runtime ticks
- 330 coin actors
- 330 rotating movement components
- 33 character movement / Pawn Sensing stacks
- 384 StaticMeshActors
- 732 StaticMeshComponents
- 536 foliage instanced-mesh components
- 372 spline-mesh components
- 35 instanced-foliage actors
- 8 instances of `SM_Iceland_Mountain_02`

After the safe rendering/VFX changes:

- Game Thread TickActors: approximately 9.44 ms
- CharacterMovement: approximately 2.62 ms
- End-physics wait: approximately 2.24 ms
- Render visibility wait: approximately 6.12 ms
- GPU: approximately 21.64 ms

The native Low preset measured 51.86 FPS, and Medium at 70% internal resolution measured approximately 50 FPS on its warmed repeat. Reducing graphics cannot overcome the approximately 19 ms Game Thread. Fixing this requires coin rotation and AI activation/tick work, both explicitly excluded from this milestone.

### 4.4 Forest rendering remains expensive

Forest High produces approximately:

- 5,311 draw calls
- 4.70 million primitives
- 21.64 ms GPU time

The map uses World Partition and HLOD layers but no Forest `WorldPartitionHLOD` actor packages were found. Building and visually testing Forest HLODs remains a strong future optimization.

`SM_Iceland_Mountain_02` is approximately 116 MB, has Nanite enabled, has one authored LOD, and is instanced eight times. Its bounds are approximately 102,400 × 102,400 × 37,682 units. Nanite fallback behavior still requires an interactive visualization pass.

### 4.5 First-use PSO compilation causes major spikes

Fresh and warmed packaged runs still encounter many new graphics PSOs. The runtime log reports:

- `r.D3D12.PSO.DiskCache=0`
- `r.D3D12.PSO.DriverOptimizedDiskCache=0`
- repeated 100–200 ms PSO waits

The warmed Mansion run averaged 64.60 FPS but still had three frames over 50 ms and a 345 ms maximum. The cold Medium Forest run had five 200–470 ms render-thread stalls.

A production PSO cache was not generated because the available harness cannot traverse the complete route and exercise every material. Enabling a cache without representative collection would not prove first-run coverage.

### 4.6 Streaming is not the steady-state bottleneck

After optimization:

- Main Menu desired texture data: 98.7% average, 100% median
- Mansion: 99.0% average, 100% median
- Forest: 99.5% average, 100% median
- Maximum steady RenderAssetStreaming time: 0.43 ms Mansion, 0.17 ms Forest

Initial traversal streaming could not be measured because automated walking/camera input was unavailable. The cold PSO/streaming spikes mean traversal stutter is not ruled out.

### 4.7 PCG is not currently responsible

No PCG components were found in the production Main Menu, Mansion, or Forest maps. No PCG graph was modified.

## 5. Safe stabilization changes

### Renderer/configuration

1. Removed the global `r.Streaming.PoolSize=5000` override so Unreal quality tiers can control the texture pool.
2. Corrected `r.VT.TileBorderSize=512` to `4`, the UE 5.6 default.
3. Removed the unused Windows Vulkan SM6 target from Windows packaging; D3D11 SM5 and D3D12 SM6 remain.
4. Disabled CPU collision queries for visual-only Niagara portal particles.
5. Added explicit texture, view-distance, foliage, and grass scalability tiers.

### Scalability policy

| Tier | UE internal resolution default | Texture pool | View distance | Foliage/grass density |
|---|---:|---:|---:|---:|
| Low | 50% | 512 MB | 0.4 | 0.0 |
| Medium | 71% | 768 MB | 0.6 | 0.4 |
| High | 87% | 1,200 MB | 0.8 | 0.8 |
| Epic | 100% | 2,000 MB | 1.0 | 1.0 |
| Cinematic | 100% | 3,000 MB | Engine cinematic | Engine cinematic |

The headline High measurements deliberately override internal resolution to 100% to provide a native-1080p comparison. Epic preserves authored density. Lower tiers use Unreal temporal upscaling while maintaining a 1920×1080 output.

## 6. Test results

| Test | Result |
|---|---|
| Main Menu idle, native 1080p High | Passed performance target: 157.18 FPS |
| Mansion idle, native 1080p High | Passed average target: 64.60 FPS |
| Mansion persistent spike criterion | Failed: first-use PSO stalls remain |
| Forest idle, native 1080p High | Failed target: 43.51 FPS |
| Forest native Low | Failed target: 51.86 FPS |
| Forest Medium/70% internal resolution | Failed target: approximately 50 FPS warmed |
| RTX D3D12 SM6 Unreal Insights capture | Passed |
| Three-map Development cook/package | Passed |
| Main Menu, Mansion, Forest packaged map loads | Passed |
| Direct Main Menu → Mansion and Mansion → Forest travel | Passed in the existing packaged travel harness |
| Interactive New Game click | Unable to verify headlessly |
| Interactive walking/looking/traversal | Unable to verify headlessly |
| Mansion TV video playback and cost | Unable to verify |

The prior travel harness recorded:

- Main Menu → Mansion LoadMap: 0.939 s
- Mansion → Forest LoadMap: 1.860 s

Performance changes did not modify map, menu, portal, player, or transition logic. The final package contains and directly loads all three approved production maps.

## 7. Media/TV status

The Mansion map inventory contains no persistent media component, and no Mansion media-play event was present in the automated runtime logs. The intended TV trigger could not be activated with the headless harness. TV playback is therefore **unable to verify**, not passed.

Forest contains an inherited broken media reference:

`file://C:/Users/Marcus Gonzalez/Downloads/Isabella Jumpscare.mp4`

That developer-local reference fails in the packaged log. It was not changed because cutscenes/jump scares are outside this milestone.

## 8. Blueprint compile verification

The post-optimization `CompileAllBlueprints` commandlet processed the project and returned:

- Exit code: 76
- Compiler errors: 165
- Warnings: 94
- Failed Blueprint assets: 7

Failed assets:

1. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/TitleScreen/WBP_TitleScreenMenu`
2. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_VideoSettings`
3. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_GameSettings`
4. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_ControlsSettings`
5. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_AudioSettings`
6. `/Game/Blueprints/BP_WEAPON`
7. `/Game/LazyDevAac990ce745b2V1/data/Content/LazyDevAbandonedTunnel/DemoContent/Blueprints/BP_DemoCharacter`

The five production widgets contain inherited UE 5.6 `ShowDecisionDialogFromMenu` delegate-signature errors. `BP_WEAPON` has stale `Equip Weapon` nodes. `BP_DemoCharacter` is marketplace demo content with missing Enhanced Input actions.

No Blueprint was modified during this performance pass. These failures predate the optimization, but they mean the “no Blueprint compile errors” success criterion is not met.

## 9. Packaging result

The final Development cook/package:

- Used exactly these production root maps: `/Game/Main_Menu_Map`, `/Game/Mansion_GameStart`, and `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`
- Cooked/staged 2,296 packages and 4,682 IoStore entries
- Used D3D12 SM6 and retained D3D11 SM5 support
- Completed successfully with AutomationTool exit code 0
- Clean BuildCookRun duration: 988.58 seconds
- Output: generated/ignored `Build/FoundationCleanupPackage/Windows/SOTM1.exe`

The package includes the final config and Mansion light changes. Packaged output must not be committed.

## 10. Metrics that could not be captured

The following are not estimated:

- Exact triangles; RHI primitives are reported instead
- Runtime total component count; editor-loaded inventory was used
- Standalone `stat blueprint` and `stat collision` aggregate values were not exposed in the packaged CSV columns
- Interactive walking, camera rotation, and long traversal streaming time
- TV playback performance
- Interactive New Game button timing
- Visual before/after screenshots for the fill-light shadow change
- Interactive per-pass Unreal Insights analysis

Animation, streaming, RHI, actor, tick, memory, GPU, Game Thread, Render Thread, and Niagara counters were captured. No GC marker appeared in the steady CSV windows; this does not prove that a long traversal has no GC spike.

## 11. Files intentionally modified

### Configuration/report

1. `Config/DefaultEngine.ini`
2. `Config/DefaultScalability.ini`
3. `ProjectDocs/SOTM_Performance_Audit_Report.md`

### Mansion fill-light external actor packages

4. `Content/__ExternalActors__/Mansion_GameStart/0/BV/NKZEE1D45FH3MMVOINU2B7.uasset`
5. `Content/__ExternalActors__/Mansion_GameStart/1/MK/O4T64A23CPOPCC22F5G8A4.uasset`
6. `Content/__ExternalActors__/Mansion_GameStart/4/Q6/6VKNFAYZU5MU6CUPSEWVL5.uasset`
7. `Content/__ExternalActors__/Mansion_GameStart/6/P5/G4VS6ZO75HQDPQXTO25ZE3.uasset`
8. `Content/__ExternalActors__/Mansion_GameStart/7/P1/XXZ855LU9QWGEXST5N5V8S.uasset`
9. `Content/__ExternalActors__/Mansion_GameStart/7/XO/9OAEO1Q8ZCN2EMZNKKNQXF.uasset`
10. `Content/__ExternalActors__/Mansion_GameStart/8/0N/DMGDBA8X7KKBOBVZ4KRT2J.uasset`
11. `Content/__ExternalActors__/Mansion_GameStart/8/25/9R031XKQXOPDZZXKKDZWXK.uasset`
12. `Content/__ExternalActors__/Mansion_GameStart/9/01/XB6HPB0OA1J8T72L34T62N.uasset`
13. `Content/__ExternalActors__/Mansion_GameStart/9/0R/VMRCH1OZM6ZB4J7DNQ4NBR.uasset`
14. `Content/__ExternalActors__/Mansion_GameStart/9/2R/IR8YD3PYMSMAIOWKHCINX6.uasset`
15. `Content/__ExternalActors__/Mansion_GameStart/9/JK/Y70N7UFGV62VJQPBFLJNPU.uasset`
16. `Content/__ExternalActors__/Mansion_GameStart/9/W3/SPIKW7NLL1K6Q19CRB3HAM.uasset`
17. `Content/__ExternalActors__/Mansion_GameStart/A/ZJ/56FIQ1N0ZNTSS1GDCN18TE.uasset`
18. `Content/__ExternalActors__/Mansion_GameStart/B/1Q/PLYVRKJKAYPKM6TIIBE4PM.uasset`
19. `Content/__ExternalActors__/Mansion_GameStart/C/AD/XIOA5L76K79GNH5Y5CV45B.uasset`
20. `Content/__ExternalActors__/Mansion_GameStart/D/9L/PWG0CHZ545NKJM2CUWVBKP.uasset`

All 17 packages contain the radius correction. Fifteen also contain the shadow-casting change; the two lights that already had shadows disabled retained that state.

## 12. Git commit list

Commit exactly the 20 files listed in section 11.

Do not commit:

- `Saved/`
- `Intermediate/`
- `DerivedDataCache/`
- `Build/` packaged output
- Any pre-existing untracked requirement PDF, text, video, or earlier audit report unless separately approved

Nothing has been staged or committed by this performance pass.

## 13. Remaining performance risks

1. Forest’s 330 rotating coin components and 33 active character/AI stacks prevent 60 FPS.
2. Forest has no built HLOD actors.
3. Forest remains approximately 5,300 draw calls and 4.7 million primitives.
4. First-use PSO creation produces 100–345+ ms stalls.
5. Mansion lighting passed the available captured-view check; a foreground
   full-screen art-direction sign-off is still recommended.
6. Short movement passed in Mansion and Forest; long World Partition
   traversal remains unprofiled.
7. Mansion TV playback is absent/unconnected in the production Mansion.
8. The five production menu/settings widgets pass targeted compilation;
   `BP_WEAPON` and the isolated demo Blueprint remain intentionally unresolved.
9. The Forest media source uses a broken developer-local path.
10. `SM_Iceland_Mountain_02` needs Nanite/fallback visualization review.
11. The Gameplay Ability System searches all `/Game/` cue paths at startup because no narrower paths are configured.

## 14. Recommended next work

The next task should be a narrowly scoped **production correctness and performance-blocker repair**:

1. Obtain client clarification on where and when the missing Mansion TV should
   exist before authorizing any content/logic connection work.
2. Add a reproducible interactive performance test route or developer input harness.
3. Collect and ship a representative PSO cache for Main Menu → Mansion → Forest.
4. During the authorized coin milestone, replace 330 component ticks with a material/instanced or distance-gated rotation solution.
5. During the authorized AI milestone, activate and tick enemies only near the player.
6. Build Forest HLODs in a separate checkpoint and visually inspect cell transitions.

## 15. Milestone conclusion

Verified achievements:

- Main Menu remains well above 60 FPS.
- Mansion improved from 16.46 to 64.60 average FPS at native 1080p High.
- Forest improved from 16.14 to 43.51 FPS at native 1080p High.
- Niagara CPU collision and Mansion shadow bottlenecks were identified and safely reduced.
- RTX D3D12 SM6 profiling and an Unreal Insights trace were captured.
- Final three-map Development cook/package succeeded.
- Production maps still load and direct travel references remain valid.
- The five affected production menu widgets pass targeted compilation.
- Runtime New Game reaches the Mansion with the correct production character.
- Short player-movement tests pass in both Mansion and Forest.
- Entering the Mansion portal opens the approved Forest.
- Mansion lighting remains visually acceptable in the captured view.

The milestone is **not complete under the original success criteria** because:

- Forest does not reach 60 FPS at any tested graphics tier.
- PSO frame spikes remain.
- `BP_WEAPON` and an isolated demo Blueprint retain pre-existing compile
  errors, but neither has production Asset Registry referencers.
- Mansion and Forest movement were verified, but the production Mansion has
  no connected TV/video playback path.

No new gameplay development should begin until the user approves whether the next pass may touch the production menu widget errors and, later, the coin/AI tick ownership responsible for the Forest CPU limit.

---

## 16. Gameplay-readiness finalization addendum

**Date:** 2026-07-30

This addendum supersedes the earlier statements in sections 6-15 about the
five production menu widgets, interactive route verification, Mansion
lighting, and TV verification. No gameplay system was implemented.

### 16.1 Production Blueprint repairs

The five production Menu System Pro widget packages were loaded in a clean
editor session and compiled individually after their parent/dependency assets
were available. A second clean targeted compile pass returned `compiled=true`
with no compiler error for all five:

1. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/TitleScreen/WBP_TitleScreenMenu`
2. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_VideoSettings`
3. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_GameSettings`
4. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_ControlsSettings`
5. `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Settings/WBP_AudioSettings`

There are no remaining known compiler failures in these five affected
production menu widgets. Git confirms that the successful targeted compiles
did not require a persistent binary asset change. The earlier
`CompileAllBlueprints` failures were caused by broad compile ordering/transient
inherited UMG generated-field state rather than a persistent error in the
production widget packages. An unsafe transient compile state was discarded,
and the five files currently match `HEAD`.

`/Game/Blueprints/BP_WEAPON` was not changed. Asset Registry reports no
referencers, and its stale `Equip Weapon` call targets a function that does
not exist on `BP_Speedster_C`. Inventing a replacement would redesign or
extend gameplay behavior, which is outside this task.

`/Game/LazyDevAac990ce745b2V1/data/Content/LazyDevAbandonedTunnel/DemoContent/Blueprints/BP_DemoCharacter`
was also left unchanged. Asset Registry reports no referencers, and it is
isolated marketplace demo content rather than part of the production route.

### 16.2 Runtime production-flow verification

The runtime route was exercised through the production widgets' bound handlers
because the automation-launched editor did not expose foreground Slate focus.
This used the same widget events invoked by normal UI input and did not bypass
the map-travel logic:

1. Main Menu intro -> Title Screen
2. Play -> Single Player
3. New Game -> Save Game Menu
4. Save slot -> confirmation dialog
5. Confirm -> `/Game/Mansion_GameStart`
6. Enter `/Game/BP_ForestPortal`
7. Travel -> `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

Results:

- New Game opened `Mansion_GameStart`.
- The Mansion spawned
  `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter.BP_MenuSystemCharacter_C`.
- Mansion movement passed: the pawn moved approximately 466 Unreal units
  during a three-second forward-input test.
- Entering the production `BP_ForestPortal` opened the approved `CH1` Forest.
- Forest movement passed: the same production pawn moved approximately 1,724
  Unreal units during a three-second forward-input test.

### 16.3 Mansion TV result

The TV check found a production-content gap rather than only an automation
limitation:

- The PIE Mansion world contained 1,976 actors but no actor whose name, label,
  or class identified a TV, video, media, television, or screen actor.
- No loaded Mansion static-mesh component used a `/Game/Video` asset, media
  texture, or media-player material.
- The only live non-default `MediaPlayer` object was
  `/Game/d_oll/DEATH_ANIME/NewMediaPlayer`, unrelated to the Mansion.
- `/Game/Video/VideoWidget/WBP_Scene1` is referenced by `/Game/Video/Scene1`
  and `/Game/Video/VideoWidget/Scene1`; neither is referenced by
  `Mansion_GameStart`.

Mansion TV playback is therefore **not passed / unable to verify in the
production flow**. Connecting or creating it would be gameplay/content work
and was intentionally not attempted.

### 16.4 Mansion lighting result

The captured Mansion view retains warm illumination, readable wall and
furniture detail, and key-light shadows. No blacked-out region or major visual
downgrade was observed after shadows were disabled on the 15 fill lights. A
concentrated bright patch remains near the character, but no additional
lighting change was made.

Generated evidence, excluded from Git:

- `Saved/Screenshots/WindowsEditor/GameplayReadiness_Mansion_Lighting.png`
- `Saved/Screenshots/GameplayReadiness_Forest.png`

### 16.5 Current FPS and stutter limitation

A trustworthy new interactive FPS sample could not be collected. The
automation-launched editor had no foreground window handle and throttled PIE;
its reported 3-6.5 FPS/frame delta is a background-editor artifact and is
deliberately excluded rather than presented as project performance.

The authoritative completed-audit captures remain:

| Scenario | Authoritative completed-audit result | Finalization observation |
|---|---:|---|
| Main Menu idle | 157.18 FPS | Route and widgets function; no valid new foreground FPS sample |
| Mansion movement | 64.60 FPS steady-state benchmark | Movement works; first-use PSO waits still appear in the live log |
| Mansion TV | No metric | Production TV/video path is absent/unconnected |
| Forest movement | 43.51 FPS benchmark | Movement works; no valid new foreground FPS sample |

The live finalization log recorded at least 150 first-use PSO creation hitches
after Mansion loading, with no PSOs reported as precached. Normal play cannot
yet be described as completely stutter-free. No FPS or smoothness value was
estimated to replace the unavailable foreground capture.

### 16.6 Remaining known risks and deferred systems

1. Forest remains below 60 FPS in the completed benchmark.
2. The 330 rotating coin components remain untouched and are deferred to the
   authorized coin milestone.
3. The 33 AI/character stacks remain untouched and are deferred to the
   authorized AI milestone.
4. First-use PSO compilation can still cause visible stalls.
5. Long-duration World Partition traversal remains unprofiled.
6. Mansion TV playback is absent/unconnected in the production Mansion.
7. `BP_WEAPON` and the isolated demo Blueprint retain pre-existing errors but
   have no production Asset Registry referencers.
8. A final full-screen lighting sign-off on a foreground editor/game window is
   recommended even though the captured view is acceptable.

### 16.7 Exact intentionally modified files

The only persistent file intentionally changed by this finalization task is:

1. `ProjectDocs/SOTM_Performance_Audit_Report.md`

The five widgets were compiled and verified but currently match `HEAD`, so
they must not be included in the commit merely because they were inspected.

The earlier Performance Audit changes are still uncommitted. The complete
recommended combined technical-cleanup commit remains exactly the 20 files
listed in section 11: `DefaultEngine.ini`, `DefaultScalability.ini`, this
report, and the 17 Mansion fill-light external actor packages.

Nothing was staged or committed.

### 16.8 Finalization conclusion

Passed:

- All five affected production menu widgets compile successfully.
- The runtime New Game path reaches the Mansion.
- The correct production character spawns.
- Player movement works in Mansion and Forest.
- The Mansion portal reaches the approved Forest map.
- Mansion lighting remains visually acceptable in the captured view.
- No new gameplay system was implemented.

Not passed:

- The production Mansion contains no connected TV/video playback path.
- A new foreground interactive FPS/stutter capture was unavailable.
- Forest performance and first-use PSO hitches remain known limitations.

Before beginning a client gameplay milestone, decide whether the missing
Mansion TV is an immediate production-flow blocker or belongs to a later,
explicitly authorized content milestone.
