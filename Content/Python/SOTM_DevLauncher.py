"""Editor-only shortcuts for loading the three approved production maps."""

import unreal


OWNER = "SOTMFoundationDevLauncher"
MENU_NAME = "LevelEditor.MainMenu.SOTMFoundation"

MAPS = {
    "Open Production Main Menu": "/Game/Main_Menu_Map",
    "Open Production Mansion": "/Game/Mansion_GameStart",
    "Open Production Forest": (
        "/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1"
    ),
}


def open_map(map_path):
    """Load one approved production map in the Unreal Editor."""
    if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
        unreal.log_error("SOTM Dev Launcher: map does not exist: {}".format(map_path))
        return False

    return unreal.EditorLoadingAndSavingUtils.load_map(map_path)


def register_menu():
    """Register an editor-only main-menu launcher for foundation smoke testing."""
    menus = unreal.ToolMenus.get()
    try:
        menus.unregister_owner_by_name(OWNER)
    except Exception:
        pass

    main_menu = menus.extend_menu("LevelEditor.MainMenu")
    main_menu.add_sub_menu(
        OWNER,
        "SOTMFoundation",
        "SOTMFoundation",
        "SOTM Dev",
        "Open an approved production map directly for development testing.",
    )

    launcher_menu = menus.extend_menu(MENU_NAME)
    for index, (label, map_path) in enumerate(MAPS.items()):
        entry = unreal.ToolMenuEntry(
            name="SOTMFoundation.Map{}".format(index),
            type=unreal.MultiBlockType.MENU_ENTRY,
        )
        entry.set_label(label)
        entry.set_tool_tip("Load {}".format(map_path))
        entry.set_string_command(
            unreal.ToolMenuStringCommandType.PYTHON,
            "",
            "import SOTM_DevLauncher; "
            "SOTM_DevLauncher.open_map({!r})".format(map_path),
        )
        launcher_menu.add_menu_entry("SOTMFoundationMaps", entry)

    menus.refresh_all_widgets()
    unreal.log("SOTM Dev Launcher registered for three production maps.")


if __name__ == "__main__":
    register_menu()
