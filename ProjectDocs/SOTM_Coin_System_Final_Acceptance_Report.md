# SOTM Coin System Final Acceptance Report

**Project:** The Secrets of the Mansion — Chapter 1  
**Client task:** Task 4 — Coin System FINAL  
**Date:** 13 August 2026  
**Result:** Final Coin HUD implemented; Coin System accepted in Unreal Editor PIE with the limitations stated below. No package, lighting/environment edit, staging, or commit was performed.

## 1. Original documented Coin requirements

The available client documentation requires magical Forest Coin pickups, tracking of total Coins collected, displaying Coins collected on the HUD, and saving collected Coins where appropriate. The future side panel also names objectives, upgrade requirements, boss progress, and other mission information, but those systems have no authoritative data yet and were intentionally not fabricated.

## 2. Phase 1 functionality reused

The final pass reused the existing `/Game/Blueprints/Coin` integration and all 330 placed CH1 Coins. It did not rebuild collection. Existing behavior remains: Bitcoin mesh presentation, rotation, production-player overlap validation, one authoritative `TryCollectCoin` transaction, duplicate protection, pickup sound, collision shutdown, and disappearance.

Natural PIE traversal through five real existing Coin overlaps produced the authoritative sequence `0 -> 1 -> 2 -> 3 -> 4 -> 5`. The actors disappeared through the real pickup path; no development command was used for this five-Coin traversal.

## 3. Phase 2 persistence reused

The final pass reused the Player State subsystem, SaveGame bridge, stable instance GUIDs, schema-v1 compatibility, New Game reset, and Mansion/CH1 travel behavior from Phase 2. No persistence schema was redesigned, no Coin GUID was generated or regenerated, and the GUID migration script was not run.

## 4. Final Coin HUD implementation

The production widget remains `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI`, parented to `USOTMIngameUIWidget`. The native extension creates a single `SOTM_CoinCounterText` in the existing Canvas rather than replacing the HUD. The Health Bar and unrelated widget hierarchy were not modified.

## 5. Displayed authoritative value

The counter now displays `USOTMPlayerStateSubsystem::GetLifetimeCoinsCollected()`. `AvailableCoins` remains intact for later spending/upgrades, but is not used as the value labeled “Coins Collected.” No spending behavior was added.

## 6. HUD presentation

The final text format is:

`COINS COLLECTED   N`

It is a text-only gold counter with a dark shadow, left justification, font size 22, at the existing safe top-left location `(36, 36)`. Its Canvas allocation is `360 x 52`, enlarged from the Phase 1 placeholder to accommodate three-digit values without colliding with the existing Health display.

An asset audit found no clearly suitable UI-ready Coin icon. The Fab Bitcoin content provides the 3D Coin mesh/material and raw supporting textures, not a finished UMG icon. Per the requirement, no arbitrary material texture or external artwork was introduced.

## 7. Pickup feedback

On a genuine increase, the counter briefly scales to `1.12` and brightens, then returns to scale `1.0` and its normal gold color after `0.16` seconds. The timer is cleared safely during widget destruction. Initialization, load, and reset synchronization do not falsely pulse unless the authoritative lifetime count actually increases. No elaborate VFX or `+1` architecture was added.

## 8. Event-driven update architecture

The existing flow is preserved:

`Coin overlap -> authoritative TryCollectCoin -> OnCoinsChanged -> HUD refresh`

There is no Tick, per-frame binding, actor polling, or widget-local Coin total. `NativeConstruct` uses `RemoveDynamic` followed by `AddDynamic`; `NativeDestruct` removes the binding. This makes widget recreation duplicate-safe.

## 9. Initial HUD synchronization

`NativeConstruct` immediately reads `LifetimeCoinsCollected`. Observed live results included:

- New Game reset: authoritative `(Available, Lifetime, IDs) = (0, 0, 0)`, HUD immediately `COINS COLLECTED   0`.
- Isolated saved state: load returned true with `(6, 6, 6)`, HUD immediately `COINS COLLECTED   6` without another pickup.

## 10. Save/load HUD test

The Phase 2 isolated slot `SOTM_CoinPhase2_Automation` was used; client save slots were not touched.

- Loaded state: `6`, HUD `COINS COLLECTED   6`.
- A new valid existing Coin ID was collected through the authoritative production API: `6 -> 7`.
- Save returned true.
- After reset/reload: `(Available, Lifetime, IDs) = (7, 7, 7)`, HUD immediately `COINS COLLECTED   7`.

Natural overlap behavior was independently proven by the `0 -> 5` traversal. The isolated `6 -> 7` step validated the save/HUD bridge, not physical traversal.

## 11. Respawn HUD test

With seven collected Coins, the existing Player System was tested through partial damage, fatal damage, death presentation, and normal checkpoint respawn:

- Health `100 -> 75` after 25 damage.
- Fatal damage set the Player dead while the Coin HUD remained `7`.
- After the existing death presentation, respawn completed with Health `100`, Player dead false, lifetime Coins `7`, HUD `COINS COLLECTED   7`.
- Exactly one production HUD widget and one Coin counter were present after respawn.

No Player System or Health HUD implementation was changed.

## 12. Map-travel HUD test

PIE runtime travel was tested in both directions:

- CH1 before leaving: lifetime `7`, one HUD counter.
- Mansion: lifetime `7`, HUD immediately `COINS COLLECTED   7`, one HUD widget.
- Return to CH1: lifetime `7`, HUD `COINS COLLECTED   7`, one HUD widget; 330 Coin actors loaded, seven recorded collected and seven hidden.

This confirms authoritative GameInstance-subsystem state and clean HUD recreation across travel without a zero flash or duplicated widget.

## 13. New Game reset test

The existing production `ResetRuntimeStateForNewGame` path produced `(0, 0, 0)`. The event-driven HUD immediately rendered `COINS COLLECTED   0`; widget text was not manually reset. The isolated six-Coin test slot was then reloaded to restore the test state.

## 14. Natural multi-Coin collection test

Five existing CH1 Coin actors were collected by moving the production player through their real overlap volumes. Available and lifetime totals both reached five, and live inspection found exactly one counter rendering `COINS COLLECTED   5`. The Coin class’s real collision/overlap, authoritative award, sound, and disappearance path was used.

## 15. 9-to-10 and 99-to-100 layout test

Higher digit boundaries were exercised in Development PIE through unique existing Coin IDs without altering `CoinValue`:

- `9 -> 10`: rendered `COINS COLLECTED   10`.
- `99 -> 100`: rendered `COINS COLLECTED   100`.
- At `100`, live inspection captured pulse scale `1.12`; 400 ms later it was `1.0` and text remained correct.

No clipping was observed in the fixed `360 x 52` allocation at these values.

## 16. Resolution/layout test

The normal Editor PIE presentation was inspected: the top-left counter remained visible, separate from Health, and readable through one-, two-, and three-digit values.

A second live viewport resolution could **not** be captured: the installed Unreal MCP advertises `set_viewport_resolution` and `set_resolution`, but returned `UNKNOWN_ACTION` and `NOT_IMPLEMENTED` respectively. Therefore this report does not claim a measured second-resolution pass. Static layout evidence shows a top-left `(0,0)` anchor, `(0,0)` alignment, position `(36,36)`, and fixed `360 x 52` slot, which should preserve the top-left placement across common resolutions; this is not a substitute for the unavailable second-resolution visual capture.

Recommended manual check: run PIE/Standalone at `1280x720` once and confirm the counter remains visible and does not overlap Health.

## 17. Persistent-ID validation

Validation-only inspection in the live CH1 world reported:

- total Coin actors: `330`;
- valid non-zero GUIDs: `330`;
- unique GUID strings: `330`;
- at the validation point: `8` collected/hidden and `322` visible/uncollected.

No GUID or Coin transform was changed during validation.

## 18. Coin actor regression

- Existing Bitcoin visual: retained.
- Rotation: existing rotating presentation retained.
- Natural production-player detection: passed with five real overlaps.
- Invalid collector: rejected; lifetime stayed `7`.
- First valid authoritative collection: accepted; `7 -> 8`.
- Duplicate attempt with the same stable ID: rejected; total stayed `8`.
- Award value: existing provisional `CoinValue = 1` retained.
- Pickup sound and disappearance: existing Phase 1 path retained and exercised during natural collection.
- Persistence: collected actors were hidden after load/travel; uncollected actors remained visible and collectible.

## 19. Health and player regression

Health decrease, death, normal respawn, and full-health restoration passed as described in section 11. Forest movement input was routed to the correct production pawn; `W` changed world location, and Space produced a measured upward velocity of `127.44` with increased Z. Sprint input (`LeftShift + W`) was accepted and moved the pawn, though the short automation sample ended against nearby world collision and is not a reliable speed comparison. No movement/camera code was modified.

The affected production Blueprints compiled successfully outside PIE without saving:

- `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI`
- `/Game/Blueprints/Coin`
- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject`

The `SOTM1Editor Win64 Development` C++ target also built successfully after the HUD changes. No new Coin/HUD Blueprint or C++ compile error was introduced.

Unrelated existing runtime warnings remain in CH1: legacy `BP_AI` accesses a missing Blackboard return value, some AI movement reports collisions/stuck movement, and `Isabella_Jumpscare` references a missing developer-local media file. These were not caused by or modified in this Coin task.

## 20. Lighting/environment confirmation

**No lighting, exposure, fog, bloom, post process, Lumen, sky, camera post process, render show flag, environment, foliage, PCG, terrain, mountain, portal, PlayerStart, or other environment setting was modified.** No CH1 map or external actor was saved by the final HUD pass. Lighting differences seen during testing were ignored as required.

## 21. Remaining client/document ambiguities

- Whether a future HUD should show lifetime collected, spendable balance, or both after spending begins.
- Coin costs and thresholds for Speed, Lightning Throw, Purple Burst, and other upgrades.
- Whether the word “Coins” or story wording such as “fragments” should be final in UI/voice presentation.
- Whether the client wants a bespoke Coin HUD icon later; no UI-ready icon currently exists.
- `CoinValue = 1` remains a safe provisional implementation value, not a client-approved balance decision.

These belong to future Upgrade/Objectives/HUD milestones and were not implemented here.

## 22. Exact files/assets modified by this final pass

Intentionally modified:

- `Source/SOTM1/Public/UI/SOTMIngameUIWidget.h`
- `Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp`
- `ProjectDocs/SOTM_Coin_System_Final_Acceptance_Report.md` (created)

No `.uasset`, map, external actor, lighting asset, Coin instance, or save schema file was intentionally modified by this final pass. Other dirty worktree files belong to the already completed Phase 2 implementation and must be handled according to its separate allowlist.

## 23. Exact Git allowlist for the final HUD commit

Stage only:

```text
Source/SOTM1/Public/UI/SOTMIngameUIWidget.h
Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp
ProjectDocs/SOTM_Coin_System_Final_Acceptance_Report.md
```

Do not stage `.codex_mcp_session`, `Saved`, generated output, or unrelated modified assets as part of this final HUD commit. This task staged and committed nothing automatically.

## 24. Suggested commit title

`Finalize production Coin HUD and acceptance report`

## 25. Suggested commit description

`Display LifetimeCoinsCollected in the production in-game HUD, add a lightweight event-driven pickup pulse, preserve duplicate-safe delegate lifecycle and immediate load/travel synchronization, and document final Coin System PIE acceptance. No lighting, environment, map, persistence, or unrelated gameplay changes.`

## 26. Client-facing completion update

Client Task 4 is complete in Editor acceptance. The existing Forest Coins now have a clean production HUD displaying lifetime Coins collected, update immediately through the authoritative Coin event, and give a subtle pickup pulse. Natural pickups reached 0–5, save/load restored the HUD immediately, collected Coins stayed gone, New Game reset to zero, respawn and Mansion/Forest travel preserved state, and all 330 persistent IDs remain valid and unique. The existing Health and movement systems remained functional. No lighting, environment, Coin placement, or unrelated gameplay system was changed. A second-resolution visual check remains a short manual step because the installed viewport-resolution tool is not implemented.
