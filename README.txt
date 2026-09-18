THE SECRETS OF THE MANSION - CHAPTER 1
GitHub Download, Setup, Build, and Run Guide
============================================================

PROJECT INFORMATION
-------------------

Project file: SOTM1.uproject
Required Unreal Engine version: Unreal Engine 5.6
Required platform for the current development setup: Windows 64-bit
Visual Studio target: SOTM1Editor
Build configuration: Development Editor | Win64
Production startup map: /Game/Main_Menu_Map


1. REQUIRED SOFTWARE
--------------------

Install the following before downloading the project:

1. Epic Games Launcher.
2. Unreal Engine 5.6.
3. Visual Studio 2022 Community, Professional, or Enterprise.
4. Git for Windows.
5. Git LFS (Large File Storage).

In the Unreal Engine 5.6 installation options, install:

- Core Components
- Starter Content (recommended)
- Engine Source (recommended for debugging)
- Editor Symbols for Debugging (optional but useful)

In Visual Studio Installer, install these workloads:

- Game development with C++
- Desktop development with C++

Also make sure these components are installed:

- Visual Studio Tools for Unreal Engine
- MSVC v143 C++ x64/x86 build tools
- Windows 11 SDK 10.0.22621.0 or a compatible newer SDK
- .NET support installed by Visual Studio/Unreal

The repository contains a .vsconfig file. After cloning, Visual Studio may offer
to install its listed components automatically. You can also open .vsconfig from
File Explorer and follow the Visual Studio Installer prompts.


2. ENABLE GIT LFS BEFORE CLONING
--------------------------------

Open PowerShell, Command Prompt, or Git Bash and run:

    git lfs install

Confirm that Git LFS is available:

    git lfs version

This project stores Unreal assets, maps, images, audio, video, and other large
binary files through Git LFS. A normal Git clone without a successful LFS pull
may leave small text pointer files instead of real .uasset and .umap files.


3. CLONE THE GITHUB REPOSITORY
------------------------------

Choose a folder with plenty of free disk space. Avoid OneDrive-synchronized
folders and unusually long directory paths.

Run:

    git clone YOUR_GITHUB_REPOSITORY_URL "CURRENT GAME 5.6"
    cd "CURRENT GAME 5.6"
    git lfs pull

Replace YOUR_GITHUB_REPOSITORY_URL with the actual HTTPS or SSH GitHub URL.

If the repository was downloaded as a ZIP instead of cloned, Git LFS-managed
content may be incomplete. Cloning with Git and running git lfs pull is strongly
recommended.

Optional verification:

    git lfs ls-files
    git status

After a clean download, git status should normally report a clean working tree.


4. VERIFY UNREAL ENGINE ASSOCIATION
-----------------------------------

The project is associated with Unreal Engine 5.6.

In File Explorer:

1. Right-click SOTM1.uproject.
2. Select "Switch Unreal Engine version" if that option is available.
3. Select Unreal Engine 5.6.
4. Confirm the selection.

Do not convert the project to another engine version unless the project owner
has approved the upgrade.


5. GENERATE VISUAL STUDIO PROJECT FILES
---------------------------------------

Recommended method:

1. Right-click SOTM1.uproject.
2. Select "Generate Visual Studio project files".
3. Wait for generation to finish.

This creates or refreshes SOTM1.sln and the generated Visual Studio metadata.

If the right-click option is unavailable, run this command from Command Prompt.
Adjust the Unreal Engine installation path if UE 5.6 is installed elsewhere:

    "C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="FULL_PATH_TO_PROJECT\SOTM1.uproject" -game -engine

Example project path:

    C:\UE Projects\CURRENT GAME\CURRENT GAME 5.6\SOTM1.uproject


6. BUILD THE EDITOR TARGET IN VISUAL STUDIO
-------------------------------------------

1. Open SOTM1.sln in Visual Studio 2022.
2. Wait for Visual Studio to finish loading and indexing the solution.
3. In the top toolbar, choose:

       Solution Configuration: Development Editor
       Solution Platform: Win64

4. In Solution Explorer, right-click the SOTM1 project and choose Build.
   You may also use Build > Build Solution.
5. Wait for the build to finish.
6. Confirm that Visual Studio reports "Build succeeded".

The first build can take longer because Unreal must generate project metadata,
compile C++ modules, and prepare caches.


7. OPEN AND RUN THE PROJECT
---------------------------

After the Editor build succeeds:

1. Close Visual Studio only if you do not need it for debugging.
2. Double-click SOTM1.uproject.
3. If Unreal asks whether to rebuild missing modules, choose Yes.
4. Wait for shaders and Derived Data Cache processing to finish.
5. Do not assume the project is frozen during the first shader compilation.
6. Confirm that the project opens with no C++ module build failure.

To run the game in the Editor:

1. Open /Game/Main_Menu_Map if it is not already open.
2. Click the Play button.
3. The game should begin at the production Main Menu.
4. Use New Game to enter the Mansion.

Expected production flow:

    Main Menu -> Mansion -> Forest

The first PIE session may stutter temporarily while shaders and assets finish
loading. Later runs should be smoother once the local caches are populated.


8. IMPORTANT EXTERNAL PLUGIN NOTE
---------------------------------

SOTM1.uproject currently contains this optional external plugin search path:

    ../../../MCP/Unreal_mcp/plugins

On the original development computer, this resolves to an external Unreal MCP
tooling folder. Unreal MCP is a development automation tool and is not part of
the production game flow.

If the project opens normally, no action is required.

If Unreal reports an error specifically about this external plugin directory,
either install the same Unreal MCP tooling at the expected relative path or ask
the project maintainer for the approved portable configuration. Do not download
random plugins or remove required production plugins to bypass an error.


9. COMMON PROBLEMS AND FIXES
----------------------------

Problem: Unreal assets are only a few bytes or contain Git LFS pointer text.

Fix:

    git lfs install
    git lfs pull


Problem: "The following modules are missing or built with a different engine version."

Fix:

1. Confirm Unreal Engine 5.6 is selected.
2. Close Unreal Editor.
3. Regenerate Visual Studio project files.
4. Build SOTM1Editor using Development Editor | Win64.
5. Reopen SOTM1.uproject.


Problem: Visual Studio cannot find a compiler or Windows SDK.

Fix:

1. Open Visual Studio Installer.
2. Modify the Visual Studio 2022 installation.
3. Install "Game development with C++" and the Windows SDK.
4. Reboot if the installer requests it.
5. Regenerate project files and build again.


Problem: The build succeeds but a newly compiled Unreal DLL is blocked by Windows.

Fix:

Check Windows Security notifications and the file's Properties dialog. Do not
disable Smart App Control, antivirus, or other security protections globally.
Ask the project owner before changing security policy.


Problem: The first launch is slow or appears to pause.

Fix:

Allow shader compilation and Derived Data Cache generation to complete. Keep the
project on a fast SSD and ensure sufficient free disk space.


Problem: Maps or textures are missing.

Fix:

1. Close Unreal Editor.
2. Run git lfs pull again.
3. Check git status for incomplete or locally modified files.
4. Do not create replacement assets before confirming the Git LFS download.


10. SAFE GENERATED-FILE CLEANUP
-------------------------------

If project generation or compilation becomes corrupted, close Unreal Editor and
Visual Studio first. The following folders are generated locally and can usually
be regenerated:

- .vs
- Binaries
- DerivedDataCache
- Intermediate
- Saved (WARNING: contains local SaveGames, logs, screenshots, and settings)

Do not delete Saved unless its local saves and evidence are backed up first.

Never delete these project-source folders/files during troubleshooting:

- Config
- Content
- Source
- ProjectDocs
- SOTM1.uproject
- .gitattributes
- .gitignore


11. OPTIONAL: CREATE A WINDOWS DEVELOPMENT BUILD
------------------------------------------------

Packaging is not required merely to work on or run the project in Unreal Editor.

When a packaged test build is required:

1. Open the project in Unreal Engine 5.6.
2. Confirm the Editor build and production maps work in PIE first.
3. Select a Windows Development configuration.
4. Use Platforms > Windows > Package Project.
5. Choose a new output folder outside Content, Source, and Config.
6. Launch the generated SOTM1.exe from the packaged output folder.

Do not commit packaged output, Binaries, Intermediate, Saved, or
DerivedDataCache to Git.


12. SOURCE CONTROL RULES
------------------------

Before committing changes:

    git status --short

Commit only intentional source files, configuration, documentation, and required
Unreal assets. Do not commit generated files from:

- Binaries
- DerivedDataCache
- Intermediate
- Saved
- local packaged-build folders

Unreal binary assets must remain under Git LFS according to .gitattributes.


QUICK START SUMMARY
-------------------

1. Install Unreal Engine 5.6, Visual Studio 2022 C++ tools, Git, and Git LFS.
2. Run git lfs install.
3. Clone the repository.
4. Run git lfs pull inside the project folder.
5. Associate SOTM1.uproject with Unreal Engine 5.6.
6. Generate Visual Studio project files.
7. Open SOTM1.sln.
8. Build SOTM1Editor using Development Editor | Win64.
9. Open SOTM1.uproject.
10. Wait for shader compilation to finish.
11. Open /Game/Main_Menu_Map and click Play.

