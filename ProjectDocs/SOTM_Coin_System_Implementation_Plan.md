# SOTM Coin System Implementation Plan

**Project:** The Secrets of the Mansion — Chapter 1  
**Task:** Client Task 4 — Coin System documentation review only  
**Date:** 12 August 2026  
**Status:** Planning only; no Unreal asset, map, Blueprint, C++, build, package, Git stage, or commit operation was performed.

## 1. Documents reviewed

### Fully reviewed requirement sources

- `ProjectDocs/Client_Requirements.txt`
- `ProjectDocs/CHAPTER 1 — Voice Actor Lines.pdf` — 2/2 pages, text extraction and rendered-page review.
- `ProjectDocs/Mansions Quests_ Chapter 1.pdf` — 8/8 pages, text extraction and rendered-page review.
- `ProjectDocs/Project_Audit_Instructions.pdf` — 1/1 page, text extraction and rendered-page review.

### Project-state reports reviewed for Coin-related evidence

- `ProjectDocs/SOTM_Project_Audit_Report.md` — requirements summary, architecture, Task 4, HUD/save findings, estimates, tests, and clarification questions.
- `ProjectDocs/SOTM_Player_System_Foundation_Report.md` — current `USOTMPlayerStateSubsystem` and SaveGame bridge.
- `ProjectDocs/SOTM_Performance_Audit_Report.md` — Forest actor/tick measurements and deferred Coin optimization.
- `ProjectDocs/SOTM_Foundation_Map_Ownership.md` — authoritative production maps and the 330-coin Forest population.
- `ProjectDocs/SOTM_Foundation_Cleanup_Report.md` — confirmation that Coin gameplay was not implemented during foundation work.
- The remaining Markdown reports were searched for Coin/currency/fragment references. Their Coin references are scope-exclusion statements or confirmation that later lighting/AI/death work did not change Coin gameplay; they introduce no additional Coin requirements.

### Known unavailable/incomplete sources

`Project_Audit_Instructions.pdf` names `GoogleDoc_1.pdf` and `GoogleDoc_2.pdf`, but neither file exists in `ProjectDocs`. Their contents could not be reviewed. This is a material documentation gap and prevents treating the available requirements as necessarily exhaustive.

`ProjectDocs/VID-20260728-WA0002.mp4` is a visual reference rather than a written Coin contract. It was not re-analysed for this documentation-only plan; the earlier project audit’s implementation findings were used where relevant. No Coin behavior, value, save rule, or HUD wording is inferred from the video.

## 2. Source-of-truth hierarchy

The documentation does not publish a formal revision table. The safest hierarchy is:

1. **`Client_Requirements.txt`** — explicitly titled “LATEST CLIENT REQUIREMENTS”; authoritative for magical coins as currency, the side-panel requirement, and Chapter 1 currency carrying into the Chapter 2 setup.
2. **Latest client update embedded in `Project_Audit_Instructions.pdf`** — authoritative where it adds the later requirement that Timmy/Purple Burst uses collected Chapter 1 coins.
3. **`Mansions Quests_ Chapter 1.pdf` and `CHAPTER 1 — Voice Actor Lines.pdf`** — authoritative story and presentation intent, but they call the collectibles glowing golden “fragments” or “pieces,” not coins.
4. **Current project files and post-audit technical reports** — authoritative for what is implemented now, not for intended design.
5. **`SOTM_Project_Audit_Report.md`** — a derived technical assessment, not a client requirement source.

The missing Google documents may contain newer or more precise rules. No silent resolution should be made where the available sources conflict or omit detail.

## 3. Exact documented Coin System requirements

### 3.1 Magical Coin pickups

Explicitly documented:

- The player recovers powers by collecting **magical coins** in the Forest.
- The quest/voice documents describe **glowing golden fragments** that hold pieces of Speed and Lightning.
- Collectibles are scattered around the Forest.
- Coins act as currency for later ability progression.

Not explicitly documented:

- Coin value per pickup.
- Exact pickup sound, VFX, animation, magnetism, collection radius, or feedback duration.
- Whether a pickup must be destroyed versus hidden after collection.
- Whether every one of the 330 placed actors is intended to be collectible.
- Whether collected instances must stay absent after checkpoint reload or application restart.

Pickup sound, disappearance, player validation, one-award-only protection, and stable collected-instance persistence are therefore technical requirements/recommendations needed for correctness, not all separately stated client presentation requirements.

### 3.2 Total coins collected

Explicitly documented:

- The HUD side panel must display **Coins collected**.
- Coins are a currency used for purchases in later systems.
- Chapter 1 coins must remain available for the later Mansion/Timmy/Purple Burst setup.

Not defined:

- Starting value. The safest default is `0`, but this remains an implementation default rather than a documented number.
- Whether “Coins collected” means current spendable balance, lifetime collected count, or both.
- Whether spending later reduces the HUD value.
- Coin costs, thresholds, refund behavior, caps, or negative-balance rules.

The documents support two distinct future concepts—**available balance** and **lifetime Chapter coins collected**—but do not define their display semantics. This requires client clarification before purchase logic exists. For this four-requirement milestone, pickup can increment both fields while no spending API is exercised.

### 3.3 HUD

Explicitly documented:

- A **side-panel HUD** must show Coins collected.
- The same intended panel eventually contains current objective, coins required for upgrades, boss progress, and other tasks.

This task should add only the current coin value. “Coins required for upgrades” belongs to the later upgrade/objective work and is outside scope.

No exact label, typography, icon, screen position, animation, or formatting is specified. The production character currently references `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI`; that is the narrowest existing production HUD asset to extend. The older audit path under `Designs/Design_Silence/Widgets/Ingame` does not exist in the current content tree.

### 3.4 Save/persistence

Explicitly documented or necessarily implied by the approved flow:

- Chapter 1 collected currency must survive the later return to Mansion and Chapter 2 setup.
- “Save collected coins when appropriate” requires disk persistence through the existing save system.

Not defined:

- Save immediately on every pickup, at checkpoints, on map transition, through manual save, or some combination.
- Whether death before a checkpoint rolls back recently collected coins.
- Whether New Game clears all coin data.
- Whether individual pickup IDs must persist.

Because saving only a balance while respawning all placed pickups creates repeatable currency, persistent pickup IDs are technically required if the approved behavior is that collected coins remain gone after load. That behavior is sensible but not stated explicitly; it must be confirmed.

## 4. Requirement ambiguities and conflicts

| Topic | Evidence | Status |
|---|---|---|
| Coin versus fragment | Latest client text says magical coins; quest and voice PDFs say glowing golden fragments/pieces of powers. | **CLIENT/DOCUMENT CLARIFICATION REQUIRED.** Treat as one collectible only after confirmation. |
| Balance versus lifetime total | Coins are currency, but HUD wording is “Coins collected.” | **CLIENT/DOCUMENT CLARIFICATION REQUIRED.** Preserve both concepts internally if approved. |
| Spending and HUD reduction | Later purchase intent exists; no display rule exists. | Out of scope now; must be decided before upgrades. |
| Starting total | No number is given. | Recommend `0` as a reversible default. |
| Per-pickup value | No number is given. | Recommend configurable `CoinValue=1`, not a hard-coded irreversible rule. |
| Save moment | Carry-over is required, save timing is not. | Recommend dirty-on-pickup plus save on existing manual/checkpoint/map-exit paths; optional immediate save is a policy choice. |
| Collected pickup respawn | No explicit statement. | Confirm; persistent IDs are needed to prevent farming after load. |
| All 330 placements | Existing production Forest contains 330 Coin actors; documents give no required count. | Do not add/remove/reposition during this milestone. Validate existing instances only. |
| Pickup feedback | Existing Blueprint has sound; documents do not specify it or VFX. | Reuse sound if acceptable; do not invent final VFX. |
| Missing requirements | Two Google documents named by the audit package are absent. | Obtain exports before final acceptance if possible. |

## 5. Existing project Coin architecture

### Current production path

`/Game/Blueprints/Coin`
→ placed as 330 World Partition external actors in
`/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1`

The Coin Blueprint contains:

- a static-mesh component using `/Game/Fab/BitCoin/bitcoin/StaticMeshes/bitcoin`;
- a pickup-area collision component;
- a `RotatingMovementComponent`;
- overlap event metadata;
- `PlaySound2D` using `/Game/Sound_FX/ScreenRecording_10-16-2025_01-55-20`;
- actor destruction logic.

What currently works according to the prior audit and current asset metadata:

- visual mesh exists;
- coins are already placed throughout the production Forest;
- rotation exists;
- overlap response, pickup sound, and disappearance/destruction exist.

What does not exist:

- authoritative coin state;
- increment/transaction API;
- production-player validation;
- one-award-only guard;
- coin-changed event;
- HUD binding;
- SaveGame fields;
- collected-instance persistence;
- map-travel restoration;
- duplicate-ID validation.

### Player and Chapter state

`USOTMPlayerStateSubsystem` is an existing `UGameInstanceSubsystem`. It currently owns authoritative player lives, health, checkpoint, death/Game Over state, input locks, map-entry handling, and save/load bridging. Its lifetime already spans normal map travel. It is the correct existing owner to extend for Coin state; `BP_MenuSystemCharacter` should not become the authority.

`USOTMPlayerBlueprintLibrary` already exposes stable Blueprint-facing access to the player-state subsystem. It is the appropriate place for narrow Coin query/collection helpers if Blueprint ergonomics require them.

`BP_MenuSystemCharacter` contains a legacy variable named `How many coins`, but no verified end-to-end currency graph uses it. It must not be authoritative. At most, it may be synchronized temporarily for compatibility after a confirmed reference search; new logic should read the subsystem.

### Save architecture

The project retains Menu System Pro’s:

- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_SaveGameManager`
- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject`

The player foundation already extends that object additively with `SOTM_*` fields and reads/writes them reflectively from `USOTMPlayerStateSubsystem`. Coin data can follow the same tested bridge instead of creating a second SaveGame system.

Current save schema version is `1` and contains no Coin fields.

### HUD

The production character references:

`/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI`

It currently contains marketplace health/ammo presentation and explicitly describes some content as cosmetic. It has no Coin widget/binding. No dedicated Coin-counter widget was found.

## 6. Existing reusable assets

| Asset/system | Reuse decision | Reason |
|---|---|---|
| `/Game/Blueprints/Coin` | **Repair and reuse** | All 330 production placements already depend on this class. Replacing the class or map placements would create unnecessary World Partition risk. |
| Fab Bitcoin mesh/material/textures | **Reuse provisionally** | Already used by the Coin Blueprint. Visual approval is separate from functionality. |
| Existing pickup sound | **Reuse provisionally** | Existing feedback is better than removing it; client/audio approval is still needed. |
| `USOTMPlayerStateSubsystem` | **Extend** | Existing authoritative, map-persistent state and save bridge. |
| `USOTMPlayerBlueprintLibrary` | **Extend minimally if needed** | Stable Blueprint API; prevents Coin actor from knowing player internals. |
| `BP_CustomSaveGameObject` | **Extend additively** | Existing production save payload already carries `SOTM_*` player state. |
| `WBP_IngameUI` | **Extend narrowly** | Production HUD created/referenced by the production character. |

No dedicated 2D Coin icon was found. The Fab package provides a 3D mesh, material, and textures, but no verified UI-ready icon/brush. Do not assume a raw texture is visually or legally suitable as a HUD icon without inspection. A text-only `Coins: N` display is the safe functional fallback pending approved UI art.

## 7. Existing broken/disconnected logic

- The Coin actor destroys itself without crediting authoritative state.
- The legacy `How many coins` variable is disconnected and player-bound; it would not safely persist across pawn replacement or map travel.
- The HUD has no Coin subscription or display.
- Save/load serializes no Coin balance or collected IDs.
- A loaded save can restore player state while every placed Coin reappears, enabling duplicate collection after implementation unless instance persistence is added.
- There is no atomic “try collect” operation, so simultaneous/repeated overlaps are not guarded.
- The 330 `RotatingMovementComponent` instances were measured as part of the Forest CPU bottleneck. Keeping one ticking rotation component per Coin is not acceptable long-term.
- `/Game/Coin` is a separate static mesh imported from `Chapter1_Forest.fbx`; it is not the mesh used by the production Coin Blueprint and should not be adopted merely because its name matches.
- Generic ProGameMenu/other marketplace SaveGame assets are not the active Chapter-state contract and should not be used for a parallel Coin save system.

## 8. Proposed authoritative coin-state architecture

```text
BP Coin pickup
    -> USOTMPlayerBlueprintLibrary::TryCollectCoin(...)
        -> USOTMPlayerStateSubsystem (authority)
            - validates persistent ID and positive value
            - records collected ID
            - increments AvailableCoins
            - increments LifetimeCoinsCollected
            - broadcasts OnCoinsChanged / OnCoinCollected
            - marks Coin state dirty
        -> pickup disables itself and plays feedback only after success

USOTMPlayerStateSubsystem
    -> WBP_IngameUI subscribes to OnCoinsChanged
    -> existing BP_CustomSaveGameObject stores/restores Coin state
```

Rules:

- One authority: `USOTMPlayerStateSubsystem`.
- One atomic API: `TryCollectCoin(PersistentCoinId, CoinValue)` returns success/failure.
- No direct writes from Coin Blueprint or HUD.
- No HUD polling or Event Tick.
- No direct dependence on `BP_MenuSystemCharacter` variables.
- No upgrade purchase implementation in this task. A future read-only balance query/event is sufficient as a hook.

Recommended state, pending semantics approval:

- `int32 AvailableCoins = 0`
- `int32 LifetimeCoinsCollected = 0`
- `TSet<FGuid> CollectedCoinIds` at runtime, serialized as `TArray<FGuid>` if Blueprint SaveGame support requires it
- `bool bCoinStateDirty`
- `OnCoinsChanged(AvailableCoins, LifetimeCoinsCollected)`
- `OnCoinCollected(PersistentCoinId, Value)`

Both integers increment on collection. No spending/decrement behavior is implemented in this milestone.

## 9. Magical Coin pickup implementation plan

Planned exact flow:

1. Coin becomes active in the Forest.
2. It queries authoritative state for its persistent ID. If already collected, disable collision and hide/destroy without awarding.
3. On pickup-area overlap, validate that the overlapping actor is the controlled production player through the existing player-state/Blueprint-library contract—not a class-name-only cast and not any arbitrary actor.
4. Use an instance `bCollectionInProgress` guard to reject re-entry in the same frame.
5. Call `TryCollectCoin(PersistentCoinId, CoinValue)`.
6. Authority rejects invalid/duplicate IDs, non-positive values, or repeated transactions.
7. On success, authority records ID, increments totals once, broadcasts events, and marks save state dirty.
8. Only after success: immediately disable overlap/collision, hide the mesh, and stop rotation.
9. Play the existing pickup sound. Destroy the actor after feedback, or leave it hidden until unload; do not let feedback timing reopen collision.
10. Save according to the approved policy in section 12.

Explicit client requirements are collection and tracked currency. Player validation, atomicity, collision shutdown, ID checks, event broadcasts, and ordering are recommended implementation safeguards.

Performance plan inside the Coin milestone:

- Remove the per-actor `RotatingMovementComponent` runtime tick from the repaired Blueprint.
- Prefer a shared material-based visual rotation/animation if it preserves the look; otherwise use distance-gated, low-frequency animation managed without 330 independent ticks.
- Capture before/after Forest Game Thread and frame-time data before claiming this optimization succeeded.

## 10. Total coin tracking plan

The documents support two meanings but do not define their relationship:

- **Available Coins:** spendable future currency.
- **Lifetime Coins Collected:** cumulative Chapter collection progress.

Recommended implementation is to preserve both values so a later purchase cannot erase objective/progression history. During this limited milestone they remain equal because spending is out of scope.

The HUD field must be selected after clarification:

- If the label remains “Coins collected,” display `LifetimeCoinsCollected`.
- If the gameplay intent is wallet balance, display `AvailableCoins` and label it “Coins.”
- If both are required later, that is future HUD/upgrade scope and should not be added now.

Starting value: use configurable/default `0` only if the client does not provide another value. Coin value: expose `CoinValue`, default `1` provisionally.

## 11. HUD coin display plan

Modify only `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI` or add one small child Coin-counter widget used only by it.

Minimal presentation:

- one text label (`Coins` or `Coins collected`, pending approval);
- one numeric text value;
- optional icon only if an existing texture is visually approved for UI use.

Lifecycle:

1. On widget initialization/construct, obtain `USOTMPlayerStateSubsystem` through `USOTMPlayerBlueprintLibrary`.
2. Bind once to `OnCoinsChanged`.
3. Immediately read current state so the initial value is correct after map travel/load.
4. Update only the numeric text when the delegate fires.
5. Unbind safely on widget destruction if the binding type requires it.

Do not bind the text property through a per-frame UMG function, use Event Tick, redesign the HUD, or add “coins required for upgrades” now.

## 12. Save/persistence plan

### Currency balance

Increment the existing player save schema version and add to `BP_CustomSaveGameObject`:

- `SOTM_AvailableCoins`
- `SOTM_LifetimeCoinsCollected`
- `SOTM_CollectedCoinIds`

`USOTMPlayerStateSubsystem::WriteStateToSaveObject` and `ReadStateFromSaveObject` should serialize/restore these fields using the existing additive reflection bridge. Loading an older save should migrate missing Coin fields to zero/empty rather than reject an otherwise valid save.

`ResetRuntimeStateForNewGame` must clear both counts, collected IDs, and dirty state. Continue/load must restore them.

### Save timing

No authoritative timing is documented. Recommended policy:

- Atomically update runtime state on every successful pickup.
- Mark Coin state dirty immediately.
- Persist through the existing manual Save path.
- Persist on the existing checkpoint save path.
- Flush dirty state before approved production map travel/return-to-Mansion travel.
- Decide with the client whether every pickup also causes an immediate disk save.

Immediate save provides strongest anti-loss behavior but may cause I/O hitches when coins are collected rapidly. Checkpoint/manual/map-transition saving is smoother but allows rollback after a crash or death. The death/checkpoint rollback rule must be approved; it should not be invented here.

### Individual collected state

If collected pickups must remain gone after reload, save the stable ID set alongside counts. On actor initialization, a Coin whose ID is already collected hides and disables itself before it can overlap.

## 13. Persistent pickup-ID strategy

Recommended safest key:

- Add an instance-editable, read-only-at-runtime `FGuid PersistentCoinId` to the production Coin class/Blueprint.
- Assign missing IDs once in the editor with a narrowly scoped migration/validation tool.
- Store the ID on each placed actor instance, not in a runtime pointer and not derived from transform.
- Validate that every production Coin has a valid, unique ID before saving the map packages.
- Never regenerate a valid ID during normal construction, PIE, cook, duplication, or load.

Do not use:

- actor pointers;
- array indices;
- display labels such as `Coin125`;
- transforms/locations;
- transient spawn order;
- `AActor::GetActorGuid()` directly at runtime. Unreal 5.6 documents that actor GUID access is available only in editor builds.

One-time migration implications:

- The production `CH1` World Partition map has 330 external Coin actor packages. Baking unique IDs may intentionally modify many external actor `.uasset` files even though no positions change.
- The future implementation must generate an exact Git allowlist from the migration result and verify that only Coin external actors changed.
- Do not stage an entire `__ExternalActors__` directory blindly.

If the client explicitly says picked coins may respawn after load, the ID migration can be omitted. That would reduce implementation time but permit repeated collection across saves.

## 14. Map-travel and checkpoint behavior

- Runtime counts live in `USOTMPlayerStateSubsystem`, so they survive `Main Menu -> Mansion -> CH1 Forest` and future `Forest -> Mansion` travel within the same GameInstance.
- On entering a map, HUD reads the existing total and binds to future changes.
- Each loaded Coin checks its persistent ID against the authoritative collected set.
- Before an intentional travel that must preserve progression, flush dirty Coin state through the existing SaveGame bridge if the approved policy requires disk durability.
- Checkpoint saves include Coin counts and IDs.
- Respawn must not reset runtime Coin totals.
- New Game must reset Coin totals and IDs.
- Retry/Game Over behavior must follow the existing Player System save/checkpoint policy; whether post-checkpoint coins roll back is unresolved and requires client approval.

No new objective, upgrade, Timmy, ability, boss, or chest behavior should subscribe during this milestone. The delegate is merely a safe future integration hook.

## 15. Proposed file modification allowlist

This is a plan, not authorization. Exact paths must be reconfirmed immediately before implementation.

### Expected C++ files

- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerBlueprintLibrary.h`
- `Source/SOTM1/Private/SOTMPlayerBlueprintLibrary.cpp`

No new C++ Coin class is required unless Blueprint cannot safely expose the persistent-ID lifecycle and atomic API. Prefer the minimal extension above.

### Expected Blueprint/UI assets

- `Content/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI.uasset`
- Optional new small child widget, only if keeping Coin presentation isolated is safer: `Content/UI/WBP_SOTMCoinCounter.uasset`

### Expected SaveGame assets

- `Content/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject.uasset`

### Expected Coin assets

- `Content/Blueprints/Coin.uasset`
- Conditional: only the 330 Coin instance external-actor packages reported by a validated ID migration under `Content/__ExternalActors__/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1/...`
- Optional one-time editor migration script if approved: `Content/Python/SOTM_AssignPersistentCoinIds.py`

Do not modify `/Game/Coin`, the Fab art assets, Coin transforms, or non-Coin external actors unless a separately verified issue requires it.

### Expected documentation files

- `ProjectDocs/SOTM_Coin_System_Implementation_Report.md` (future implementation report)
- This plan only if implementation discoveries require an explicit amendment.

## 16. Recommended implementation order

1. Obtain clarification on terminology, displayed value, save timing, death rollback, persistent pickup absence, and whether all 330 are in scope.
2. Create a Git checkpoint and exact pre-change status snapshot.
3. Extend `USOTMPlayerStateSubsystem` with Coin state, delegates, atomic collection, reset, and backward-compatible save migration.
4. Extend `USOTMPlayerBlueprintLibrary` with only the Blueprint-facing query/collection helpers required by Coin and HUD.
5. Add SaveGame fields to `BP_CustomSaveGameObject`; test old-save migration, New Game, Save, and Load before touching map actors.
6. Repair `/Game/Blueprints/Coin`: player validation, atomic transaction, duplicate guard, initialization check, safe feedback ordering.
7. Replace the 330 independent rotation ticks with a visually equivalent lower-cost method and profile before/after.
8. If approved, assign/validate persistent IDs across only the placed production Coin actors and capture the exact changed external packages.
9. Add the minimal event-driven Coin display to production `WBP_IngameUI`.
10. Compile affected C++/Blueprints, run PIE tests, then a focused Development verification only if packaged behavior needs confirmation.
11. Produce an exact modified-file allowlist; stage/commit only with separate approval.

## 17. Test matrix

| Area | Setup/action | Expected result |
|---|---|---|
| Initial state | New Game with approved default | Authoritative and HUD totals show `0`; collected-ID set empty. |
| Single pickup | Collect one value-1 Coin | `0 -> 1` exactly once; event fires once; HUD updates immediately; feedback plays; pickup becomes non-interactive. |
| Repeat overlap | Force multiple overlap callbacks in one frame | Only one successful transaction and one increment. |
| Wrong actor | Enemy/physics actor overlaps Coin | No award, no disappearance. |
| Multiple pickups | Collect several natural placed Coins | Totals equal the sum of values; each contributes once. |
| Duplicate ID | Test two Coins with the same ID | Validator reports failure; runtime authority prevents duplicate award. |
| Invalid ID | Load a Coin with no valid persistent ID | Development error/warning; no silent persistent award. |
| HUD initial sync | Enter Forest with nonzero state | Correct total is visible without collecting a new Coin. |
| HUD event | Collect while HUD is visible | Number changes immediately without Tick polling. |
| Save/load total | Collect, save by approved policy, restart/load | Both approved total fields restore exactly. |
| Save/load pickup | Reload the collected Coin area | If persistent absence is approved, collected Coin is hidden/disabled and cannot award again. |
| Old save | Load schema-v1 save with no Coin fields | Existing player state loads; Coin state safely defaults to zero/empty. |
| New Game reset | Start New Game after a Coin save | Counts and collected IDs reset according to policy. |
| Map travel | Mansion -> Forest and future Forest -> Mansion -> Forest | Runtime/disk totals remain correct; HUD resynchronizes; collected Coin state remains correct. |
| Checkpoint | Collect before/after checkpoint, die, respawn/load | Matches the explicitly approved rollback/persistence policy. |
| Rapid collection | Collect a dense cluster | No duplicate awards, visible hitch, or repeated disk-save spike beyond approved budget. |
| Performance | Identical Forest route before/after rotation change | Captured Game Thread/frame-time evidence shows no regression and should reduce Coin tick cost. |
| Player regression | Damage, death, lives, respawn, checkpoint, Save/Load | Existing Player System behavior remains unchanged. |
| AI regression | Isabel patrol/detect/chase/attack/jump scare smoke test | No Coin change breaks the established AI flow. |
| Flow regression | Main Menu -> New Game -> Mansion -> Forest | Production travel and player possession remain functional. |
| Compile | Compile affected C++ and production Blueprints | No new C++ or production Blueprint errors. |

## 18. Acceptance criteria

The future four-requirement Coin milestone is acceptable only when:

- A production player collecting one Coin increments authoritative state exactly once.
- Non-player overlaps cannot collect Coins.
- Coin feedback and disappearance occur only after a successful transaction.
- Total tracking semantics match an explicit client decision.
- The production HUD shows the approved Coin value immediately and after initial load/travel, without polling.
- Save/load restores the approved Coin totals.
- If approved, collected placed Coins remain unavailable after load through stable IDs.
- New Game, checkpoint, death, retry, and map-travel behavior match the approved save policy.
- The 330 Coin actors do not retain the known independent rotation-tick architecture unless profiling proves an alternative unnecessary.
- No objective, upgrade, Timmy, boss, ability, chest/key, AI, or general HUD-redesign work is introduced.
- Existing Player System, Isabel AI/jump scare, Main Menu, Mansion, and Forest flow pass regression tests.
- Affected C++ and production Blueprints compile without new errors.
- Exact intentionally modified files are reported; external actor files are allowlisted individually.

## 19. Client/document clarification questions

1. Are “magical coins,” “glowing golden fragments,” and “pieces of Speed and Lightning” the same collectible and final terminology?
2. Should the HUD show current spendable balance, lifetime Chapter coins collected, or both? What exact label should it use?
3. Should the starting value be zero and should every placed pickup be worth one Coin?
4. Must a collected placed Coin remain gone after death, checkpoint reload, map travel, and full application restart/load?
5. When should Coin state write to disk: every pickup, checkpoints, manual saves, map transitions, or a combination?
6. If the player dies before reaching a checkpoint, do coins collected since the previous checkpoint remain or roll back?
7. Does New Game always clear Coin totals and collected IDs? Does Retry ever clear them?
8. Are all 330 existing CH1 Coin placements approved collectibles, or will placement/content review happen separately?
9. Is the current Bitcoin-like 3D mesh approved as the final magical Coin/fragment look?
10. Is the existing pickup sound approved, and is additional VFX required now?
11. Is a text-only HUD counter acceptable until approved Coin icon art is supplied?
12. Please provide exports of the two missing Google documents named in `Project_Audit_Instructions.pdf`.

Questions about upgrade prices, abilities, objectives, Timmy, boss progression, and purchase transactions are intentionally deferred to their own milestones.

## 20. Estimated implementation time for these four requirements only

Estimate is based on the inspected state: 330 placed Blueprint actors, existing but disconnected pickup feedback, an established GameInstance player-state/save bridge, no Coin HUD, and a potential World Partition ID migration.

| Work item | Estimated hours |
|---|---:|
| Reconfirm requirements and create checkpoint/allowlist | 1–2 |
| Authoritative Coin state, delegates, Blueprint API | 4–6 |
| Backward-compatible SaveGame schema and tests | 4–6 |
| Repair Coin pickup, duplicate safety, feedback ordering | 3–5 |
| Replace/profile per-actor rotation cost | 3–6 |
| Minimal event-driven HUD counter | 2–4 |
| Persistent-ID migration and validation for 330 actors | 4–8 |
| PIE/regression/save/travel/performance tests and report | 5–8 |
| **Total with persistent placed-Coin state** | **26–45 hours** |

Realistic planning figure: **34 hours** (approximately **11.3 working days at 3 hours/day**).

If the client explicitly permits collected Coins to respawn after load and persistent IDs are omitted, the narrower estimate is **18–28 hours**. That option is not recommended for a saved currency system because it permits repeat collection after reload.

These estimates exclude Coin placement/redesign, objectives, upgrades, prices, purchase transactions, abilities, Timmy, boss, chest/key, and general HUD redesign.

