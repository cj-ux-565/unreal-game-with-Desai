# The Secrets of the Mansion – Chapter 1
## Unreal Engine 5.6 Technical Audit and Recovery Plan

**Audit date:** 28 July 2026  
**Scope:** Investigation and planning only  
**Project:** `SOTM1.uproject`  
**Audit method:** Read-only Unreal Editor/MCP inspection, Unreal asset-registry inspection, direct configuration/source/log inspection, PDF extraction plus rendered-page review, and sampled video-frame review.

No Unreal asset was created, edited, renamed, moved, deleted, compiled, or saved during this audit. No gameplay implementation or C++ change was made.

The status terms used below are deliberately strict:

- **Asset exists:** a relevant package is present.
- **Logic exists:** executable Blueprint/C++ logic is present.
- **Logic is connected:** the logic is referenced by the intended map/system.
- **Appears functional:** static inspection found a complete path, but this is still not a substitute for a controlled PIE test.
- **Broken or incomplete:** the path is missing required steps, contains known errors, or conflicts with the requirement.
- **Unable to verify:** proving the claim would require changing/opening levels, compiling, playing, packaging, unavailable files, or another action prohibited during this audit.

---

# 1. Files successfully reviewed

## Supplied requirements

- `ProjectDocs/CHAPTER 1 — Voice Actor Lines.pdf` — all 2 pages extracted and visually reviewed.
- `ProjectDocs/Mansions Quests_ Chapter 1.pdf` — all 8 pages extracted and visually reviewed.
- `ProjectDocs/Project_Audit_Instructions.pdf` — all 1 page extracted and visually reviewed.
- `ProjectDocs/Client_Requirements.txt` — reviewed completely.
- `ProjectDocs/VID-20260728-WA0002.mp4` — 74 seconds, 640×360. Frames sampled throughout the full duration and reviewed as contact sheets.

## Unreal project evidence

- `SOTM1.uproject`
- All files under `Config/`, including startup-map, rendering, input, gameplay-tag, and packaging settings.
- All C++ files under `Source/`.
- Complete asset-registry inventory: approximately 17,261 `.uasset` files and 73 `.umap` files.
- Relevant gameplay Blueprint structure, components, variables, event/function graphs, input assets, widgets, AI assets, save assets, projectiles, sequences, map dependencies, external actors, and current editor logs.
- World Partition/external-actor evidence for the mansion maps and forest-related content.
- Existing compiler/load/reference errors already recorded in the project logs. No new compile was started.

---

# 2. Files or video I could not inspect

## Missing reference documents

`Project_Audit_Instructions.pdf` asks for two additional Google documents to be downloaded as:

- `GoogleDoc_1.pdf`
- `GoogleDoc_2.pdf`

Neither file exists in `ProjectDocs/`. Direct access to the referenced Google Docs was unavailable, so their contents were **not reviewed**:

- `1qwO9QvcpGedNP_8HkD-P-S-c_ZN8OYF_-VlkcHLabPY`
- `13bfDzk73wtLKXPbFH6lprti6zUHmUh7X5CwecEp2_yc`

The client should supply exported PDFs before scope or dialogue is treated as final.

## Video limitation

The full video was inspected visually through sampled frames. It shows mansion gameplay, Timmy on a counter/table, the mage, an Isabel/cousin chase, an icy/foggy forest, bright pink/purple magical effects, the game logo, and a “COMING SOON 2026” card. Visible subtitles include portions of Timmy’s introduction and “She is powerful, cruel.”

The local browser could not attach to the MP4, so the fallback media reader was used. I could not reliably transcribe or verify the complete audio track. The video is therefore valid visual-reference evidence, but not a verified dialogue/audio specification.

## Runtime limitations

Because this audit forbids compiling, saving, and implementation, the following could not be proven:

- Blueprint runtime behavior in PIE.
- Per-map Level Blueprint execution for every candidate map without switching the editor’s open world.
- Navigation reachability, collision behavior, AI combat timing, and packaged performance.
- Blueprint compile health beyond errors already present in saved editor logs.
- A clean cook/package result.
- The exact runtime destination of every marketplace menu button.

Any statement marked “appears functional” is based on connected static logic, not a completed gameplay test.

---

# 3. Requirements summary

## Intended story

Chapter 1 is “Lightning in the Dark.” The player is a mage whose Speed and Lightning powers were stolen by Isabel/Isabella, a possessed doll. Timmy Bottom Smith, a magical teddy bear, introduces the mystery in the mansion and explains that the forest is the only place where the powers can be recovered.

Isabel appears, removes or confirms the loss of the player’s powers, and defeats the player in a scripted encounter. The player is dragged into Isabel’s foggy Forest Domain. Timmy then guides the player through objectives while cousin dolls hunt the player.

## Intended gameplay loop

1. Receive an objective from Timmy.
2. Explore the forest while avoiding or escaping cousin dolls.
3. Collect magical coins/fragments.
4. Spend the currency at upgrade stations.
5. Unlock **Speed** first; it uses stamina/cooldown.
6. Unlock **Lightning Throw** second; it stuns cousins rather than killing them.
7. Locate the hidden chest, obtain the key, and open the boss gate.
8. Fight Isabel using movement and recovered abilities.
9. Watch the ending sequence and proceed into the Chapter 2 setup.

The latest client message changes/clarifies the currency model: coins collected during Chapter 1 are also used through Timmy to unlock the Chapter 2 ability.

## Player progression

- Start with no Speed and no Lightning Throw.
- Collect enough currency/fragments to restore Speed.
- Continue collecting to reveal/reach a hidden chest and recover Lightning Throw.
- Obtain the boss key and enter the arena.
- Defeat Isabel.
- Preserve collected coins through the return-to-mansion transition.
- Approach Timmy at the next checkpoint.
- Use coins to unlock **Purple Burst**, the Chapter 2 ability, on `BP_menuSystemCharacter`.

## Intended Chapter 1 ending

Isabel reaches low health and calls for her mother. A giant spider/spider mother breaks into the scene and pulls Isabel away, leaving her fate ambiguous. The portal back to the mansion opens, the ending cutscene plays, skippable credits follow, and the player returns to the mansion. Approaching Timmy establishes the next checkpoint and unlock path for Purple Burst.

There is a narrative ambiguity requiring client confirmation: the quest PDF ends on the spider reveal and fade to black, while the latest requirement adds portal activation, credits, mansion return, and an immediately available Chapter 2 ability purchase.

---

# 4. Current project architecture

## Executive architecture finding

The project is not currently one coherent Chapter 1 implementation. It is a large assembly of marketplace/demo packs with a small amount of custom Blueprint work placed on top. There is no authoritative gameplay framework for Chapter state, objectives, currency, lives, checkpoints, ability ownership, boss state, or save persistence.

The most important architectural facts are:

- Gameplay code is almost entirely Blueprint-based.
- The C++ module contains no production gameplay system.
- `BP_menuSystemCharacter` has many unrelated marketplace features and several Chapter 1 variables, but it has become a “god Blueprint.”
- Menu, save, inventory, weapon, speed, ability, and UI systems come from different packs and are not consistently integrated.
- World Partition is used for major mansion content.
- Multiple candidate maps and redirectors obscure the real boot/gameplay route.

## C++ source

Relevant paths:

- `Source/SOTM1/SOTM1.cpp`
- `Source/SOTM1/SOTM1.h`
- `Source/SOTM1/MCP_Test.cpp`
- `Source/SOTM1/MCP_Test.h`

`AMCP_Test` is an empty test actor with enabled Tick and empty `BeginPlay`/`Tick`. There is no C++ quest manager, player-state model, save model, AI system, boss system, ability system, or developer-test framework. **Status: asset/code exists, production gameplay logic missing.**

## Startup and global framework

`Config/DefaultEngine.ini` currently specifies:

- Editor startup: `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/Demo_Menu`
- Game default: `/Game/DarkForest/Maps/Demo_Menu`
- Server default: `/Game/ProMainMenuV3/Levels/LVL_MainMenu`
- Global GameMode: `/Game/MenuSystemPro/Blueprints/GameFramework/BP_PlayLevelGameMode`
- GameInstance: `/Game/MenuSystemPro/Blueprints/GameFramework/BP_MenuSystemGameInstance`

The game-default redirector chain resolves to the Menu System Pro example level:

`/Game/DarkForest/Maps/Demo_Menu` → `/Game/MenuSystemPro/.../Levels/Demo_Menu` → `/Game/MenuSystemPro/.../Levels/Menu`

This is not a stable Chapter 1 boot path.

`BP_PlayLevelGameMode` and `BP_MenuLevelGameMode` contain only empty lifecycle events. `BP_MenuSystemGameInstance` provides generic marketplace menu/settings/save behavior, not Chapter 1 progression.

## Player framework

Primary character:

- `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter`

The character has movement, camera, sprint/stamina logic, a particle-based Speed state, an Ability System Component, inventory component, flashlight, sword/weapon sample elements, HUD creation, pause, skill-tree opening, and the ability to spawn `/Game/Abilitys/BP_LightingThrow`.

Notable variables include `How many coins`, `Speed`, `FreeToThrowLightnings`, `CurrentHealth`, `Currentmana`, `isdead?`, stamina, HP, and skill-widget references. Their presence does not make them functional. Static graph inspection found:

- Sprint/stamina: meaningful logic exists.
- Lightning projectile spawn: logic exists.
- Ability ownership/unlock: not connected end to end.
- Health/damage/death/lives/respawn: variables exist, required state machine does not.
- Five-life system: missing.
- Checkpoint handling: missing.
- Chapter state persistence: missing.
- Ability System Component: present but no verified Chapter 1 grant/activation path.

Enhanced Input assets exist, including move, look, jump, pause, sprint, interact, and skill-tree actions. The inspected mappings did not establish a complete unlocked-Lightning/Purple Burst input path.

## Gameplay Ability System

- `/Game/Abilitys/GA_Spell` commits a cooldown, plays a montage, and spawns `/Game/Abilitys/BP_ThrowFire`.
- `/Game/Abilitys/GE_Fireball` and `/Game/Abilitys/BP_Fireball` are effectively shells.
- Project gameplay tags contain only `Ability1` and `ability cooldown 1`.

This is a generic fire-spell sample, not a finished Speed/Lightning/Purple Burst architecture. The GAS component exists, but the required abilities are not represented as a coherent granted, costed, saved, and UI-reported set.

## AI architecture

Custom enemy assets:

- `/Game/AI/BP_AI`
- `/Game/AI/BP_AI_Controller`
- `/Game/AI/BT_AI`
- `/Game/AI/BD_AI`
- `/Game/AI/BTTask_ChasePlayer`
- `/Game/AI/BTTask_RoamAround`
- `/Game/AI/BTTask_AttackPlayer`

`BP_AI` uses **Pawn Sensing**, not AI Perception. It sets a blackboard “seeing player” value, traces for attacks, applies damage, plays attack/jump-scare media, disables input, and opens levels/screens. The controller starts the Behavior Tree. The tree contains roam and chase/attack branches.

There is only one Behavior Tree and one Blackboard in the asset registry and no EQS asset. No dedicated Isabel boss controller/tree was found. The current AI is a prototype cousin/enemy path and is not a complete boss system. Runtime correctness of its decorators and navigation could not be verified.

## Currency, objectives, upgrades, and progression

- `/Game/Blueprints/Coin` rotates, responds to overlap, plays a sound, and destroys itself.
- It does **not** increment the character’s coin total, update a shared currency service, notify objectives, persist state, or fund an upgrade.
- Approximately 330 `Coin_C` external-actor instances exist, showing that pickups were placed before the economy was implemented.
- `/Game/Skill_Tree/WB_SkillTree` contains only lifecycle events.
- `/Game/Skill_Tree/WB_Skill` contains button audio/hover behavior but no purchase/unlock transaction.
- No functional objective/quest manager or upgrade-station gameplay asset was found.

These systems are asset shells or disconnected prototypes.

## Save, checkpoint, death, and respawn

Generic menu-pack save assets exist:

- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_SaveGameManager`
- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject`
- `/Game/MenuSystemPro/Blueprints/SaveGame/BP_AutoSaveTrigger`

The custom save object stores general metadata such as area name/text, save count, play time, and timestamp. It does not store Chapter 1 coins, unlocked abilities, lives, checkpoint ID, boss state, opened chest/gate, or completed objectives.

`/Game/d_oll/DEATH_ANIME/BP_DeathScreen` has Retry/Main Menu behavior. There is no verified five-life loop, distinct game-over state, safe respawn, or checkpoint restore.

## UI

Relevant assets include:

- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Widgets/Ingame/WBP_IngameUI`
- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Widgets/Ingame/WBP_HealthBar`
- `/Game/UI/PauseMenu`
- `/Game/d_oll/DEATH_ANIME/BP_DeathScreen`
- `/Game/d_oll/DEATH_ANIME/WBP_Jumpscare`
- `/Game/Skill_Tree/WB_SkillTree`
- `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Menus/Special/WBP_Credits`

The in-game HUD can update marketplace health/ammo-style widgets, and the health bar has binding functions for health, stamina, and cooldown. No connected side objective panel with objective text, coins, upgrade requirement, and boss progress was found. The pause menu is minimal. The credits UI is generic marketplace content.

## Environment, PCG, foliage, and navigation

- Two registered PCG graphs were found: `/Game/Levels/PCG/PCG_Forest` and `/Game/Levels/PCG/PCG_Forest_ground`.
- The mansion worlds contain large numbers of static mesh actors plus World Partition HLODs.
- Mansion external actors include a `NavMeshBoundsVolume` and Recast navigation data.
- The currently open menu example level had no `NavMeshBoundsVolume`.
- No EQS asset was found.

The environment art base exists, but navigation validity, cousin coverage, boss-arena pathing, foliage collision, streaming boundaries, and packaged performance remain unverified.

## Audio, VFX, cinematics, and media

The registry contains hundreds of generic sounds and multiple Niagara/marketplace VFX packages. Relevant custom-named audio is limited mainly to jump-scare/objective SFX; no imported set matching the supplied Timmy/Isabel/cousin voice script was found.

Five Level Sequences were found, only a subset relevant to the game:

- `/Game/AI/Jumpscare`
- `/Game/JumpscareSequnce1`
- `/Game/Cinematics/Takes/2026-07-03/Scene_1_01`
- `/Game/Cinematics/Takes/2026-07-03/Scene_1_01_Subscenes/Isabella_Jumpscare2_Scene_1_01`
- one unrelated marketplace cloth/scaffolding sequence

This is not the required mansion intro, Isabel appearance, forest drag, unlock moments, boss entrance, spider ending, credits transition, and mansion-return suite.

## Asset hygiene and known errors

- 98 `ObjectRedirector` assets remain.
- `Content/SuperPowers.umap` and `Content/SuperPowers/SuperPowers.umap` are reported as invalid/corrupt package files.
- `/Game/DarkForest/Maps/Demo` and `/Game/DarkForest/Maps/Chapter1` log missing redirected built-data imports.
- `BP_WEAPON` logs compile errors, including missing pins and missing function `Equip Weapon` on `BP_Speedster_C`.
- `BP_NvidiaReflexApply` logs a missing `/Script/StreamlineBlueprint` import.
- UDP Messaging is configured with stale network data and logs a failed bind.
- An obsolete iOS configuration import is logged.
- No default Sound Concurrency object is configured.

## Packaging and performance posture

Packaging is set to Development, not Distribution. `bCookAll=False`, `bCookMapsOnly=False`, no intentional Chapter 1 map list was established, and unrelated plugin directories are always cooked. The project includes many large demo/showcase maps and packs.

Rendering simultaneously enables costly features/settings including ray tracing, Lumen-related features, Nanite/Virtual Shadow Maps, a large texture pool, and Forward Shading. This mix must be revalidated for the target hardware. The project contains thousands of placed static-mesh actors, World Partition HLODs, dense foliage content, and very large sample maps. A controlled performance budget does not yet exist.

---

# 5. Map and level walkthrough

| Level | Asset path | Intended/observed purpose | GameMode | Important contents and systems | Current playability / major problems |
|---|---|---|---|---|---|
| Menu example (actual configured launch target) | `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/Menu` | Marketplace Menu System Pro showcase | Global default appears to be `BP_PlayLevelGameMode`; per-world override not proven | Menu actors, menu camera, two PlayerStarts, many example props | Opens the wrong architectural entry point. No verified Chapter 1 route. Current editor world is this example map. |
| Demo_Menu redirectors | `/Game/DarkForest/Maps/Demo_Menu`, `/Game/MenuSystemPro/.../Levels/Demo_Menu`, `/Game/Demo_Menu` | Redirector chain | Inherits global | Redirects to the map above | Fragile and misleading startup configuration. |
| Main_Menu_Map | `/Game/Main_Menu_Map` | Candidate custom main menu | Unable to verify | Small standalone world; exact button routing not proven | Not the configured `GameDefaultMap`. |
| Chapter1_Level | `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/Chapter1_Level` | Large candidate Chapter 1/menu-derived world | Unable to verify; global default is fallback | Dependencies include marketplace menu/gameplay content | Large duplicate/candidate map; connection to real launch path not established. |
| Mansion | `/Game/Mansion` | Primary mansion environment | Unable to verify override; global default is fallback | World Partition; about 1,884 static-mesh actors, PlayerStart, menu-system actor, NavMesh bounds/data, triggers, `BP_ForestPortal`, HorrorBear skeletal actor | Environment exists. Timmy interaction, intro flow, Isabel scripted loss, objectives, and portal behavior are not connected. |
| Mansion_GameStart | `/Game/Mansion_GameStart` | Candidate playable mansion start | Unable to verify override; global default is fallback | Similar mansion duplicate; PlayerStart, menu actor, navigation, triggers, `BP_ForestPortal`, one `BP_AI` | Has more gameplay-shaped placement than `Mansion`, but no verified start route or coherent scripted intro. Duplicate-world maintenance risk. |
| Chapter1_Forest | `/Game/Levels/Chapter1_Forest` | Candidate production forest | Unable to verify | PCG/forest content and external-actor ecosystem; coin/enemy placement exists elsewhere in the forest world data | Environment and placements exist, but objectives, currency, unlocks, chest/key, gate, boss entrance, and ending chain are not proven. |
| Forest_Prototype | `/Game/Levels/Forest_Prototype` | Forest prototype/test composition | Unable to verify | Small prototype world | Useful as a future controlled test bed, but no explicit developer-test tooling was found. Not a proven production level. |
| DarkForest Demo/Chapter1 | `/Game/DarkForest/Maps/Demo`, `/Game/DarkForest/Maps/Chapter1` | Marketplace/candidate dark-forest maps | Unable to verify | DarkForest pack content | Logged missing built-data imports; `Chapter1` redirects/delegates into this damaged chain. |
| MenuSystemPro Forest/CH1 maps | `/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/Forest`, related `CH1` content | Menu-pack-derived candidates | Unable to verify | Marketplace dependencies | Duplicated naming obscures ownership. Not proven as the authoritative forest. |

### Player starts

PlayerStart actors exist in the menu and mansion candidates. Their exact spawn transform and whether the active GameMode possesses `BP_menuSystemCharacter` in each target map could not be runtime-verified. A single authoritative start map and spawn contract must be established before gameplay work.

### Level Blueprint and sequencer status

No dependable end-to-end Level Blueprint chain was found for mansion → forest → boss → ending → mansion. The custom sequence inventory is limited mainly to jump-scare material. Because opening/saving every candidate level was prohibited, non-current Level Blueprint internals are marked **unable to verify** rather than assumed empty.

### Clearly unrelated or sample maps

The project contains many maps that should not be treated as Chapter 1 implementation evidence:

- AI overview/demo maps
- Dash VFX demo content
- Forest pack Overview/Showcase maps
- Forestglade Overview/Showcase maps
- ForestSet Overview/Showcase maps
- Free Furniture samples
- HorrorBear demo map
- Iceland environment example maps
- LazyDev demo/starter maps
- Mage presentation map
- Magic explosion demo map
- Military weapon overview map
- PN Grass demo maps
- ProGameMenu and ProMainMenuV3 sample maps
- Spider asset demo map
- SuperPowers demo/starter maps
- ThirdPerson template map
- Victorian Hotel demo/overview maps
- Video/media example scene maps

These should eventually be excluded from the production cook or removed only after dependency and license review. They must not be deleted during initial recovery.

---

# 6. Actual current gameplay flow

The only flow supported by current configuration and static evidence is:

1. The executable requests `/Game/DarkForest/Maps/Demo_Menu`.
2. Redirectors resolve that request into the Menu System Pro example `Menu` map.
3. Generic marketplace menu/save/settings actors become available.
4. A dependable route from that menu into one authoritative Chapter 1 map was not established.

The intended gameplay route is therefore **not currently connected as a complete playable chapter**.

System-by-system actual flow:

- **Main menu:** generic menu assets exist; Chapter 1 destination is unverified.
- **Mansion:** two major candidate worlds exist; environment, PlayerStart, navigation, triggers, portal actor, and in one version one enemy exist.
- **Timmy introduction:** HorrorBear art/animation exists, but no interactive Timmy companion/controller/objective-giver was found.
- **Scripted Isabel loss:** jump-scare/enemy logic exists, but a complete mansion cutscene and scripted transfer to forest was not found.
- **Forest:** extensive environment/PCG/coin/enemy placement exists.
- **Objectives:** no authoritative quest state advances the player.
- **Coins:** pickups can disappear and play sound but do not fund progression.
- **Speed unlock:** sprint/stamina implementation exists on the player; no connected coin/upgrade/objective unlock transaction.
- **Lightning unlock:** projectile logic exists; no proven unlock/input/save path, and it damages instead of implementing the required cousin stun behavior.
- **Chest/key:** chest/key art exists; no functional inventory/gate transaction was found.
- **Boss entrance:** no connected key-gate-boss-start chain.
- **Isabel fight:** no dedicated boss AI, boss health, attacks, phases, projectiles, or victory state.
- **Ending/credits:** generic credits and jump-scare sequences exist; the required spider ending and skippable chapter-credit flow do not.
- **Return to mansion:** not connected.
- **Chapter 2 setup:** no Timmy checkpoint, saved coin balance, or Purple Burst purchase/grant path.

---

# 7. Requirement-versus-project gap analysis

| System | Asset exists | Logic exists | Connected | Appears functional | Broken/incomplete / unable to verify |
|---|---:|---:|---:|---:|---|
| Forest environment | Yes | PCG/environment logic exists | Partly | Unable to verify | Gameplay path and performance unverified |
| Cousin AI | Yes | Yes | Partly placed | Unable to verify | Prototype sensing/tree; known request says AI is broken |
| Isabel jump scare | Yes | Yes | Partly | Unable to verify | Media/sequence variants and level connection unclear |
| Player movement/sprint | Yes | Yes | Yes | Likely | Needs PIE; mixed marketplace code |
| Health/damage | Variables/UI exist | Fragmented | Partly | No | No authoritative health/death contract |
| Five lives/respawn | No coherent system | No | No | No | Missing |
| Coins | Yes; ~330 placements | Pickup destroy/sound only | Placement yes | Pickup effect only | Currency, UI, save, spend missing |
| Speed unlock | Yes | Sprint logic exists | No progression link | No | Missing transaction/persistence |
| Lightning Throw | Yes | Projectile/damage exists | Spawn path partly exists | Unable to verify | Unlock/input unclear; damage conflicts with stun requirement |
| Purple Burst | No matching gameplay ability | No | No | No | Missing |
| Skill tree | Widget shells | Button/audio only | Player can open UI | No | Purchases/unlocks missing |
| Upgrade stations | No functional asset found | No | No | No | Missing |
| Objectives | SFX/text fragments only | No manager | No | No | Missing |
| HUD side panel | Generic HUD exists | Health/stamina bindings | Partly | Unable to verify | Objective/coins/boss data absent |
| Timmy companion | Mesh/animations/textures exist | No companion logic | Skeletal actor placed in mansion | No | Missing interaction, dialogue, hints, progression |
| Chest/key/gate | Art assets exist | No complete transaction | No | No | Missing |
| Pause menu | Yes | Basic open-level/remove-parent | Partly | Unable to verify | Needs polish and correct resume/settings flow |
| Death screen | Yes | Retry/main-menu logic | Enemy references it | Unable to verify | No lives/game-over/checkpoint integration |
| Save/checkpoint | Generic menu save assets | Generic metadata save | Not Chapter-connected | No | Chapter state fields missing |
| Boss fight | Isabel/cousin art exists | No dedicated boss system | No | No | Missing |
| Portal | Yes | Only event shells | Placed in mansion | No | Transition/victory conditions missing |
| Cutscenes | A few jump-scare sequences | Partial | Partial | Unable to verify | Most required sequences missing |
| Credits | Generic marketplace widget/spawner | Generic activation/deactivation | No chapter link | Unable to verify | Skippability and return path not established |
| Voice | Script exists | N/A | No matching VO set found | No | Recording/import/integration missing |
| Packaging | Settings exist | Development setup | Project-wide | No clean package evidence | Wrong start map, corrupt references, content bloat |

---

# 8. Detailed analysis of all 14 tasks

## Task 1 — Forest

**Status:** Partially complete — **45%**

- **Exists:** `/Game/Levels/Chapter1_Forest`, `/Game/Levels/Forest_Prototype`, `/Game/Levels/PCG/PCG_Forest`, `/Game/Levels/PCG/PCG_Forest_ground`, multiple forest marketplace packs, foliage and World Partition content.
- **Maps:** `Chapter1_Forest` is the strongest production candidate; `Forest_Prototype` is the strongest small test candidate.
- **Working:** environmental art, PCG assets, foliage content, placed coins/enemies.
- **Disconnected/broken:** no authoritative entry/exit, no objective routing, no verified progression order, no boss-gate path, no tested collision/nav coverage, and duplicate forest maps.
- **Must be created/repaired:** production-map selection, spawn/streaming contract, objective zones, upgrade/chest/gate/boss route, navigation/collision validation, performance pass.
- **Dependencies:** Player, AI, coins, objectives, abilities, chest/key, boss.
- **Risks:** heavy foliage/static-mesh counts, mixed rendering features, World Partition/HLOD complexity, sample-map contamination.
- **Client questions:** Which forest layout is approved? What is the target playtime and hardware frame-rate target?

## Task 2 — Isabel AI and Jump Scare

**Status:** Broken — **35%**

- **Blueprints/AI:** `/Game/AI/BP_AI`, `BP_AI_Controller`, `BT_AI`, `BD_AI`, three custom BT tasks.
- **Sequences/widgets/media:** `/Game/AI/Jumpscare`, `/Game/JumpscareSequnce1`, `/Game/d_oll/DEATH_ANIME/WBP_Jumpscare`, jump-scare media assets.
- **Working:** detection event, blackboard update, chase/attack tasks, damage trace, animations/SFX, jump-scare UI/media hooks exist.
- **Disconnected/broken:** uses Pawn Sensing rather than requested AI Perception; only one generic AI tree; decorator/runtime correctness unverified; no EQS; no robust last-known-position/search/state recovery; jump-scare variants are duplicated; known client instruction explicitly says existing AI is broken.
- **Must be created/repaired:** cousin state machine, AI Perception, navigation validation, attack cooldown/range contract, damage/death interface, original cinematic jump-scare trigger, safe player lock/unlock, test harness.
- **Dependencies:** Player health/lives, navigation, animation, UI, checkpoints.
- **Risks:** repairing a single large enemy Blueprint while preserving animations; confusion between cousin and boss Isabel.
- **Client questions:** Is the current CruelDoll look approved for cousins and main Isabel? What behavior differentiates crawl/twitch/sprint/hover variants?

## Task 3 — Player System

**Status:** Partially complete — **45%**

- **Blueprint:** `/Game/MenuSystemPro/Blueprints/Player/BP_MenuSystemCharacter`.
- **Input:** CharacterOnFoot Enhanced Input context and move/look/jump/pause/sprint/interact/skill-tree actions.
- **Abilities:** `/Game/Abilitys/BP_LightingThrow`, `/Game/Abilitys/GA_Spell`, fire-spell sample assets.
- **UI:** marketplace in-game HUD and health bar.
- **Working:** third-person movement, camera, sprint/stamina, HUD creation, pause/skill-tree opening, Lightning projectile spawn logic.
- **Disconnected/broken:** current health/mana/death variables are not a complete system; five lives, respawn, checkpoint restore, damage invulnerability, ability ownership, and Chapter save data are absent. GAS is present but not coherently used.
- **Must be created/repaired:** authoritative player-state component, health/damage/death/lives, checkpoint respawn, ability interface, stamina tuning, input gating during cinematics/jump scares, save integration.
- **Dependencies:** Save/checkpoint, HUD, AI damage, objective manager.
- **Risks:** `BP_menuSystemCharacter` is oversized and contains unrelated weapon/inventory samples; `BP_WEAPON` already logs compiler errors.
- **Client questions:** Exact health values, damage values, life-loss rules, and whether game over resets only the checkpoint, the level, or the Chapter.

## Task 4 — Coin System

**Status:** Placeholder — **20%**

- **Blueprint:** `/Game/Blueprints/Coin`.
- **Art:** `/Game/Coin`, Fab Bitcoin mesh/material content.
- **Maps:** approximately 330 placed `Coin_C` external actors.
- **Working:** rotation, overlap response, pickup sound, destroy actor.
- **Disconnected/broken:** no currency increment, duplicate-pickup protection, HUD notification, objective notification, save persistence, upgrade debit, or Chapter 2 carry-over.
- **Must be created/repaired:** currency component/service, pickup interface, data-driven values, transaction API, HUD binding, save fields, spent/available balance policy, placement audit.
- **Dependencies:** Player state, save, objectives, upgrades, Timmy/Purple Burst.
- **Risks:** placed pickups may respawn incorrectly after save/load; “coin” versus “fragment” terminology conflicts.
- **Client questions:** Are fragments and coins the same item? Are spent coins permanently removed? Must all 330 be collectible?

## Task 5 — Skill Tree / Upgrade System

**Status:** Placeholder — **15%**

- **Widgets:** `/Game/Skill_Tree/WB_SkillTree`, `/Game/Skill_Tree/WB_Skill`.
- **Player link:** character can create/toggle the skill-tree widget.
- **Working:** UI can be opened; buttons have basic audio/hover behavior.
- **Disconnected/broken:** no skill definitions, costs, prerequisites, station interaction, coin transaction, grant/revoke, persistence, locked-state display, or Chapter 2 Purple Burst purchase.
- **Must be created:** data-driven skill definitions, upgrade-station actor, transaction validation, unlock/grant path, feedback, save support, ability input and HUD state.
- **Dependencies:** Currency, player ability architecture, save, UI, Timmy.
- **Risks:** deciding whether to repair the existing widget or replace its internals; coexistence with unused GAS/fire sample.
- **Client questions:** Exact costs for Speed, Lightning Throw, and Purple Burst; whether Speed/Lightning unlock automatically or only at stations.

## Task 6 — Objectives

**Status:** Missing — **5%**

- **Assets:** `/Game/AI/Objective_SFX` and generic area text in the custom save object are not an objective system.
- **Working:** no end-to-end objective progression found.
- **Must be created:** objective definitions, state manager, event interface, Timmy dialogue triggers, HUD model, map markers/minimap decision, persistence, boss progress, success/failure transitions.
- **Dependencies:** Every gameplay task emits objective events; save and HUD consume them.
- **Risks:** objectives will become brittle if implemented directly in Level Blueprints.
- **Client questions:** Complete ordered objective list, required/optional objectives, minimap requirement, and exact failure/retry rules.

## Task 7 — HUD / UI

**Status:** Partially complete — **35%**

- **Widgets:** marketplace `WBP_IngameUI`, `WBP_HealthBar`, `/Game/UI/PauseMenu`, death/jump-scare widgets, skill-tree widgets, marketplace credits.
- **Working:** generic health/stamina/cooldown bindings and basic menu widgets exist.
- **Disconnected/broken:** no side panel for current objective, coin totals/costs, boss progress, lives, key state, or ability unlock feedback. Pause is minimal. Death/game-over are not properly separated.
- **Must be created/repaired:** coherent HUD view model, objective panel, five-life display, coin/cost feedback, boss bar, ability/cooldown presentation, polished pause, death, game over, credits skip prompt, accessibility/resolution checks.
- **Dependencies:** Player, objectives, currency, boss, save.
- **Risks:** 149 widget Blueprints from multiple menu packs make ownership and style consistency difficult.
- **Client questions:** Approved visual style, controller support, target resolutions, subtitle/accessibility requirements.

## Task 8 — Teddy Bear Companion

**Status:** Placeholder — **10%**

- **Assets:** HorrorBear mesh/animations, Timmy cover textures, a placed HorrorBear skeletal actor in mansion content.
- **Requirements source:** Timmy dialogue in both PDFs.
- **Working:** art can represent Timmy.
- **Disconnected/broken:** no dedicated Timmy Blueprint, dialogue component, interaction, objective-giver, hologram behavior, hint system, checkpoint, ability unlock, or coin transaction.
- **Must be created:** `BP_Timmy`-style actor/component architecture, interaction/dialogue interface, objective hooks, hologram/appear events, subtitle/audio playback, Purple Burst upgrade interaction, saved state.
- **Dependencies:** Objectives, dialogue/audio, currency, abilities, save, cinematics.
- **Risks:** voice files are not present; story spelling/name inconsistencies.
- **Client questions:** Is Timmy stationary, holographic, or a following companion? Confirm “Bottom Smith” versus other spellings and approved voice recordings.

## Task 9 — Chest and Key System

**Status:** Placeholder — **10%**

- **Art assets:** `/Game/Chest_Keys/chest`, `/Game/Chest_Keys/GateKeys`, Fab chest/key meshes.
- **Related shells:** `BP_PhysicsDoor`, `SimpleDoor`, generic inventory/item assets, `/Game/BP_ForestPortal`.
- **Working:** art exists.
- **Disconnected/broken:** no verified interactable chest, key grant, inventory/key-state persistence, gate validation, boss-entry event, locked feedback, or objective notification.
- **Must be created:** interactable chest, key item/state, gate actor, objective events, one-time persistence, animation/audio/VFX, boss start handoff.
- **Dependencies:** Interaction, objectives, save, boss, UI.
- **Risks:** generic marketplace inventory is overbuilt and disconnected; using it may create more work than a focused key-state component.
- **Client questions:** Is the chest revealed after a coin threshold, and is the key a visible inventory item or a binary quest flag?

## Task 10 — Demo Version

**Status:** Broken — **25%**

- **Exists:** menu packs, candidate mansion/forest maps, player, enemy, coins, HUD fragments, environment.
- **Working:** isolated prototypes and environment scenes.
- **Disconnected/broken:** configured start path is a marketplace example; no stable start-to-ending vertical slice; corrupt map/reference errors; no clean build evidence.
- **Must be created/repaired:** authoritative boot map, New Game path, short complete objective chain, checkpoints, fail/retry, ending, credits, packaging profile, cook list.
- **Dependencies:** all foundation systems; can use reduced content scope after core integration.
- **Risks:** “demo” scope is undefined and may be treated as the whole Chapter.
- **Client questions:** Required demo duration, start/end points, target platform, distribution channel, and whether voice/cutscenes must be final quality.

## Task 11 — Boss Fight

**Status:** Missing — **5%**

- **Assets:** Isabel/CruelDoll art and animations, generic enemy AI, Spider art/demo content, generic projectile/VFX packs.
- **Working:** no dedicated Isabel boss path was found.
- **Disconnected/broken:** no boss controller/tree, AI Perception, Purple Burst projectile, dark-power attack set, boss health component/bar, phases, cousin summon behavior, arena gate lock, victory event, portal event, or persistence.
- **Must be created:** complete boss architecture, multiple attacks, telegraphs, damage interface, health/phases, arena lifecycle, victory contract, portal activation, objective/HUD/save/cutscene events.
- **Dependencies:** Player health/abilities, AI foundation, arena/nav, HUD, objectives, save, cutscenes.
- **Risks:** largest single integration task; missing animation/attack-design decisions and no proven arena.
- **Client questions:** attack list, number of phases, difficulty/duration, cousin summons, whether Lightning damages or stuns Isabel, checkpoint location.

## Task 12 — All Cutscenes

**Status:** Placeholder — **15%**

- **Sequences:** two custom jump-scare sequences, one recorded take/subscene, generic marketplace sequence.
- **Media:** jump-scare media and video scene shells.
- **Working:** a jump-scare presentation path has partial assets.
- **Disconnected/broken:** mansion intro, Isabel appearance, scripted loss, forest drag, Speed unlock, Lightning unlock, boss entrance, spider ending, portal/credits transition, and return-to-mansion scene are missing or unverified.
- **Must be created:** shot list, Level Sequences, cameras, animation/blocking, gameplay-state tracks/events, skip handling, subtitle/audio integration, restore-control logic, checkpoint-safe replay rules.
- **Dependencies:** locked story, characters/animations, voice, maps, objective/boss events.
- **Risks:** cinematic work will be wasted if maps and gameplay contracts change first.
- **Client questions:** final dialogue, voice availability, shot/style approval, which scenes are skippable, and whether skipped scenes must still execute gameplay events.

## Task 13 — Polish

**Status:** Missing — **10%**

- **Exists:** large art/VFX/audio libraries and visually rich environment content.
- **Working:** the reference video demonstrates an intended atmosphere.
- **Disconnected/broken:** inconsistent UI, duplicated systems, unbudgeted VFX/rendering, collision/nav unknowns, placeholder audio, no accessibility pass, no consistent feedback language.
- **Must be done:** lighting/atmosphere pass, VFX/audio feedback, animation transitions, collision/foliage cleanup, performance profiling, UI consistency, subtitles, controller/keyboard UX, save/error messaging.
- **Dependencies:** feature lock.
- **Risks:** polishing before architecture recovery would hide defects and cause rework.
- **Client questions:** target visual platform, minimum specification, frame-rate goal, and final audio/VO budget.

## Task 14 — Final Testing, Bug Fixes and Packaging

**Status:** Broken / not started as a final phase — **10%**

- **Evidence:** known corrupt maps, Blueprint errors, redirectors, missing imports, stale networking config, development packaging configuration, no clean cook evidence.
- **Working:** project opens and asset registry is readable.
- **Disconnected/broken:** no acceptance test suite, no clean compile report, no target-platform package, no controlled cook map list, no playthrough/save migration/performance test evidence.
- **Must be done:** warning/error triage, redirector/reference cleanup, automation tests, smoke maps, clean cook/package, hardware matrix, save/load/restart tests, skip/cutscene tests, collision/nav/performance regression, release configuration.
- **Dependencies:** feature and content lock.
- **Risks:** marketplace content bloat, plugin dependencies, corrupt packages, rendering cost, hidden Blueprint warnings.
- **Client questions:** target platform/store, build size limit, minimum hardware, input devices, language support, and release acceptance criteria.

---

# 9. Detailed boss-fight analysis

| Latest requirement | Audit result | Evidence / gap |
|---|---|---|
| Isabel has dark powers | **Missing** | Dark/magic VFX packs exist, but no connected Isabel dark-power attack logic. |
| Isabel fires Purple Burst projectiles | **Missing** | No Purple Burst gameplay ability/projectile was found. Existing projectiles are Lightning/fire sample content. |
| Complete boss AI | **Missing** | Only generic cousin AI/controller/tree exists. |
| Multiple boss attacks | **Missing** | Generic enemy has a melee-style trace attack; no dedicated boss attack set/phases. |
| Boss health system | **Missing** | No dedicated boss health component/state/HUD path. |
| Victory condition | **Missing** | No authoritative boss-defeated event or saved victory state. |
| Defeat opens portal to mansion | **Existing but disconnected / placeholder** | `BP_ForestPortal` exists and is placed, but its graph is only event shells; no boss-victory binding. |
| Ending cutscene plays | **Missing** | No complete boss ending sequence found. |
| Spider takes Isabel | **Placeholder** | Spider asset/demo content exists; no connected ending sequence or gameplay event. |
| Credits play afterward | **Existing but disconnected** | Generic `WBP_Credits` and `BP_CreditsSpawner` exist; no Chapter 1 endpoint connection. |
| Credits are skippable | **Unable to verify** | Generic credits behavior exists, but the Chapter-required skip path and event completion are not proven. |
| Player returns to mansion | **Missing** | No connected post-credit level transition/checkpoint path. |
| Next checkpoint is approaching Timmy | **Missing** | No Timmy gameplay actor or Chapter checkpoint system. |
| `BP_menuSystemCharacter` unlocks Chapter 2 ability through Timmy | **Missing** | Character exists; Timmy transaction/grant path does not. |
| Chapter 2 ability is Purple Burst | **Missing** | No matching ability implementation. |
| Purple Burst uses Chapter 1 coins | **Missing** | Coin credit, persistence, spending, Timmy interaction, and ability grant are all absent. |

Boss conclusion: the fight is not an integration/polish task. It is a new gameplay feature built on foundations that also require repair.

---

# 10. Critical bugs and blockers

## P0 — blocks reliable development

1. **No authoritative boot and progression route.** The configured game map redirects into a marketplace example.
2. **No central Chapter state model.** Coins, objectives, abilities, lives, checkpoints, boss state, and save persistence have no shared authority.
3. **Missing source requirements.** Two referenced Google documents are absent.
4. **Known package/reference failures.** Two corrupt `SuperPowers` maps and broken DarkForest built-data references exist.
5. **Known Blueprint compiler errors.** `BP_WEAPON` cannot find the expected Speedster equip function/pins.
6. **No functional currency transaction.** Placed coins disappear without crediting the player.
7. **No functional health/lives/checkpoint loop.** AI damage cannot be safely integrated until this contract exists.

## P1 — blocks the vertical slice

8. Timmy is art only, not a gameplay companion.
9. Objectives and upgrade stations are missing.
10. Chest/key/gate and forest portal are not gameplay systems.
11. Lightning behavior conflicts with the requirement: current projectile applies damage, while the quest specifies cousin stun.
12. Cousin AI is a prototype and the client explicitly identifies AI as broken.
13. Mansion and forest ownership is unclear because of duplicates/redirectors.

## P2 — blocks final quality/release

14. Boss fight and Purple Burst are missing.
15. Most required cutscenes and voice assets are missing.
16. Generic credits are not connected to the ending.
17. 98 redirectors and many demo/sample assets create reference/cook risk.
18. Packaging remains in Development with no verified production map/cook list.
19. Rendering and content settings present a high performance risk.
20. No developer cheat/test framework exists, making regression work unnecessarily slow.

---

# 11. Recommended implementation order

## Safest order

1. **Requirements lock and asset ownership**
   - Obtain the two missing documents.
   - Confirm authoritative mansion, forest, menu, and boss-arena maps.
   - Confirm terminology, costs, ending order, target platform, and acceptance criteria.

2. **Recovery baseline**
   - Create a branch/backup outside this audit.
   - Resolve the startup map, broken package references, compiler errors, and production cook scope.
   - Do not delete samples yet; isolate them from the production route.

3. **Foundation gameplay state**
   - Introduce one Chapter-state contract for health, five lives, currency, abilities, key, objectives, checkpoint, and boss outcome.
   - Extend/replace the custom save model to persist that state.

4. **Player death/checkpoint loop**
   - Repair damage, health, death, lives, game over, respawn, and input/cinematic locks.

5. **Currency → objectives → HUD**
   - Repair coin pickup, objective events, side-panel HUD, and persistence before placing/tuning more content.

6. **Ability and upgrade architecture**
   - Keep the usable sprint/stamina and Lightning projectile presentation, but route them through a clean unlock/state interface.
   - Replace the skill-tree internals; do not build transactions in the current empty widgets.

7. **Forest vertical slice**
   - One spawn, coin objective, cousin encounter, Speed unlock, Lightning unlock, chest, key, gate, and checkpoint.

8. **AI repair**
   - Repair cousin AI on the new damage/objective/test contracts.
   - Build original jump scare as an enemy-state outcome, not as ad hoc level loading.

9. **Timmy**
   - Implement dialogue/objective/hint/upgrade interfaces once the systems he calls are stable.

10. **Boss and arena**
    - Build the dedicated Isabel boss system, attacks, health/phases, victory event, and portal.

11. **Cutscenes and ending**
    - Produce sequences only after level blocking and event contracts are stable.

12. **Demo integration, polish, optimization, final QA/package**

## Repair versus replace

**Repair/reuse:**

- `BP_menuSystemCharacter` movement, camera, and sprint/stamina presentation.
- `BP_LightingThrow` visuals/projectile movement, after changing its effect contract to stun where required.
- CruelDoll mesh/animations and basic AI task concepts.
- Environment art, PCG graphs, mansion/forest dressing.
- Menu System Pro settings/menu layer if licensing and target UI are approved.
- Jump-scare media/sequence assets as reference components.

**Replace or substantially rewrite internals:**

- Coin transaction logic.
- Skill-tree/upgrade transaction logic.
- Chapter save schema.
- Health/lives/checkpoint orchestration.
- Objective manager.
- Timmy gameplay logic.
- Chest/key/gate logic.
- Forest portal logic.
- Dedicated boss AI/state machine.

**Do not touch yet:**

- Final cutscene camera work.
- Final lighting/VFX optimization.
- Broad sample-content deletion.
- Production packaging cleanup that could invalidate still-undecided map ownership.

## Best first milestone

A **15–20 minute Chapter 1 systems vertical slice** using one mansion start and one controlled forest route:

- New Game reaches the correct character.
- Timmy assigns a coin objective.
- Coins persist and update HUD.
- Player can lose a life and respawn at a checkpoint.
- Speed unlocks through a real transaction.
- One cousin detects/chases/attacks and triggers a recoverable death flow.
- Lightning unlocks and stuns the cousin.
- Chest/key/gate completes the slice.

This milestone proves the architecture before boss/cinematic production begins.

---

# 12. Milestone plan

| Milestone | Scope and deliverables | Dependencies | Acceptance criteria | Hours | Days at 3 h/day |
|---|---|---|---|---:|---:|
| M0 Requirements and recovery baseline | Missing docs, map ownership, boot route, error inventory, build/cook definition | Client answers | One agreed flow diagram; one authoritative map list; clean development compile/cook target defined | 45 | 15 |
| M1 Core state and player loop | Chapter state, save schema, health, five lives, death/game over, checkpoints, input locks | M0 | Damage → death → life decrement → checkpoint restore works; save/load preserves state | 85 | 29 |
| M2 Currency, objectives, HUD | Coin transactions, objective manager, side panel, persistence | M1 | Pickup increments exactly once; objectives advance; HUD/save remain consistent | 95 | 32 |
| M3 Abilities, upgrades, Timmy foundation | Speed/Lightning unlocks, station/UI, Timmy interaction/dialogue hooks | M2 | Costs validated; unlock persists; Timmy can drive objective/upgrade events | 105 | 35 |
| M4 Forest vertical slice and cousin AI | Production route, nav/collision, AI Perception, attacks, jump scare, chest/key/gate | M1–M3 | Start-to-gate slice is replayable without full Chapter; AI recovers from lost sight; death flow is safe | 135 | 45 |
| M5 Boss fight | Arena, Isabel AI, Purple Burst attacks, phases, health, victory, portal | M1–M4 | Boss can start/reset/finish independently; all attacks telegraph; victory opens portal once | 125 | 42 |
| M6 Cinematics and Chapter ending | Intro/drag/unlocks/boss/spider ending, skippable credits, mansion return, Timmy Purple Burst purchase | M3–M5; final VO/story | Skip and normal playback produce identical gameplay state; player returns with correct coins/checkpoint | 115 | 39 |
| M7 Demo integration and polish | Menu flow, sound/VFX/UI polish, performance, accessibility, content/cook cleanup | M0–M6 | Complete Chapter demo meets agreed frame rate and UX checklist | 80 | 27 |
| M8 Final QA and release package | Regression, save/load matrix, automation, clean cook/package, hardware testing | Feature lock | Zero blocker/critical defects; repeatable signed-off build; documented known issues | 76 | 26 |

Milestone hours total **861**, matching the realistic task estimate. Some work can overlap after M1, but final integration remains sequential.

---

# 13. Task-by-task time estimate

Hours are based on the inspected state, not just asset names. “Implementation” includes substantial repair where the current asset is only a shell.

| Task | Investigation / debugging | Implementation | Integration | Testing | Polish | Best case | Realistic | Risk-adjusted |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1. Forest | 5 | 16 | 10 | 8 | 10 | 35 | 49 | 66 |
| 2. Isabel AI and Jump Scare | 7 | 20 | 12 | 10 | 8 | 41 | 57 | 77 |
| 3. Player System | 6 | 22 | 14 | 12 | 8 | 45 | 62 | 84 |
| 4. Coin System | 3 | 10 | 8 | 6 | 4 | 22 | 31 | 42 |
| 5. Skill Tree / Upgrade | 5 | 24 | 16 | 10 | 8 | 45 | 63 | 85 |
| 6. Objectives | 4 | 18 | 14 | 10 | 6 | 37 | 52 | 70 |
| 7. HUD / UI | 5 | 20 | 14 | 10 | 10 | 42 | 59 | 80 |
| 8. Teddy Bear Companion | 5 | 18 | 14 | 8 | 8 | 38 | 53 | 72 |
| 9. Chest and Key | 3 | 10 | 8 | 6 | 4 | 22 | 31 | 42 |
| 10. Demo Version | 4 | 12 | 12 | 14 | 8 | 36 | 50 | 68 |
| 11. Boss Fight | 8 | 40 | 24 | 20 | 14 | 76 | 106 | 143 |
| 12. All Cutscenes | 6 | 34 | 20 | 14 | 14 | 63 | 88 | 119 |
| 13. Polish | 6 | 20 | 14 | 16 | 24 | 57 | 80 | 108 |
| 14. Final Testing / Packaging | 8 | 18 | 18 | 28 | 8 | 57 | 80 | 108 |
| **Total** | **75** | **282** | **198** | **182** | **134** | **616** | **861** | **1,164** |

## Evidence supporting the estimates

- Forest is content-rich but requires routing, collision/nav, gameplay integration, and optimization rather than ground-up art production.
- AI has useful tasks/animations but needs a real state model, Perception, recovery behavior, damage integration, and extensive nav testing.
- Player movement can be retained, while lives/checkpoint/state/ability ownership must be built.
- Coins and chest/key are small individually, but both cross UI, save, objectives, and level persistence.
- Skill/upgrade and objective systems are largely missing and are foundation work.
- UI has many reusable widgets but no coherent Chapter view model.
- Timmy requires new gameplay behavior and dialogue integration; voice assets are missing.
- Boss and cutscenes are the largest content/iteration risks because dedicated implementation is absent.
- Final QA is large because of 73 maps, 17,000+ assets, corrupt references, redirectors, plugins, World Partition, and no current clean-package evidence.

Tasks 11–14 have the widest uncertainty. Missing boss design, final voice/cinematic assets, target hardware, and the two unavailable documents can move them outside the stated ranges.

---

# 14. Total realistic project estimate

- **Best case:** 616 hours
- **Realistic:** 861 hours
- **Risk-adjusted:** 1,164 hours

Realistic conversion:

- At **3 hours/day:** **287 working days**
- At **1.5 hours/day:** **574 working days**
- At **6 hours/day:** **144 working days** (rounded up)

These are productive working days, not calendar days. They exclude client-response delays, voice-recording lead time, marketplace-license problems, store certification, and major visual redesign.

The estimate can fall materially only if the client approves a much smaller demo scope, supplies final voice/cinematic assets, and accepts existing environment/UI quality. It can rise if the two missing documents add mechanics or if the current maps fail runtime/nav/package testing.

---

# 15. Testing strategy

## What already exists

- Small prototype/sample maps exist, including `Forest_Prototype`, but none is a verified Chapter developer map.
- Generic marketplace save slots exist.
- No project-specific cheat manager, debug menu, teleport command set, checkpoint selector, ability grant, coin grant, boss-start command, or cutscene test controller was found.
- No dedicated boss test map or automated gameplay test suite was found.
- `AMCP_Test` is empty and is not a testing framework.

## Recommended future developer framework

Do not create this until the recovery architecture is approved. Once approved, add:

1. **Developer test map**
   - Lightweight rooms for player damage/lives, coin pickup, upgrade station, Timmy, cousin AI, chest/key/gate, portal, and boss attacks.

2. **Project Cheat Manager or development-only subsystem**
   - `SOTM.GotoCheckpoint <Id>`
   - `SOTM.SetCoins <Value>`
   - `SOTM.GrantAbility Speed|Lightning|PurpleBurst`
   - `SOTM.SetLives <Value>`
   - `SOTM.StartObjective <Id>`
   - `SOTM.StartBoss [Phase]`
   - `SOTM.KillBoss`
   - `SOTM.PlaySequence <Id>`
   - `SOTM.ReturnToMansion`
   - `SOTM.ClearSave`

3. **Debug overlay**
   - Current map/checkpoint/objective, coins, unlocked abilities, lives/health, AI state/target, boss phase, save slot, and last gameplay event.

4. **Checkpoint/save-slot selector**
   - Development-only predefined saves: Mansion Intro, Forest Start, Before Speed, Before Lightning, Before Chest, Boss Entrance, Boss Phase 2, Ending, Mansion Return.

5. **Automation**
   - Functional tests for coin idempotency, purchase validation, five-life transitions, save/load, objective order, gate requirements, boss reset/victory, cutscene skip-state equivalence, and post-credit return.

## How to test each system without replaying the Chapter

| System | Isolated test |
|---|---|
| Player movement/stamina | Spawn in developer test room; run deterministic drain/recovery timing checks. |
| Health/lives/death | Damage volumes with 1, exact-lethal, and overkill damage; validate five deaths and game over. |
| Checkpoints | Select checkpoint from debug menu, respawn, restart editor, reload slot. |
| Coins | Spawn single/stacked coins, collect during lag/overlap, save/reload, verify no duplicate credit. |
| Upgrades | Set balance just below/equal/above cost; buy, reload, and try duplicate purchase. |
| Objectives | Start any objective ID; emit required event; verify HUD/save/next objective. |
| HUD | Inject view-model values and test target resolutions/input methods. |
| Timmy | Select dialogue/objective/upgrade state directly in test map. |
| Cousin AI | Toggle sight/hearing, block nav, teleport player, force attack, lose target, stun. |
| Jump scare | Trigger success/failure/skip/restart without level travel. |
| Chest/key/gate | Test no key, key acquired, saved key, opened gate, reload after open. |
| Boss | Start at each phase, force attacks, set health, die/retry, kill boss, validate one victory event. |
| Portal/ending | Trigger boss-victory event directly; test normal and skipped sequences. |
| Credits/mansion return | Start credits alone; skip at several timestamps; verify identical mansion checkpoint and coins. |
| Packaging | Automated smoke launch into the production boot map, New Game, checkpoint load, and quit/relaunch. |

---

# 16. Client clarification questions

## Requirements and scope

1. Please supply exported copies of the two missing Google documents.
2. Is “coin” the final name for the glowing fragments, or are coins and fragments separate collectibles?
3. What is the approved Chapter/demo duration?
4. Which maps are authoritative for main menu, mansion, forest, and boss arena?
5. Which marketplace/demo maps may be excluded from production?

## Progression

6. Exact coin costs for Speed, Lightning Throw, and Purple Burst?
7. Are Speed and Lightning purchased at stations, granted automatically at thresholds, or granted by Timmy?
8. Can the player spend coins before the Chapter 2 Purple Burst purchase, and can that make Purple Burst unaffordable?
9. Does Lightning only stun cousins, damage Isabel, or do both with target-specific effects?
10. What exactly is saved at each checkpoint?

## Player and fail states

11. Exact player health, cousin damage, boss damage, invulnerability time, and respawn rules?
12. After all five lives are lost, does Game Over reset the checkpoint, forest, Chapter, or save slot?
13. Are lives restored at checkpoints or only on New Game?

## Timmy and narrative

14. Confirm Timmy’s canonical name/spelling (“Timmy Bottom Smith” in supplied PDFs versus variants visible elsewhere).
15. Is Timmy stationary in the mansion, a hologram in the forest, or a following companion?
16. Are voice recordings available, and which dialogue document is final?
17. Must every cutscene be skippable? If skipped, should subtitles/dialogue summaries appear?
18. Does the portal open before the spider cutscene or after it?
19. Do credits occur before or after physically entering the portal?
20. Is Purple Burst purchased immediately at the Chapter 1 ending or only teased for Chapter 2?

## AI and boss

21. Which cousin variants are required and how do their behaviors differ?
22. What are Isabel’s required attacks beyond Purple Burst?
23. How many boss phases, target fight duration, and difficulty level?
24. Does Isabel summon cousins? At what health thresholds?
25. Is the existing CruelDoll/Spider art final and licensed for release?

## Release

26. Target platform/store, minimum hardware, frame-rate target, resolution, controller support, and maximum build size?
27. Required languages, subtitles, accessibility, and content warnings?
28. What are the client’s objective acceptance tests for “Demo Version” and “Chapter 1 complete”?

---

# 17. Recommended first development task

After approval and receipt of the missing requirements, the first implementation task should be:

## Establish the authoritative Chapter 1 boot, state, and test baseline

Scope:

1. Select one main menu, one mansion map, one forest map, and one boss-arena ownership plan.
2. Correct the development boot/New Game route.
3. Define the Chapter state schema: coins, abilities, health, lives, objective, checkpoint, key, boss state.
4. Repair the save object to persist that schema.
5. Build a minimal developer test map/debug entry so every later system can be tested in isolation.
6. Resolve the known compile/load blockers that prevent a clean baseline.

Acceptance criteria:

- The project starts in the approved menu.
- New Game spawns `BP_menuSystemCharacter` in the approved mansion.
- A development command/selector can enter the player-system test area.
- Chapter state can be saved, editor/game restarted, and loaded unchanged.
- A clean development compile and controlled test cook have no blocker errors.
- No production gameplay feature is considered complete merely because an asset exists.

This should precede forest expansion, AI repair, boss work, or cinematics. It removes the highest rework risk and gives every later milestone a stable integration target.

---

**Audit stop point:** Investigation and planning are complete. No Unreal project implementation should begin until this report is approved and the critical client questions are answered.
