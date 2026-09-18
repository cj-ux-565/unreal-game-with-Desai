# SOTM Coin System Phase 2 — Persistence Report

Date: 2026-08-13

## Outcome

Coin Phase 2 is implemented and verified in Unreal Editor PIE. Coin totals and stable collected-Coin identities now use the existing Player State subsystem and existing production SaveGame object. Collected CH1 Coins stay unavailable after load and map travel, uncollected Coins remain collectible, New Game resets Coin progression, and schema-v1 saves load with safe zero-Coin defaults.

No package, Git stage, or Git commit was performed. Lighting and map presentation were intentionally not modified.

## 1. Phase 1 architecture reused

- `USOTMPlayerStateSubsystem` remains the only authoritative Coin owner.
- `/Game/Blueprints/Coin` remains the production pickup and still uses the Phase 1 native overlap transaction, sound, collision shutdown, hide, and destroy flow.
- `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI` remains the production HUD and still reads the subsystem and listens to `OnCoinsChanged`; no Tick or duplicate HUD currency exists.
- The existing Player System SaveGame/reflection bridge and existing save paths were extended. No second SaveGame class or currency system was created.

## 2. SaveGame fields added

The existing `/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject` was extended additively with:

- `SOTM_AvailableCoins` — integer;
- `SOTM_LifetimeCoinsCollected` — integer;
- `SOTM_CollectedCoinIds` — string containing one normalized GUID per line.

The newline-delimited string is the serialized equivalent of a GUID collection and is handled only by the Player State save bridge.

## 3. Save schema/version handling

- `USOTMPlayerStateSubsystem::CurrentSaveVersion` is now `2`.
- Schema 2 writes health/lives/checkpoint state plus both Coin totals and the sorted GUID collection.
- GUIDs are sorted before serialization, producing deterministic save data.
- Invalid GUID strings found in a save are ignored with a warning rather than crashing or creating collectible-state corruption.

## 4. Backward compatibility

Schema 1 is accepted. It reads existing Player data and intentionally defaults:

- Available Coins: `0`;
- Lifetime Coins: `0`;
- Collected IDs: empty.

PIE isolated-save result: a schema-v1 object with lives `2/3` and health `75` saved and loaded successfully; restored Player values were `2/3`, `75`, and Coin state was `(0, 0, 0)`.

## 5. Persistent Coin ID implementation

`ASOTMCoinPickup` now owns an instance-edited, SaveGame-marked `FGuid PersistentCoinId`. It is assigned in Editor data and is never generated at runtime. It is not derived from actor pointers, transforms, array indices, or spawn order.

At BeginPlay/stream-in, each Coin queries the authoritative subsystem. A collected ID disables overlap/collision and hides the actor without awarding or playing sound. Each pickup also subscribes to the existing Coin-state event so an explicit load performed after actor BeginPlay immediately refreshes actor availability. The delegate is removed safely at EndPlay.

An invalid placed ID logs an error and disables that pickup rather than permitting an unstable award.

## 6. 330-Coin migration/validation result

`Content/Python/SOTM_AssignPersistentCoinIds.py` is a CH1-only Editor migration:

- operates only on `/Game/Blueprints/Coin.Coin_C` instances in production CH1;
- preserves valid unique IDs;
- assigns only missing/duplicate IDs;
- validates every actor transform before saving;
- refuses to run if unrelated map packages are already dirty;
- saves only an exact dirty-package set equal to its modified-Coin allowlist;
- refuses the save if the dirty set differs;
- does not change components, meshes, materials, visuals, or non-Coin actors.

Migration result: `330` Coins received stable IDs. Final PIE validation: count `330`, valid `330`, unique `330`, duplicate count `0`, dirty map packages `0`. An idempotence rerun reported `total=330 modified=0 prior_duplicates=0`.

No Coin transforms were changed: the migration captured and compared each actor transform around its ID edit and aborted on any mismatch.

## 7. Duplicate ID validation

Migration validation rejects duplicate final IDs. Runtime collection is atomic: the subsystem checks `CollectedCoinIds` before adding the ID and totals. The same ID returned `false` when attempted twice before save and again after load. It never changed totals on rejection.

## 8. Exact external actor packages modified

Exactly `330` CH1 Coin external-actor packages were intentionally modified. The complete package-by-package list is maintained in:

`ProjectDocs/SOTM_Coin_Phase2_ExternalActor_Allowlist.txt`

The allowlist contains exactly 330 lines. Do not stage the full `__ExternalActors__` directory; stage only those listed files. The CH1 `.umap` was not modified for this migration.

## 9. Collection persistence behaviour

The transaction order is:

1. validate GUID, value, living bound production player, and duplicate state;
2. add GUID to the authoritative collected set;
3. increment Available and Lifetime totals;
4. mark Coin state dirty;
5. broadcast existing Coin events;
6. disable the pickup, play its existing sound, and destroy it.

One natural CH1 overlap was reverified after the persistence changes. Teleporting the production pawn onto existing `Coin_C_1` caused the real overlap path to change authoritative state `0 -> 1`, with Lifetime `1` and collected-ID count `1`.

## 10. Save timing used

Coin state changes immediately in memory and is marked dirty. It is written through existing approved Player System save paths:

- manual/current save;
- checkpoint save where the existing checkpoint path saves;
- death/respawn save where existing Player settings invoke it;
- existing production transition paths that already save;
- explicit-slot API used only for isolated verification.

No save-on-Tick or new per-pickup disk-write system was introduced.

## 11. Save/load test

An isolated slot named `SOTM_CoinPhase2_Automation` was used without touching client save slots.

- Five distinct valid IDs collected: results were five `true` transactions.
- Saved at Available `5`, Lifetime `5`, collected count `5`.
- Runtime reset produced `(0, 0, 0)`.
- Load restored `(5, 5, 5)`.
- The same five actor instances were hidden and collision-disabled immediately after explicit load.
- A sixth uncollected ID remained active, collected once, and saved at `(6, 6, 6)`.
- Reloaded HUD text was directly inspected as `Coins: 6`.

## 12. Uncollected Coin test

After restoring the five-Coin save, Coin six had:

- collected flag: `false`;
- hidden: `false`;
- actor collision: `true`.

Its transaction succeeded once and changed totals `5 -> 6`. The following duplicate attempt was rejected.

## 13. Respawn test

With `(6, 6, 6)` loaded, a temporary isolated CH1 checkpoint was registered through the existing Player System API. Fatal damage flowed through `USOTMPlayerBlueprintLibrary`/the Vital Component:

- one life was consumed (`5 -> 4`);
- death state activated;
- normal checkpoint respawn completed;
- health restored to `100`;
- dead state cleared;
- Coin state remained `(6, 6, 6)`.

No Player System code was changed for this test.

## 14. Map-travel test

Runtime PIE travel used the production map assets:

`CH1 -> /Game/Mansion_GameStart -> CH1`

Results:

- before leaving CH1: `(6, 6, 6)`;
- in Mansion: `(6, 6, 6)`;
- after returning to CH1: `(6, 6, 6)`;
- all first six collected Coin actors were hidden and collision-disabled after return.

This confirms GameInstance-subsystem state survives normal travel and streamed/recreated Coins query the restored authoritative state.

## 15. New Game reset test

The existing `ResetRuntimeStateForNewGame` production reset path produced:

- Available `0`;
- Lifetime `0`;
- collected count `0`;
- the six formerly collected IDs all queried as uncollected.

After reloading CH1 within the same New Game run, the first six actors were visible and collision-enabled. No save files were deleted to simulate New Game.

## 16. HUD restoration test

The existing event-driven HUD was not redesigned. After loading the six-Coin save, direct inspection of the live `SOTM_CoinCounterText` found:

- authoritative Available Coins: `6`;
- rendered text: `Coins: 6`.

No additional pickup was required to refresh it.

## 17. Old-save compatibility

An isolated slot named `SOTM_CoinPhase2_OldSchema` was created with schema version 1 and existing Player data but no Coin progression. It loaded without crash, retained Player lives/health, and supplied safe Coin defaults `(0, 0, 0)`.

## 18. Lighting and visual restriction confirmation

Lighting intentionally not modified in Coin Phase 2.

No Directional Light, Skylight, PostProcessVolume, exposure, bloom, fog, environment light, camera, terrain, PCG, foliage, Coin transform, Coin visual, or HUD layout/presentation setting was intentionally modified. No map was saved for visual reasons.

## 19. Exact files/assets intentionally modified

Source and tools:

- `Source/SOTM1/Public/Coin/SOTMCoinPickup.h`
- `Source/SOTM1/Private/Coin/SOTMCoinPickup.cpp`
- `Source/SOTM1/Public/SOTMPlayerStateSubsystem.h`
- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `Source/SOTM1/Public/SOTMPlayerBlueprintLibrary.h`
- `Source/SOTM1/Private/SOTMPlayerBlueprintLibrary.cpp`
- `Source/SOTM1/Private/Tests/SOTMPlayerFoundationTests.cpp`
- `Content/Python/SOTM_AssignPersistentCoinIds.py`

Assets and documentation:

- `Content/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject.uasset`
- the 330 exact external actors in `ProjectDocs/SOTM_Coin_Phase2_ExternalActor_Allowlist.txt`
- `ProjectDocs/SOTM_Coin_Phase2_ExternalActor_Allowlist.txt`
- `ProjectDocs/SOTM_Coin_System_Phase2_Persistence_Report.md`

The runtime `.codex_mcp_session`, `Saved`, `Intermediate`, build products, logs, and isolated test saves are generated/local data and are not commit candidates.

## 20. Exact Git allowlist

Commit only the eight source/tool paths, SaveGame asset, two ProjectDocs files, and all 330 paths enumerated in `SOTM_Coin_Phase2_ExternalActor_Allowlist.txt`. Do not use `git add Content/__ExternalActors__` broadly.

Verification/build status:

- `SOTM1Editor Win64 Development`: succeeded after final C++ change.
- `/Game/Blueprints/Coin`: compiled successfully.
- `/Game/MenuSystemPro/ExampleContent/Common/UI/WBP_IngameUI`: compiled successfully.
- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject`: compiled successfully.
- No new Coin/HUD/SaveGame compile error was observed.
- The existing `SOTM.PlayerFoundation` automation command was queued, but Unreal's automation controller remained in its interactive-frame-rate gate because the CH1 Editor viewport reported 3 FPS; therefore no automation-suite pass is claimed. The focused PIE assertions above completed independently.

Unrelated pre-existing runtime warnings observed during travel included the missing developer-machine Isabel jump-scare media path and legacy `BP_AI` Blackboard Accessed-None messages. They were not caused by or modified in Coin Phase 2.

## 21. Suggested commit title

`feat(coin): persist CH1 totals and collected pickup identities`

## 22. Suggested commit description

`Extend the existing Player State SaveGame bridge with Coin totals and stable collected GUIDs. Assign and validate unique IDs on all 330 production CH1 Coin instances, disable restored pickups, preserve uncollected pickups, support schema-v1 saves, and verify save/load, HUD restoration, respawn, map travel, New Game reset, and duplicate protection in PIE.`

## 23. Short client update

Coin System Phase 2 is complete in Editor verification. Coin totals and the identity of each collected CH1 Coin now save and restore through the existing Player System. Collected Coins stay gone after reload, respawn, and Mansion/Forest travel; uncollected Coins remain available; the HUD restores immediately; New Game resets progression; and older saves load safely. All 330 production Coin IDs were validated as unique and stable, with no Coin transforms, lighting, camera, or environment presentation changed.
