# SOTM Git LFS Safe Push Audit

Audit date: 2026-08-13  
Project: The Secrets of the Mansion  
Repository: `C:\UE Projects\CURRENT GAME\CURRENT GAME 5.6`

## 1. Current branch and remote

- Branch: `main`
- Remote: `origin`
- Fetch URL: `https://github.com/CJTechnology21/Game-USA-Unreal.git`
- Push URL: `https://github.com/CJTechnology21/Game-USA-Unreal.git`
- After an authenticated `git fetch origin main`, the local branch remains one commit ahead and zero commits behind.
- The working tree was clean before this audit report was created.

## 2. Commits ahead of origin

One local commit is pending:

```text
495754d2 Coins and Load level with coins
```

`git rev-list --left-right --count origin/main...HEAD` returned `0 1` (zero remote-only commits, one local-only commit).

## 3. Files pending push

The pending commit contains 350 paths: 343 modified and 7 added.

| Group | Count | Classification |
|---|---:|---|
| `Content/__ExternalActors__/.../CH1/` | 330 | A — required World Partition project content |
| Other `Content/` | 2 | A — required SaveGame Blueprint plus project Python utility |
| `Source/` | 11 | A — required C++ project source |
| `ProjectDocs/` | 6 | A — project documentation/allowlist |
| `.codex_mcp_session` | 1 | C — repository-local tooling state; preserved, not altered by this audit |

Non-external-actor pending paths are:

```text
.codex_mcp_session
Content/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject.uasset
Content/Python/SOTM_AssignPersistentCoinIds.py
ProjectDocs/SOTM_CH1_Continue_Lighting_Initialization_Fix_Report.md
ProjectDocs/SOTM_CH1_Persistent_Forest_Lighting_Fix_Report.md
ProjectDocs/SOTM_Coin_Autosave_OnPickup_Report.md
ProjectDocs/SOTM_Coin_Phase2_ExternalActor_Allowlist.txt
ProjectDocs/SOTM_Coin_System_Final_Acceptance_Report.md
ProjectDocs/SOTM_Coin_System_Phase2_Persistence_Report.md
Source/SOTM1/Private/Coin/SOTMCoinPickup.cpp
Source/SOTM1/Private/SOTMPlayerBlueprintLibrary.cpp
Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp
Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp
Source/SOTM1/Private/Tests/SOTMPlayerFoundationTests.cpp
Source/SOTM1/Private/UI/SOTMIngameUIWidget.cpp
Source/SOTM1/Public/Coin/SOTMCoinPickup.h
Source/SOTM1/Public/SOTMPlayerBlueprintLibrary.h
Source/SOTM1/Public/SOTMPlayerFoundationWorldSubsystem.h
Source/SOTM1/Public/SOTMPlayerStateSubsystem.h
Source/SOTM1/Public/UI/SOTMIngameUIWidget.h
```

The 330 external-actor paths are intentionally summarized by their production directory rather than reproduced here; the complete list is available from `git diff --name-status origin/main..HEAD` and is also represented by `ProjectDocs/SOTM_Coin_Phase2_ExternalActor_Allowlist.txt` in the pending commit.

## 4. LFS files pending push

- Pending LFS paths: **331**
- Composition: 330 CH1 World Partition `.uasset` external actors and `BP_CustomSaveGameObject.uasset`.
- Total local payload represented by these pending working-tree files: approximately **1.742 MiB**.
- `git lfs fsck` result: `Git LFS fsck OK`.
- Repository-wide LFS paths at `HEAD`: 16,126.

These are valid required Unreal assets. None may be ignored, untracked, converted to ordinary Git blobs, or removed to evade the quota.

GitHub did not identify one corrupt or oversized object. It refused LFS batch requests because the repository/account LFS budget is exhausted. Therefore the blocking set is the complete 331-object pending upload queue, not one exceptional file.

## 5. Largest pending files

The pending commit is small in payload despite its path count. The largest pending files in the working tree are:

| Size | Path | Classification |
|---:|---|---|
| 41,580 B | `ProjectDocs/SOTM_Coin_Phase2_ExternalActor_Allowlist.txt` | A — project documentation |
| 36,728 B | `Source/SOTM1/Private/SOTMPlayerStateSubsystem.cpp` | A — required source |
| 25,154 B | `Content/MenuSystemPro/Blueprints/SaveGame/BP_CustomSaveGameObject.uasset` | A — required project content/LFS |
| 14,413 B | `Source/SOTM1/Private/SOTMPlayerFoundationWorldSubsystem.cpp` | A — required source |
| 13,210 B | `ProjectDocs/SOTM_Coin_System_Final_Acceptance_Report.md` | A — project documentation |
| 12,141 B | `ProjectDocs/SOTM_Coin_System_Phase2_Persistence_Report.md` | A — project documentation |

The largest pending LFS object is `BP_CustomSaveGameObject.uasset` at 25,154 bytes. The CH1 external actors are generally about 5.3 KiB each. This proves the current failure is not caused by one large pending file.

For context, the repository's largest already tracked LFS paths include a 219 MB MetaHuman package, a 192 MB built-data asset, 186 MB and 121 MB texture assets, a 180 MB map, a 156 MB FBX, and a 122 MB environment mesh. They are existing project content and were not modified or candidates for removal in this maintenance task.

## 6. Current `.gitignore` findings

The existing `.gitignore` correctly covers the standard generated/local directories:

```text
.vs/
Binaries/
DerivedDataCache/
Intermediate/
Saved/
```

It also covers local packaged output (`LocalBuilds/`, `Packaged/`, `Packages/`, `Releases/`, `StagedBuilds/`), editor metadata, Python caches, and generated project files. It does not ignore `Content/`, `Config/`, `Source/`, required project assets, external actors, or the `.uproject` file.

No `.gitignore` change was needed or made.

## 7. Current `.gitattributes` findings

The existing `.gitattributes` correctly assigns Git LFS to Unreal and large binary asset types, including:

```text
*.uasset
*.umap
*.fbx
*.blend
*.psd
*.tga
*.exr
*.hdr
*.wav
*.mp4
*.mhpkg
*.png
*.jpg
*.jpeg
*.tif
*.tiff
```

The required `.uasset` and `.umap` coverage is correct. No LFS rule was disabled or changed.

## 8. Generated files safely removed from tracking

None.

`git ls-files` confirmed zero tracked paths under `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, and `.vs/`. There was therefore no safe index cleanup to perform.

`.codex_mcp_session` is tracked and changed in the pending commit. It appears to be tooling session state rather than Unreal runtime content, but it is tiny, non-LFS, already committed, and not responsible for the push failure. It was classified C (unknown/preserve) and was not removed because doing so would require a separately reviewed commit change.

## 9. Required Unreal assets preserved

Confirmed:

- No project file was deleted.
- No `Content/`, `.uasset`, `.umap`, `__ExternalActors__`, or `__ExternalObjects__` rule was added to `.gitignore`.
- No LFS rule was disabled.
- No required asset was removed from Git tracking.
- No history rewrite, BFG, `git filter-repo`, reset, revert, or force push was used.
- The pending SaveGame Blueprint and all 330 pending CH1 external actors remain tracked through LFS.
- Required `Config/`, `Source/`, `SOTM1.uproject`, Content, and MetaHuman/plugin-style project content remain present in Git.

## 10. `git lfs push --dry-run origin main` result

The authenticated dry run succeeded with exit code 0 and enumerated 331 uploads. It demonstrated that local LFS pointers and objects are internally valid and that the queue can be identified. A dry run does not reserve quota and does not prove GitHub will accept bytes.

An initial sandboxed dry run could not authenticate because Git Credential Manager exited with `0xc0000005`. The same command was then run through the authenticated host environment and succeeded. That local credential-process issue is separate from the GitHub LFS budget rejection.

## 11. Safe `git push origin main` result

The one permitted non-force push was attempted. It failed before uploading any LFS object:

```text
Uploading LFS objects:   0% (0/101), 0 B | 0 B/s, done.
batch response: This repository exceeded its LFS budget. The account responsible for the budget should increase it to restore access.
error: failed to push some refs to 'https://github.com/CJTechnology21/Game-USA-Unreal.git'
```

No further push attempts or workarounds were made. After the failure, `main` remains one commit ahead of `origin/main` at `495754d2`.

The `0/101` display is Git LFS's current upload batch, not the full queue size. The dry run independently enumerated the complete 331-object queue.

## 12. Exact failure reason

**A complete GitHub push cannot succeed until the repository owner restores/increases Git LFS budget.**

The failure is an account/repository billing or quota condition returned by GitHub's LFS batch API. It is not a C++ error, Unreal asset error, oversized pending file, corrupt LFS pointer, `.gitignore` problem, or local Git history problem.

## 13. Client/repository-owner action required

Yes. The owner of the GitHub account responsible for `CJTechnology21/Game-USA-Unreal` must restore Git LFS access by increasing/purchasing the applicable Git LFS storage/bandwidth budget or correcting its billing/quota state. Once GitHub reports LFS access restored, rerun:

```powershell
git lfs push --dry-run origin main
git push origin main
git status -sb
```

Do not remove the CH1 external actors or SaveGame Blueprint to reduce the queue.

## 14. Exact files changed by this Git-maintenance task

Created only:

```text
ProjectDocs/SOTM_Git_LFS_Safe_Push_Audit.md
```

No `.gitignore`, `.gitattributes`, gameplay, lighting, map, Blueprint, asset, source, or previously committed file was modified by this audit. Nothing was staged or committed.

## 15. Exact Git commands executed

The following Git commands were executed. Some were wrapped in PowerShell only to count/group output without altering the repository:

```text
git status
git branch --show-current
git remote -v
git log -5 --oneline
git status -sb
git rev-list --left-right --count origin/main...HEAD
git log --oneline origin/main..HEAD
git status --porcelain=v1 --untracked-files=all
git show --summary --format=fuller HEAD
git diff --shortstat origin/main..HEAD
git diff --name-status origin/main..HEAD
git diff --name-only origin/main..HEAD
git diff origin/main..HEAD -- .codex_mcp_session
git lfs version
git lfs status
git lfs status --json
git lfs ls-files --size
git lfs fsck
git check-attr filter -- <each pending path>
git ls-files
git ls-files Build
git ls-files Plugins
git ls-files '*.uproject' 'Config/*' 'Source/*'
git fetch origin main
git lfs push --dry-run origin main
git push origin main
git log -1 --oneline
```

No destructive Git command was executed.

## 16. Recommended next action

1. The repository owner restores/increases the GitHub LFS budget.
2. Run one new authenticated LFS dry run.
3. If it succeeds, run the ordinary non-force `git push origin main`.
4. Verify `git status -sb` shows `main...origin/main` with no ahead/behind marker.
5. Commit this audit report later only as part of a deliberately reviewed documentation commit; do not fold it into unrelated gameplay work automatically.

If GitHub LFS cannot be restored promptly, prepare a complete client handoff archive only when explicitly requested. A read-only handoff plan should keep `Config/`, `Content/`, required `Plugins/` if present, `Source/`, project-specific `Build/` files if present, `MetaHumans/`/other required content roots, and `SOTM1.uproject`; it may exclude only generated `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `.vs/`, and local packaged-output folders. No archive was created during this audit.
