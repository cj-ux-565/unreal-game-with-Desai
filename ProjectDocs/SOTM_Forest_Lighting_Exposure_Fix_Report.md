# SOTM Forest Lighting Exposure Fix Report

**Date:** 2026-08-11  
**Engine:** Unreal Engine 5.6  
**Production Forest:** `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`  
**Scope:** Lighting/exposure tuning only

## Outcome

CH1 no longer uses the unrestricted default eye-adaptation range that was washing the Forest toward pale blue/white during gameplay. The existing unbound post-process volume now supplies a stable Forest exposure and controlled bloom. The authored fog, magical particles, lights, layout, gameplay actors, AI, coins, HUD, objectives, portals, and progression were left unchanged.

The resulting PIE view is substantially darker and more cinematic. Trees, terrain contours, the lamp route, and the distant path retain separation; bright shards and lamps remain visible without broad bloom flooding; camera turns between bright and dark directions no longer cause exposure pumping.

## 1. Root Cause

The primary cause was **uncontrolled automatic exposure**, not a single excessive light:

- `PostProcessVolume_1` is unbound with blend weight `1`, but its exposure overrides were disabled.
- The production player camera also had exposure overrides disabled.
- CH1 therefore fell back to the engine histogram exposure range (`AutoExposureMinBrightness = 0.03`, `AutoExposureMaxBrightness = 8`).
- The Forest is intentionally dark while also containing a bright sky, white additive particles, and lamp emissives. That extreme luminance range caused eye adaptation to lift the dark scene aggressively, washing out terrain/fog and clipping bright areas.

The secondary contributor was default bloom applied to the high-contrast lamps and additive particle cards. The particle material itself was not configured with an extreme scalar value, so editing the shared DarkForest material would have been broader and riskier than a CH1-local post-process correction.

## 2. Lighting Actors and Settings Inspected

### Directional light

- Actor: `Light Source` (`LightSource`)
- Intensity: `3.773698`
- Light color: muted pink/purple (`R=170, G=156, B=177` byte color)
- Volumetric scattering intensity: `1.0`
- Result: not clearly excessive; unchanged.

### Skylights

- `SkyLight_1`: intensity `20`, but light color is black; it is not a bright ambient source.
- `SkyLight_5`: intensity `1.92`, dark green light color; modest contribution.
- Result: unchanged.

### Sky atmosphere

- No `SkyAtmosphere` actor is present in CH1.

### Exponential height fog

- One actor: `ExponentialHeightFog_1`
- Fog density: `0.011927`
- Fog height falloff: `0.459486`
- Volumetric fog: disabled
- Result: fog was not amplifying volumetric light; unchanged.

### Post process / local exposure / Lumen

- One existing unbound post-process volume: `PostProcessVolume_1`
- Blend weight: `1`
- Priority: `0`
- Exposure, bloom, lens flare, local exposure, color grade, and indirect-light numeric values were inspected with their override flags.
- The initially suspicious stored exposure/bloom/lens-flare values were not active because their overrides were false.
- Lumen GI and reflections remain on the project/default path; no Lumen quality or lighting changes were made.
- Local exposure overrides remain disabled.
- Lens flare overrides remain disabled.

### Point/spot lights

- Ten placed point lights were inventoried.
- Most intensities are in the `1.71` to `21.42` range.
- One distant white point light uses intensity `1852.02` with a `1000` attenuation radius; it is localized and was not the cause of the global washout.
- No placed `SpotLight` actor exists in CH1. The production character's existing flashlight spotlight was observed and left unchanged.

### Particle / emissive FX

- Actors: `P_Fog_2` and `P_Fog2_5`
- Both use `/Game/DarkForest/Particle/P_Fog.P_Fog`.
- The particle system references `/Game/DarkForest/Materials/Instances/M_Fog_inst`.
- The material instance is additive and uses `Hardness = 0.819048`, `Extinction = 0.838542`, and `CLR = (0.515625, 0.515625, 0.515625, 1)`.
- These are bright additive cards, but the instance does not contain an obviously extreme HDR multiplier. The source asset is shared, so it was intentionally not modified.

## 3. Exact Settings Changed

Only `PostProcessVolume_1` in CH1 was changed:

| Setting | Before | After |
|---|---:|---:|
| Override Exposure Compensation | Off | On |
| Exposure Compensation | stored `1.0`, inactive | `0.0` |
| Override Min Brightness | Off | On |
| Min Brightness | engine fallback `0.03` | `0.25` |
| Override Max Brightness | Off | On |
| Max Brightness | engine fallback `8.0` | `0.25` |
| Override Bloom Intensity | Off | On |
| Bloom Intensity | engine default `0.675` | `0.30` |
| Override Bloom Threshold | Off | On |
| Bloom Threshold | engine default `-1.0` | `1.0` |

Setting Min and Max Brightness to the same value deliberately stabilizes Forest exposure and removes eye-adaptation pumping. A value of `0.25` was selected after disposable PIE comparisons at `1.0`, `2.5`, `0.20`, and `0.25`; higher fixed values made the scene too dark, while `0.20` was slightly brighter than required.

## 4. Exposure Changes

- Enabled CH1-local exposure compensation override and set it to neutral (`0`).
- Enabled Min/Max Brightness overrides and fixed both at `0.25`.
- Did not change the production camera Blueprint or project-wide defaults.
- Did not add a new volume or manager.

## 5. Bloom Changes

- Enabled the existing CH1 post-process bloom intensity override at `0.30`.
- Enabled bloom threshold at `1.0`, preventing the full image from contributing to bloom.
- Bloom remains present on lamps and magical particles, but broad glow no longer floods the frame.
- Lens flare was inspected and left unchanged because its override was inactive and no separate flare-driven artifact was identified.

## 6. Fog Changes

None.

The authored exponential fog remains enabled and unchanged. Volumetric fog was already disabled, so volumetric scattering was not the washout source.

## 7. FX / Emissive Changes

None.

`P_Fog`, `P_Fog2`, their particle system, and `M_Fog_inst` were inspected but not edited. With stable exposure and controlled bloom, the effects remain visible without requiring a risky shared-material change.

## 8. Before / After Observations

### Before

- Forest and fog were pale blue/white.
- Sky and lamp regions were broadly clipped.
- Tree and terrain silhouettes lost depth.
- Bright particle cards and lamps dominated the image.
- Histogram exposure could vary as the camera direction changed.

Baseline captures:

- `Saved/Screenshots/SOTM_CH1_Before_Editor.png`
- `Saved/Screenshots/WindowsEditor/SOTM_CH1_Before_PIE.png`

### After

- Terrain and tree layers have clear separation.
- The lamp route and path remain readable.
- Fog remains blue, dense, and atmospheric.
- White/pink particle cards remain part of the authored visual style, with much smaller bloom influence.
- The same fixed exposure is maintained in bright and dark camera directions.

Final captures:

- `Saved/Screenshots/WindowsEditor/SOTM_CH1_After_PIE_Entry.png`
- `Saved/Screenshots/WindowsEditor/SOTM_CH1_After_PIE_Traversal.png`
- `Saved/Screenshots/WindowsEditor/SOTM_CH1_After_PIE_DarkDirection.png`

## 9. PIE Validation

| Check | Result |
|---|---|
| CH1 opens in PIE | Pass |
| Production GameMode/player spawned | Pass (`BP_MenuSystemCharacter_C_0`) |
| Forest entry / first playable view | Pass |
| Bright FX and lamp route | Pass; visible with controlled bloom |
| Dark tree/terrain direction | Pass; silhouettes and terrain remain readable |
| Forward traversal | Pass; Enhanced Input handled movement and the pawn moved approximately 950 uu horizontally/up-slope |
| Camera direction change | Pass; exposure remained stable after a 90-degree control-rotation change |
| Existing fog and VFX render | Pass |
| Health/player component presence | Pass; existing `SOTM_PlayerVitals` remained attached and untouched |
| Blueprint/runtime errors | Pass; no `LogBlueprint: Error`, Blueprint Runtime Error, fatal, ensure, or script-error entries were found in the current `SOTM1.log` |
| C++ regression | No C++ files changed; no compile was required for this data-only tuning pass |
| Mansion -> Forest route | Production target remains the previously verified CH1 route and no portal/travel asset was modified. The physical Mansion overlap was not replayed in this lighting-only session. |

## 10. Exact Files / Assets Modified

1. `Content/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/B/MF/VF3FBPQ2YE5095Z2KMMSVR.uasset`
   - Existing `PostProcessVolume_1` external actor package.
2. `ProjectDocs/SOTM_Forest_Lighting_Exposure_Fix_Report.md`
   - This report.

No `.umap`, source, Blueprint, material, particle, fog, light, player, AI, HUD, PCG, foliage, coin, objective, or portal asset was modified.

The pre-existing working-tree deletion `Build/Windows/Application.ico` is unrelated and was not touched.

## 11. Git Allowlist

Only these task files should be included in a future commit:

- `Content/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/B/MF/VF3FBPQ2YE5095Z2KMMSVR.uasset`
- `ProjectDocs/SOTM_Forest_Lighting_Exposure_Fix_Report.md`

Do not include the unrelated pre-existing `Build/Windows/Application.ico` deletion.

## 12. Suggested Commit Message

`fix(forest): stabilize CH1 exposure and control bloom`

## 13. Short Client Update

The production CH1 Forest lighting has been corrected without redesigning the level. The root cause was unrestricted automatic exposure reacting to the dark forest and bright sky/particles. CH1 now uses stable level-local exposure plus reduced, thresholded bloom. The fog, magical FX, lamps, gameplay, AI, coins, HUD, and portal flow remain unchanged. PIE entry, traversal, bright-area, and dark-direction checks pass with no new Blueprint/runtime errors.
