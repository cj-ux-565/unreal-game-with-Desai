# SOTM Client Demo Phase 3 — Timmy Speed Boost Report

Date: 2026-08-18

Engine: Unreal Engine 5.6.1
Scope: Timmy Upgrade Station, spendable Coin purchase, Speed Boost Level 1, HUD states, and persistence only

## 1. Approved Phase 3 HUD reference

`Phase3HUD.png` was inspected and used as the visual specification. The implementation follows its dark gothic panel, purple magical accents, gold Coin treatment, centered ability hierarchy, Locked/Owned states, tuning rows, action button, station prompt, and LOCKED/READY/ACTIVE/COOLDOWN gameplay presentation. The PNG is not pasted into the game; every displayed value and state is a functional widget value.

## 2. Timmy Upgrade Station architecture

`ASOTMTimmyUpgradeStation` is a small, reusable native station actor containing a spherical interaction volume and a world-space station label. `USOTMDemoPhase3WorldSubsystem` creates one transient station in CH1 at the real production HorrorBear/Timmy nearest the production Player Start. Runtime evidence found and used `/Game/HorrorBear/Mesh/SKM_HorrorBear` at `(37320, -15300, 1520)`, approximately 347 units from the initial player.

No production map or external actor was changed. The station is CH1-only and is cleaned up with its world.

## 3. Interaction architecture

- Reuses production `IA_Interact` from `/Game/MenuSystemPro/Blueprints/Input/CharacterOnFoot/IA_Interact`.
- The audited production mapping is `E` (plus the existing gamepad binding).
- The station broadcasts enter/exit events; there is no station Tick.
- The prompt is `[E] INTERACT / UPGRADE ABILITIES`.
- One upgrade widget can exist at a time.
- Opening acquires the existing Player System `Custom` input lock, uses Game-and-UI input, and enables the cursor.
- Closing releases only that lock, removes only its widget, restores the prior cursor state, and returns to Game-only input.

## 4. Coin economy architecture

The existing `USOTMPlayerStateSubsystem` remains authoritative. Phase 3 uses the existing separation between:

- `LifetimeCoinsCollected`: permanent progression evidence used by Collect All Coins.
- `AvailableCoins`: spendable currency used by purchases.

The upgrade widget reads both through the subsystem. Purchase authority is not in the widget.

## 5. AvailableCoins implementation

No duplicate widget currency was created. `TryPurchaseSpeedBoost()` validates the objective, cost, balance, ownership, and active production save context before changing state. Only `AvailableCoins` is deducted. The top-right gameplay `COINS` counter now displays available currency, while `COINS COLLECTED` and objective progress continue to display lifetime collection.

## 6. LifetimeCoinsCollected relationship

The completed acceptance transaction produced:

- Before: available 330, lifetime 330.
- Cost: 250.
- After: available 80, lifetime 330.
- Collect All Coins: still COMPLETE at 330/330.

Spending cannot undo the objective because `USOTMObjectiveSubsystem` derives progress from lifetime/unique collected state, never from available currency.

## 7. Backward-compatible save migration

Player save schema is version 3.

- Version 1 remains accepted with no Coin/ability data.
- Version 2 remains accepted. Because v2 had no spending, an older save with lifetime progress but missing/zero available currency migrates available currency from lifetime once.
- Version 3 persists the Speed Boost record in the existing versioned `SOTM_CollectedCoinIds` string as a namespaced `SOTM_SPEEDBOOST|unlocked|level` row. Coin GUID rows remain unchanged.
- Version 3 never repeats the v2 currency credit, preventing post-purchase double credit.

This avoids a destructive SaveGame Blueprint reparent and avoids creating a second competing save system.

## 8. Speed Boost unlock requirements

Authoritative requirements are all enforced:

1. Collect All Coins is complete.
2. `AvailableCoins >= SpeedBoostUnlockCost`.
3. Speed Boost is not already owned.
4. Cost is positive.
5. A valid production save context exists.

The station can still be previewed early. It displays `COLLECT ALL COINS FIRST` or `NOT ENOUGH COINS` as appropriate.

## 9. Unlock cost configuration

`USOTMPhase3Settings` centralizes the demo tuning:

- Unlock cost: 250 Coins.
- Speed multiplier: 1.40x.
- Duration: 3.0 seconds.
- Cooldown: 10.0 seconds.
- Station radius: 475 units.

These match the approved mockup/conservative requested range but remain tunable config defaults, not claimed as permanent client balancing.

## 10. Purchase transaction flow

`USOTMPlayerStateSubsystem::TryPurchaseSpeedBoost()` performs an atomic transaction:

1. Validate ownership, objective, cost, balance, and active save.
2. Deduct available currency.
3. Set unlocked and Level 1.
4. Save through the existing Menu System Pro active-slot save path.
5. On save failure, roll back the balance, ownership, and level.
6. Only after successful persistence, broadcast Coin and ownership delegates.

The upgrade UI and gameplay HUD update from those delegates.

## 11. Duplicate purchase protection

The first tested purchase returned `Success`; an immediate second request returned `AlreadyOwned`. Balance changed exactly once from 330 to 80. Lifetime stayed 330. No second save/deduction or duplicate ability instance occurred.

## 12. Level 1 implementation

Only Speed Boost Level 1 is functional, as required. Ownership and integer level persistence are future-safe, but Levels 2/3 and a general skill tree were not implemented.

## 13. Speed multiplier

Level 1 uses a configurable 1.40x multiplier. Acceptance logs captured base `MaxWalkSpeed=600` and active `MaxWalkSpeed=840`.

The runtime does not permanently replace base speed. While active, it recognizes legitimate external changes (including sprint/base movement writes), treats the changed value as the new unboosted base, applies the multiplier, and restores the latest base when the effect ends.

## 14. Duration

The configurable active duration is 3.0 seconds. A game-time timer owns the state transition; no gameplay Tick was added.

## 15. Cooldown

The configurable cooldown is 10.0 seconds. Re-activation during cooldown returned false. When the timer expired, the authoritative state returned to READY.

## 16. HUD states

The production gameplay HUD is event-driven and supports:

- `LOCKED` (red)
- `READY [Q]` (green)
- `ACTIVE` with remaining time/progress (cyan/purple)
- `COOLDOWN` with remaining time/progress (purple)

`Q` was selected after auditing input because Left Shift is already Sprint, `E` is Interact, and `F` is Skill Tree. The transient Enhanced Input mapping is installed only while the CH1 Phase 3 subsystem is active and is removed on world cleanup.

## 17. Objective relationship

Collect All Coins remains a Phase 2 objective and remains complete after spending. No Phase 4 objective was activated. The future Chest, Gate Key, and Gate rows remain display-only locked rows already present in the approved Phase 2 HUD.

## 18. Death, respawn, Game Over, and pause behavior

- Speed Boost grants no invulnerability.
- Death closes the station UI, clears active/cooldown/presentation timers, restores base movement speed, and retains purchased ownership.
- A lethal Player System damage test while ACTIVE consumed exactly one life (5 to 4), reset speed from 840 to 600, and produced READY after normal respawn.
- Game Over uses the same cleanup callback; no new death/life/respawn logic was added.
- Retry/respawn ownership comes from persistent Player State and returns to READY.
- Active/cooldown use world game-time timers, so they follow the existing pause semantics rather than advancing on wall-clock time.
- Existing Esc/Tab pause code and mappings were not modified.

## 19. Continue/relaunch result

A separate Unreal process loaded the saved Phase 3 test slot and restored:

- available 80;
- lifetime 330;
- objective 330/330 COMPLETE;
- Speed Boost unlocked;
- Level 1;
- runtime READY;
- base movement speed 600;
- no dead/Game Over state.

The exact 1920x1080 capture visually showed `COINS 80`, `COINS COLLECTED 330`, objective COMPLETE, and Speed Boost READY.

## 20. Save-slot isolation result

Two explicit temporary test slots were used:

- Test Slot 1: unlocked Level 1, available 80, lifetime 330.
- Test Slot 2: New Game state, locked Level 0, available 0, lifetime 0.

Loading Slot 1, then Slot 2, then Slot 1 restored the correct independent values every time. The two test save files were removed afterward.

## 21. New Game reset result

`ResetRuntimeStateForNewGame()` now explicitly resets unlocked=false, level=0, available=0, lifetime=0 and broadcasts the Coin/ownership events. The clean test slot restored locked/zero state and did not inherit Slot 1.

## 22. Cousin chase balance result

The acceptance route positioned the living production player within perception range of a naturally spawned Phase 2 Cousin. The Cousin entered detected/chase state through perception, while Boost activated at 1.40x. The visual/log route confirmed no invulnerability and no changes to Cousin tuning. A surrogate lethal Player System hit while Boost was active verified runtime cleanup and one-life loss. The full player-controlled run-distance feel should still receive final client balancing feedback; 1.40x/3s/10s is intentionally conservative.

## 23. Phase 1 regression

Main Menu and Mansion production maps loaded with map check 0 errors/0 warnings. The production character Blueprint compiled. No Phase 1 source, map, cinematic, Timmy introduction, Isabella introduction, knockout, dragging, menu, or pause asset was modified. The previously accepted full Phase 1 cinematic route was not replayed end-to-end during this focused no-package pass; a final human click-through remains recommended before the client demo.

## 24. Phase 2 regression

Validated in the Phase 3 CH1 runtime route:

- production Phase 2 HUD present;
- objective active and completion states updated from authoritative lifetime state;
- eight production Cousins initialized;
- natural perception entered detected/chase;
- lethal damage consumed one life;
- normal respawn completed;
- Phase 3 death cleanup did not replace Phase 2 catch/death ownership.

Production Coin, HUD, portal, character, health-bar, and legacy AI Blueprints compiled successfully. Full manual collection of all 330 physical Coins and the complete jump-scare camera presentation were not repeated; Phase 2's prior acceptance remains the source for those exhaustive checks.

## 25. UI resolution tests

- 1280x720: station prompt and Locked, requirement, Not Enough Coins, purchase-ready, Owned, Active, Cooldown, chase, and post-respawn views rendered without clipping.
- 1920x1080: exact offscreen viewport capture rendered the persisted READY HUD without clipping and with available/lifetime values clearly separated.

Evidence is stored under ignored generated output at `Saved/Screenshots/WindowsEditor/Phase3/` and is not part of the Git allowlist.

## 26. Errors found and fixed

1. Initial source compile found an incorrect SkeletalMeshActor include; corrected to the UE 5.6 header.
2. Initial subsystem compile used a non-existent direct `GetGameInstance()` world-subsystem call; corrected through `GetWorld()`.
3. Acceptance found Speed Boost save metadata was parsed after ownership fields were assigned, causing relaunch to restore locked. Assignment now occurs after metadata parsing; two-slot and separate-process relaunch passed.
4. Visual review found top-right `COINS` still displayed lifetime instead of available currency. It now displays 80 after the 250 purchase while objective/lifetime remains 330.
5. The first validation commandlet invocation used a backslash Python path interpreted as `\u`; rerun with a forward-slash path succeeded. This was tooling invocation only.

Final Editor build: succeeded.

Production Blueprint compile: six of six succeeded.

Production map checks: Main Menu, Mansion, CH1 each 0 errors/0 warnings.

Read-only validation dirty maps: none.
Runtime Blueprint errors introduced: none observed.

Known pre-existing warnings:

- `UCrowdManager` may initialize before a RecastNavMesh is available during editor map loading.
- CH1 serialized NavMesh `maxTiles` differs from the calculated value.
- The unrelated Isabella jump-scare media source still references a missing developer-local video path.

## 27. Exact files created or modified

- `Phase3HUD.png` (approved reference supplied in the project root)
- `Source/SOTM1/SOTM1.Build.cs`
- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/Ability/SOTMSpeedBoostTypes.h`
- `Source/SOTM1/Public/Ability/SOTMPhase3Settings.h`
- `Source/SOTM1/Public/Ability/SOTMTimmyUpgradeStation.h`
- `Source/SOTM1/Private/Ability/SOTMTimmyUpgradeStation.cpp`
- `Source/SOTM1/Public/Demo/SOTMDemoPhase3WorldSubsystem.h`
- `Source/SOTM1/Private/Demo/SOTMDemoPhase3WorldSubsystem.cpp`
- `Source/SOTM1/Public/UI/SOTMUpgradeStationWidget.h`
- `Source/SOTM1/Private/UI/SOTMUpgradeStationWidget.cpp`
- `Source/SOTM1/Public/UI/SOTMIngameUIWidget.h`
- `Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp`
- `ProjectDocs/SOTM_Demo_Phase3_Timmy_SpeedBoost_Report.md`

No `.uasset`, `.umap`, config, lighting, environment, chest, gate, boss, weapon, or later-ability file was modified.

## 28. Exact Git allowlist

Stage only the 15 files listed in section 27. Do not stage `Saved/`, `Intermediate/`, `Binaries/`, DerivedDataCache, logs, screenshots, temporary saves, or any unrelated asset.

No files were staged or committed automatically.

## 29. Suggested commit title

`feat: complete demo phase 3 Timmy speed boost upgrade`

## 30. Suggested commit description

`Add the CH1 Timmy upgrade station and functional approved Phase 3 UI, atomically spend available Coins after Collect All Coins, persist Level 1 Speed Boost per save slot, add Q-driven timed boost states and HUD feedback, and safely clear runtime effects on death/respawn.`

## 31. Client-facing progress update

Phase 3 is implemented: players can preview Timmy's Upgrade Station, finish Collect All Coins, spend 250 Coins to buy Speed Boost Level 1, and use the conservative 1.40x boost for 3 seconds with a 10-second cooldown. The HUD shows Locked, Ready, Active, and Cooldown states. Spending preserves the 330/330 completed objective while reducing spendable Coins to 80. Unlock ownership and balance survive Continue/relaunch, remain isolated across save slots, and reset correctly for New Game. No Forest lighting or later demo gameplay was changed.

## Acceptance boundary

This phase stops at Speed Boost. Chest, Gate Key, Gate, demo ending, Dark Magic, Lightning Throw, sword combat, Isabella boss, Spider Mother, credits, and later progression were not implemented.
