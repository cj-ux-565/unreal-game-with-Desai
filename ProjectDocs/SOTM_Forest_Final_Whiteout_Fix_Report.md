# CH1 Forest Final White-Out Fix Report

## Scope

This pass changed only the production CH1 visual post-process configuration. No player, movement, AI, HUD, objective, portal, save, navigation, PCG, or gameplay logic was changed. No package was created and no files were staged or committed.

## 1. Reproduction

- Production level: `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`
- PIE pawn: `BP_MenuSystemCharacter`
- Reproducible player location: `(37595.432465, -12093.265300, 1260.961311)`
- Control rotation: Pitch `-10`, Yaw `120`, Roll `0`
- Active camera: approximately `(37587.1467, -12018.9140, 1310.0860)`, Pitch `-20`, Yaw `120`, FOV `90`
- Baseline result: 99.94% of sampled pixels were near-white (RGB greater than 245 on every channel), leaving terrain, trees, and the player unreadable.

Before evidence: [Screenshot_1786478995.png](../Saved/Screenshots/WindowsEditor/Screenshot_1786478995.png)

## 2. Root Cause

The white screen was caused by the unbound CH1 `PostProcessVolume_1` bloom pass amplifying the scene's aggregate extreme HDR/emissive output. The brightest contributors include the fire/magic character materials (`MI_Mage_Head_Fire`, `MI_Mage_Body_Fire`, and `MI_Mage_Legs_Fire`), player-attached `P_Rays` effects, lamps, and other bright scene surfaces. None of those sources alone caused the full-screen veil; the major brightness jump occurred when CH1 bloom processed their combined HDR output.

The exact offending presentation system is:

- Actor: `PostProcessVolume_1`
- Package: `/Game/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/B/MF/VF3FBPQ2YE5095Z2KMMSVR`
- Setting: Bloom Intensity

At the fixed camera, Bloom `0.05` with a high Threshold of `10.0` still immediately recreated the white-out. Bloom `0.0` removed the veil while direct emissive pixels, fog depth, flashlight illumination, and magical FX remained visible. This demonstrates that the problem was not ordinary scene exposure and could not be safely solved by a small bloom threshold adjustment.

## 3. Controlled Elimination Results

All comparisons used the same player/camera location.

| Contributor | Diagnostic result | Conclusion |
|---|---|---|
| `P_Fog_2` (`P_Fog`) | Disabled alone; screen remained white | Not causal |
| `P_Fog2_5` (`P_Fog2`) | Disabled alone; screen remained white | Not causal |
| Both particle fog actors | Disabled together; screen remained white | Not causal |
| Exponential Height Fog | Disabled; screen remained white | Not causal. Volumetric Fog was already disabled |
| Directional `Light Source` | Disabled; screen remained white | Not causal |
| `SkyLight_1` | Disabled alone; screen remained white | Not causal |
| `SkyLight_5` | Disabled alone; screen remained white | Not causal |
| Both skylights | Scene detail improved only modestly; excessive brightness persisted | Secondary ambient contribution only |
| Post-process volume | Disabling it changed the presentation but did not identify a single light/fog source | The volume required setting-level isolation |
| Bloom within `PostProcessVolume_1` | `0.30 -> 0.0` removed the full-screen veil | Conclusive major contributor |
| Player `P_Rays` components | Disabling all seven did not remove the full-scene white-out | Visible HDR contributor, not sufficient cause |
| Skeletal meshes/emissive characters | Strong HDR sources in a stripped scene; hiding all 26 did not remove the full-scene white-out with bloom active | Contributors, not safe shared assets to edit globally |

The camera spring arm can retract near collision and bring player emissive content closer to the camera, which aggravates the bloom response, but a runtime Camera-channel diagnostic did not remove the white-out while bloom remained active. No player/camera/collision architecture was changed in this task.

## 4. Final Values

`PostProcessVolume_1`:

| Setting | Before | Final |
|---|---:|---:|
| Exposure Min Brightness | 0.25 | 0.25 (unchanged) |
| Exposure Max Brightness | 0.25 | 0.25 (unchanged) |
| Exposure Compensation | 0.0 | 0.0 (unchanged) |
| Bloom Intensity | 0.30 | 0.0 |
| Bloom Threshold | 1.0 | 1.0 (unchanged; inactive while intensity is zero) |

No global shared material was edited. No light, fog density, skylight, particle asset, player camera, or exposure compensation change was retained.

## 5. Before / After

- Before: [Screenshot_1786478995.png](../Saved/Screenshots/WindowsEditor/Screenshot_1786478995.png) — 99.94% sampled near-white pixels.
- After, same location/rotation: [Screenshot_1786485086.png](../Saved/Screenshots/WindowsEditor/Screenshot_1786485086.png) — 0.35% sampled near-white pixels. Terrain texture, tree silhouettes, fog depth, and distant lamps are visible.

The after capture is from a clean Unreal Editor restart so no diagnostic view flags or temporary console exposure offsets contaminated the result.

## 6. Multi-Direction PIE Validation

Five representative route locations were tested after allowing `BP_MenuSystemCharacter` to settle through real CharacterMovement collision:

1. Forest entry: `(37640, -15200)`, settled Z `2064.72`
2. First traversal: `(34110, -9020)`, settled Z `1369.85`
3. Deeper forest: `(24689.47, 3843.47)`, settled Z `1153.87`
4. FX/route region: `(9813.65, 14655.41)`, settled Z `2170.40`
5. Terrain-slope region: `(1317.57, 32563.14)`, settled Z `1570.78`

At every location the camera was rotated through yaw `0`, `90`, `180`, and `270` (20 captures total). Across those 20 captures the maximum sampled near-white coverage was 2.50%; no heading recreated an almost-white screen. Representative settled captures:

- [Forest entry](../Saved/Screenshots/WindowsEditor/Screenshot_1786485826.png)
- [First traversal](../Saved/Screenshots/WindowsEditor/Screenshot_1786485832.png)
- [Deeper forest](../Saved/Screenshots/WindowsEditor/Screenshot_1786485838.png)
- [FX/route region](../Saved/Screenshots/WindowsEditor/Screenshot_1786485844.png)
- [Terrain slope](../Saved/Screenshots/WindowsEditor/Screenshot_1786485850.png)

The scene retains its deliberately dark, flashlight-led horror presentation. Fog layers and distant silhouettes remain visible; direct orange magical/player FX remain visible without producing a full-screen bloom veil.

Traversal validation used real input at the slope location:

- Sprint forward changed position from `(1317.57, 32563.14, 1570.78)` to `(-354.75, 30755.35, 1829.61)`, confirming uphill traversal.
- Backward movement settled at `(48.13, 31158.23, 1764.57)`, confirming downhill/return traversal.
- No exposure pumping or white-out occurred during those moves.

## 7. Grounding / Collision Regression

The existing external actor package for `SM_Iceland_Mountain_7.StaticMeshComponent0` still contains collision profile `IgnoreOnlyPawn` and Pawn response `ECR_Ignore`. It was not modified by this lighting task. The terrain-settle and uphill/downhill traversal results also confirm the character remains grounded rather than standing on the oversized mountain convex collision.

## 8. Runtime and Performance Notes

- Removing CH1 bloom avoids an expensive full-screen blur contribution and therefore has a small positive GPU cost impact.
- No new lights, fog volumes, Niagara systems, post-process materials, or per-frame logic were added.
- No new compile errors or lighting-related runtime errors were produced.
- Existing unrelated project diagnostics remain: `BP_AI` Blackboard Accessed None errors, an invalid local `Isabella_Jumpscare` media path, UDP Messaging bind failure, and pre-existing MediaPlate/navigation warnings. They were not changed because they are outside this visual-only task.

## 9. Files / Assets Modified

Task allowlist:

- `Content/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1.umap`
- `Content/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/B/MF/VF3FBPQ2YE5095Z2KMMSVR.uasset`
- `ProjectDocs/SOTM_Forest_Final_Whiteout_Fix_Report.md`

Pre-existing unrelated working-tree items excluded from this task/allowlist:

- `Build/Windows/Application.ico`
- `Content/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/8/09/Y171LVNUAH4OKQHZWFY3W0.uasset` (the already-approved grounding fix)
- `ProjectDocs/SOTM_Forest_Ground_Collision_Fix_Report.md`
- `ProjectDocs/SOTM_Forest_Lighting_Exposure_Fix_Report.md`

## 10. Suggested Commit Message

`fix(ch1): prevent forest white-out from HDR bloom accumulation`
