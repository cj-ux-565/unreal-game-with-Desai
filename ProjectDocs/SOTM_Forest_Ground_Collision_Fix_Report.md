# SOTM Forest Ground / Collision Fix Report

## Scope

This change fixes only player grounding in the production Forest map `CH1`. It does not change the production character, CharacterMovement settings, Forest lighting, post process, fog, foliage, PCG, AI, HUD, objectives, portals, save data, or gameplay scripts.

No package, executable, Git stage, or Git commit was created.

## 1. Reproduction location

- Map: `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`
- Production pawn: `BP_MenuSystemCharacter`
- Reproduced uphill location (before fix): approximately `(X=38862.251, Y=-14287.459, Z=4788.127)`
- The character remained in `MOVE_WALKING` while visibly far above the underlying Landscape.

## 2. Root cause

The player was walking on the simple collision of one large background mountain instance, not on the Forest Landscape.

The mesh `/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Mountain_02` uses `CTF_USE_DEFAULT` and has one convex collision element. The render mesh is extremely large (about 4,198,401 source vertices and 8,388,608 source triangles), while the instance is non-uniformly scaled to `(0.75, 1.75, 1.5)`. Its single convex hull bridges broad concave/empty regions and forms an invisible walkable ramp over the Forest terrain.

Enabling complex-as-simple on this multi-million-triangle asset was rejected as unsafe for production performance. No global movement or capsule workaround was used.

## 3. Hit actor and component

- Actor label: `SM_Iceland_Mountain_7`
- Actor object: `StaticMeshActor_UAID_047C16DF651D80DE02_1822378463`
- Actor GUID: `3DC9B01F45116142B2E50588CF01A508`
- Component: `StaticMeshComponent0`
- Mesh: `/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Mountain_02`
- Actor transform:
  - Location: `(-53610, -34130, -420)`
  - Rotation: `(Pitch=0, Yaw=-100, Roll=0)`
  - Scale: `(0.75, 1.75, 1.5)`
- External actor package: `/Game/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/8/09/Y171LVNUAH4OKQHZWFY3W0`

## 4. Before / after collision findings

### Before

At the reproduced uphill point:

- CharacterMovement floor actor/component: `SM_Iceland_Mountain_7.StaticMeshComponent0`
- CharacterMovement floor impact Z: `4705.983`
- Simple downward trace hit the mountain convex hull at Z: `4687.014`
- Complex downward trace hit `Landscape1.LandscapeHeightfieldCollisionComponent_27` at Z: `1250.742`
- Simple-hull to visible-ground reference gap: approximately `3436.272 uu`
- Capsule feet were approximately `3449.386 uu` above the Landscape reference.

The issue was limited to concave/empty portions covered by this scaled mountain instance's oversized convex hull; it was not caused by global CharacterMovement settings.

### After

At the same XY location in a fresh PIE session using the saved actor setting:

- Player Z: `1341.434`
- Capsule feet Z (88 uu half-height): `1253.434`
- CharacterMovement floor impact Z: `1251.817`
- Capsule-feet to collision-surface separation: approximately `1.617 uu`
- CharacterMovement floor distance: approximately `2.150 uu`
- Hit actor/component: `Landscape1.LandscapeHeightfieldCollisionComponent_27`
- Movement mode: `MOVE_WALKING`

The corrected floor now matches the Landscape surface within normal CharacterMovement floor tolerance instead of following the invisible mountain hull.

## 5. Exact collision setting changed

Only `SM_Iceland_Mountain_7.StaticMeshComponent0` was changed:

- Collision Profile: `BlockAll` -> existing project/engine profile `IgnoreOnlyPawn`

Resolved profile behavior after a full level reload:

- Collision enabled: `QueryOnly`
- Object type: `WorldDynamic`
- Pawn: `Ignore`
- Vehicle: `Ignore` (part of the existing `IgnoreOnlyPawn` profile)
- WorldStatic, WorldDynamic, Visibility, Camera, PhysicsBody, and Destructible: `Block`

This leaves the actor visible and traceable while preventing the production player capsule from using its invalid convex hull. The underlying Landscape remains the physical walking surface. No mesh asset, collision hull, Landscape, global collision channel, or CharacterMovement setting was modified.

## 6. Player traversal validation

PIE checks used the production `BP_MenuSystemCharacter` and the saved actor setting, not a replacement pawn.

- Fresh CH1 load: passed.
- Production entry spawn: passed; the character spawned at `(37640, -15200, 1608.672)` in `MOVE_WALKING` instead of being depenetrated onto the false mountain hull.
- Prior bad uphill location: passed; floor changed to Landscape and the ~3436 uu invisible ramp was removed.
- Uphill traversal: passed; remained `MOVE_WALKING` on `LandscapeHeightfieldCollisionComponent_27`.
- Downhill traversal: passed; remained `MOVE_WALKING` on `LandscapeHeightfieldCollisionComponent_27`.
- Extended/side traversal: passed over several thousand uu without falling through or reacquiring the mountain hull.
- Sprint: passed.
- Jump and landing: passed; returned to `MOVE_WALKING`, zero velocity, walkable Landscape floor, and approximately `2.15 uu` floor distance.
- Sudden bumps / empty-air walking: not reproduced after the fix in the tested route.

Visual capture from the corrected route was saved for local review at `Saved/Screenshots/WindowsEditor/Screenshot_1786476857.png`. The numerical capsule-floor comparison above is the authoritative validation because the production view is first person.

## 7. Navigation validation

- CH1 contains seven `NavMeshBoundsVolume` actors and one `RecastNavMesh-Default` actor.
- The main bounds volume contains the reproduced bad location.
- The affected mountain component's `Can Ever Affect Navigation` value was deliberately left unchanged (`true`), so this scoped collision-profile change does not remove or resize navigation geometry, nav bounds, or enemy routes.
- The Landscape collision components remain navigation-relevant.
- The current static Recast data did not return a projected nav point or synchronous path at either the prior convex elevation or corrected Landscape elevation during the baseline/final audit. Therefore no usable pre-existing path at this coordinate could be claimed or regression-compared.
- No NavMesh rebuild, nav asset save, AI modification, or navigation-volume change was performed.

## 8. Regression results

- CH1 direct load: passed.
- Production pawn spawn and movement: passed.
- Sprint and jump: passed.
- Forest visual rendering, current lighting/exposure, fog, and VFX: unchanged by file allowlist and visually present in PIE.
- Forest boundary actors: unchanged.
- Landscape collision: unchanged and now used as the player floor.
- Health HUD and other gameplay assets/scripts: unchanged; no collision fix code was added.
- Mansion -> Forest transition assets and destination references: unchanged. Direct CH1 destination load and production spawn were verified; the full Mansion portal interaction was not replayed in this focused pass.
- C++: no source files changed and no new C++ compile issue was introduced.
- Blueprint: no Blueprint assets changed. PIE continues to report pre-existing, unrelated `BP_AI` null-Blackboard runtime errors and a Menu System Pro missing Level Meta Data message. These were outside the approved collision scope and were not caused or modified by this fix.
- Focused level validation: CH1 asset exists and loads successfully; no missing reference was introduced on the modified actor.

## 9. Exact files / assets modified by this task

1. `Content/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/8/09/Y171LVNUAH4OKQHZWFY3W0.uasset`
   - Saved collision profile change for `SM_Iceland_Mountain_7.StaticMeshComponent0`.
2. `ProjectDocs/SOTM_Forest_Ground_Collision_Fix_Report.md`
   - This report.

Pre-existing working-tree changes, including the approved Forest lighting external actor, its report, and `Build/Windows/Application.ico`, were preserved and not altered as part of this task.

## 10. Git allowlist

Only the following task-owned paths should be staged when this fix is later approved for source control:

- `Content/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/8/09/Y171LVNUAH4OKQHZWFY3W0.uasset`
- `ProjectDocs/SOTM_Forest_Ground_Collision_Fix_Report.md`

Do not include unrelated pre-existing working-tree changes unless they are handled in their own approved commit.

## 11. Suggested commit message

`Fix CH1 player grounding on Forest landscape`

## 12. Short client update

Fixed the CH1 Forest floating issue at its source. The production player was standing on a single oversized convex hull from `SM_Iceland_Mountain_7`, more than 3400 uu above the Landscape in the reproduced area. That mountain instance now uses the existing `IgnoreOnlyPawn` profile, so the player follows the Landscape while the mountain remains visible and continues blocking normal traces. Fresh PIE validation passed for entry spawn, uphill/downhill traversal, sprint, jump, and landing. No player movement, lighting, AI, UI, or gameplay systems were changed.
