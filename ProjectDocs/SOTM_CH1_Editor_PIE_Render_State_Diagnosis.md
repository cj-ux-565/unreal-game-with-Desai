# CH1 Editor / PIE Render-State Diagnosis

Date: 2026-08-12

## Result

The white/blue clipped PIE image was caused by contaminated transient render state in the long-running Unreal Editor session, not by a saved CH1 lighting asset or a BeginPlay lighting override.

A graceful editor restart cleared the state. Fresh PIE then matched the existing dark CH1 editor appearance without changing CH1 lights, materials, gameplay camera assets, Blueprints, or runtime systems.

## Controlled comparisons

- Global PostProcessVolume hidden: no meaningful improvement.
- ExponentialHeightFog hidden: no meaningful improvement.
- Unlit view: clipping remained, ruling out ordinary scene-light interaction.
- Gameplay camera Post Process Blend Weight changed from `1.0` to `0.0`: color cast changed, but extreme clipping remained; camera settings were not the root cause.
- No runtime-created Light or PostProcessComponent actors were found.
- Editor and post-BeginPlay values were identical for the directional light, both skylights, and global post-process volume.
- Fresh editor restart, with no CH1 asset changes: corrected PIE immediately.
- Second fresh PIE run: corrected result persisted.

## Editor and PIE actor values

Values were identical before PIE and after BeginPlay:

- Directional Light `Light Source`: intensity `3.773698`, color `(R=170,G=156,B=177)`, indirect intensity `1.0048`, volumetric scattering `1.0`.
- SkyLight_1: intensity `20.0`, color `(0,0,0)`, indirect intensity `1.0`, volumetric scattering `1.0`.
- SkyLight_5: intensity `1.92`, color `(R=39,G=69,B=24)`, indirect intensity `0.428798`, volumetric scattering `0.735999`.
- PostProcessVolume_1: priority `0`, blend weight `1`, unbound `true`; exposure min/max `0.25`, bias `0`, bloom intensity `0`.
- Gameplay camera post-process blend weight at fresh PIE: `1.0`.

## Evidence images

- Supplied broken PIE reference: `C:/Users/menon/Downloads/Codex Image Aug 12, 2026, 08_08_47 AM.png`
- Supplied visual authority: `C:/Users/menon/Downloads/actual.png`
- Captured editor baseline: `Saved/Screenshots/Screenshot_20260812_081423.png`
- Fresh corrected PIE interior: `Saved/Screenshots/WindowsEditor/Screenshot_1786525036.png`
- Fresh corrected PIE exterior: `Saved/Screenshots/WindowsEditor/Screenshot_1786525152.png`
- Second-run corrected PIE interior: `Saved/Screenshots/WindowsEditor/Screenshot_1786525353.png`

## Permanent changes

No lighting, material, Blueprint, gameplay, collision, or camera asset was modified for this diagnosis. The minimum fix was clearing the contaminated editor runtime state through a graceful editor restart.

