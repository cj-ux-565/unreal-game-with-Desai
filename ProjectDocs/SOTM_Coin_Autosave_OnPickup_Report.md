# SOTM Coin Autosave-on-Pickup Report

**Project:** The Secrets of the Mansion — Chapter 1  
**Client task:** Task 4 — Final Coin Persistence Fix  
**Date:** 13 August 2026  
**Result:** Successful Coin transactions now immediately save through the existing production Continue slot. No package, stage, commit, Blueprint save, map save, lighting/environment change, or Coin presentation change was performed.

## 1. Root cause of “3 Coins -> quit -> Continue -> 0”

`USOTMPlayerStateSubsystem::TryCollectCoin` correctly added the stable Coin GUID, incremented `AvailableCoins` and `LifetimeCoinsCollected`, marked Coin state dirty, and broadcast the HUD delegates. It then returned success without calling the existing disk-save path. Consequently, Coin state was safe during the current GameInstance, map travel, and respawn, but an immediate process exit could occur before any later checkpoint/manual save wrote it to disk.

## 2. Previous save timing

Before this fix, Coin pickup itself never wrote a SaveGame. The Player System saved at existing explicit points such as checkpoint autosave and manual/debug save. A player who collected Coins and quit before one of those points could lose the current Coin total and collected-ID set on Continue.

## 3. Autosave implementation

After all authoritative validation has passed and the transaction has:

1. added `PersistentCoinId` to `CollectedCoinIds`;
2. incremented `AvailableCoins`;
3. incremented `LifetimeCoinsCollected`;
4. marked the Coin state dirty;
5. broadcast `OnCoinCollected` and `OnCoinsChanged`;

`TryCollectCoin` now calls the existing `SavePlayerState()` operation exactly once. A save failure is logged clearly while the already successful runtime transaction remains valid. Rejected transactions return before this point, so invalid collectors, invalid values, dead/Game Over players, invalid GUIDs, and duplicate GUIDs do not write a save.

No second SaveGame class, parallel Coin file, world save, asynchronous framework, or UI/overlap save logic was added.

## 4. Production save slot used

The autosave calls `SavePlayerState()`, not the named test-slot helper. `SavePlayerState()` uses `GetMenuSaveContext()` to obtain the active Menu System Pro `SaveGameManager.SaveGameObject` and its `LastLoadedSaveSlotName`. If that property is blank, the configured production fallback is `Slot 1` (`Config/DefaultGame.ini`). It then calls `UGameplayStatics::SaveGameToSlot` on that same object and slot.

The verification used `Slot 1.sav`, the production fallback/active Continue slot. Its pre-test contents were backed up under `Saved/Temp`; `Saved` is generated test data and is not part of the Git allowlist.

## 5. Continue load-path verification

Menu System Pro owns Continue through `BP_SaveGameManager`, which exposes `LoadGame`, `LoadGameFromSlot`, `LastLoadedSaveSlotName`, and `SaveGameObject`. The Player State integration resolves those exact manager fields. On gameplay-world preparation, the subsystem’s existing auto-load path calls `LoadPlayerState(false)`, which reads the manager’s active SaveGame object. Targeted cross-process/recreated-PIE verification also loaded `Slot 1` through the supported Player State load API and restored the exact autosaved total and GUID set.

Direct CH1 PIE entry starts a fresh/New Game-style test session and is not equivalent to clicking Continue. Therefore restart verification explicitly invoked the active production slot load rather than falsely treating direct CH1 PIE startup as Continue.

## 6. Exact three-Coin quit/relaunch test

A clean production-slot state `(0,0,0)` was saved, then the production player was moved through the real overlap volumes of three existing CH1 Coin actors (`Coin_C_1`, `Coin_C_10`, and `Coin_C_100`). The observed authoritative sequence was `(1,1,1)`, `(2,2,2)`, then `(3,3,3)`, proving the natural pickup path invoked the autosave. The final state was:

- Available Coins: `3`;
- Lifetime Coins: `3`;
- collected IDs: `3`;
- HUD: `COINS COLLECTED   3`.

PIE was stopped immediately without a manual save. A fresh PIE world loaded the production `Slot 1` state:

- load result: true;
- state: `(3,3,3)`;
- HUD: `COINS COLLECTED   3`;
- hidden collected Coins: `3`;
- visible uncollected Coins: `327`.

This confirms the required natural `3 -> immediate exit -> Continue/load -> 3` behavior. No presentation code, Coin value, or Coin transform was saved or changed; runtime teleport was used only to traverse the existing overlap volumes efficiently.

## 7. One-Coin immediate-quit test

From clean state, one successful existing Coin transaction returned true and immediately produced `(1,1,1)` with HUD `COINS COLLECTED   1`. The measured synchronous transaction including save took `2.233 ms`; `Slot 1.sav` timestamp and size changed immediately.

After closing/relaunching the Editor process and loading `Slot 1`:

- state restored to `(1,1,1)`;
- HUD immediately displayed `COINS COLLECTED   1`;
- one Coin was hidden;
- 329 Coins remained visible.

## 8. Rapid collection test

Starting from the restored one-Coin state, four further unique existing IDs were collected in immediate succession. Results were `[true, true, true, true]`, ending at `(5,5,5)` with HUD `COINS COLLECTED   5`.

Measured synchronous transaction/save times were:

- `1.677 ms`;
- `0.724 ms`;
- `0.544 ms`;
- `0.619 ms`;
- average `0.891 ms`, maximum `1.677 ms` for this sample.

After immediate PIE exit and fresh load, state restored to `(5,5,5)`, the HUD showed `5`, five Coins were hidden, and 325 remained visible.

## 9. Collected/uncollected Coin restoration

After the five-Coin reload, a nearby uncollected Coin was confirmed visible and collected successfully, producing `(6,6,6)` in `0.966 ms`. After immediate exit/reload:

- state restored to `(6,6,6)`;
- HUD displayed `6`;
- six Coins were hidden;
- 324 uncollected Coins remained visible/available.

Duplicate and invalid-collector attempts both returned false, left lifetime Coins at `5`, and left the production save file timestamp byte-for-byte unchanged at the filesystem timestamp level. Therefore rejected transactions caused neither currency nor an unnecessary autosave.

## 10. New Game regression

With saved Coin progress present, the existing New Game reset path produced `(0,0,0)` and HUD `COINS COLLECTED   0`. Persisting that existing reset through the production save operation succeeded. After exit/reload, `Slot 1` restored `(0,0,0)` and HUD `0`.

The autosave change does not call reset and does not weaken New Game behavior. Continue loads saved state; New Game explicitly resets it.

## 11. Autosave performance observation

The existing SaveGame write is synchronous. On the current development PC, one-Coin and rapid-pickup samples measured approximately `0.54–2.23 ms`, with the four-pickup rapid sample averaging `0.891 ms`. No visible hitch was observed during this focused PIE test. This is a small local SaveGame object, not a world/package save.

These measurements are local development-PC observations, not guarantees for slower storage. If future profiling on target hardware shows a perceptible hitch, asynchronous/coalesced persistence can be considered separately; it was not justified by current evidence and would conflict with the requested immediate one-save-per-success semantics.

## 12. Exact files modified

This autosave-fix task intentionally modified only:

- `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp`
- `ProjectDocs/SOTM_Coin_Autosave_OnPickup_Report.md` (created)

No test source change was necessary because end-to-end disk behavior requires the real Menu System Pro manager/SaveGame context and was verified in PIE instead of introducing a brittle mocked unit test.

Pre-existing dirty files from Coin Phase 1/2/final HUD work remain separate. No `.uasset`, CH1 external actor, Coin actor, Coin transform, HUD layout, pickup sound, map, lighting, environment, or camera asset was modified by this fix.

## 13. Exact Git allowlist

For this autosave-only change, stage only:

```text
Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp
ProjectDocs/SOTM_Coin_Autosave_OnPickup_Report.md
```

If the earlier Coin phases and final HUD have not yet been committed, use their separate documented allowlists rather than using `git add -A`. Do not stage `.codex_mcp_session`, `Saved`, `Intermediate`, packaged output, or unrelated assets.

## 14. Suggested commit title

`Autosave Coin progression after every successful pickup`

## 15. Suggested commit description

`Write authoritative Coin totals and collected GUIDs through the active Menu System Pro Continue slot immediately after each successful TryCollectCoin transaction. Rejected and duplicate collection attempts do not save. Verify one-, three-, five-, and six-Coin immediate-exit restoration, New Game reset, HUD restoration, and synchronous save cost in PIE.`

## 16. Client update

The final Coin persistence gap is fixed. Every successful Coin pickup now immediately saves the authoritative Coin totals and collected Coin ID through the same active save slot used by Continue. Immediate-exit tests restored 1, 3, 5, and 6 Coins correctly; collected Coins remained gone, uncollected Coins remained available, duplicate/invalid attempts did not save, and New Game still resets to zero. The Editor Development target and all affected production Blueprints compile successfully. No lighting, environment, map, Coin presentation, HUD layout, or unrelated gameplay system was changed.
