"""Assign and validate stable IDs on production CH1 Coin external actors only.

Run from Unreal Editor with CH1 open. Existing valid unique IDs are preserved.
The script does not change actor transforms, components, visuals, or non-Coin actors.
"""

import unreal


CH1_PACKAGE = "/Game/MenuSystemPro/ExampleContent/Designs/Design_Silence/Levels/CH1"
COIN_CLASS = "/Game/Blueprints/Coin.Coin_C"


def _guid_key(value):
    return value.export_text().replace("-", "").replace("{", "").replace("}", "").lower()


def run():
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world or world.get_outermost().get_name() != CH1_PACKAGE:
        raise RuntimeError("Open production CH1 before running the Coin ID migration.")

    coin_class = unreal.load_class(None, COIN_CLASS)
    if not coin_class:
        raise RuntimeError("Production Coin class could not be loaded.")

    initial_dirty = [package.get_name() for package in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
    if initial_dirty:
        raise RuntimeError(
            "Refusing Coin migration while map packages are already dirty: " + ", ".join(sorted(initial_dirty))
        )

    coins = list(unreal.GameplayStatics.get_all_actors_of_class(world, coin_class))
    coins.sort(key=lambda actor: actor.get_path_name())
    seen = {}
    modified = []
    duplicates = []

    for coin in coins:
        before_transform = coin.get_actor_transform()
        coin_id = coin.get_editor_property("persistent_coin_id")
        key = _guid_key(coin_id)
        valid = key and key != "0" * 32

        if valid and key not in seen:
            seen[key] = coin.get_path_name()
            continue

        if valid:
            duplicates.append((key, seen[key], coin.get_path_name()))

        new_id = unreal.Guid()
        new_id.set_editor_property("a", unreal.MathLibrary.random_integer(2**31 - 1) + 1)
        new_id.set_editor_property("b", unreal.MathLibrary.random_integer(2**31 - 1) + 1)
        new_id.set_editor_property("c", unreal.MathLibrary.random_integer(2**31 - 1) + 1)
        new_id.set_editor_property("d", unreal.MathLibrary.random_integer(2**31 - 1) + 1)
        new_key = _guid_key(new_id)
        while new_key in seen or new_key == "0" * 32:
            new_id.set_editor_property("d", unreal.MathLibrary.random_integer(2**31 - 1) + 1)
            new_key = _guid_key(new_id)

        coin.modify()
        coin.set_editor_property("persistent_coin_id", new_id)
        if not coin.get_actor_transform().equals(before_transform):
            raise RuntimeError("Transform changed unexpectedly: " + coin.get_path_name())

        seen[new_key] = coin.get_path_name()
        modified.append((coin.get_path_name(), coin.get_package().get_name()))

    final_ids = [_guid_key(c.get_editor_property("persistent_coin_id")) for c in coins]
    if len(final_ids) != len(set(final_ids)) or any(not value or value == "0" * 32 for value in final_ids):
        raise RuntimeError("Coin ID validation failed; no save was performed.")

    expected_packages = {package_name for _, package_name in modified}
    dirty_packages = {
        package.get_name(): package for package in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    }
    if set(dirty_packages) != expected_packages:
        raise RuntimeError(
            "Dirty package allowlist mismatch; no save performed. Expected={} Actual={}".format(
                sorted(expected_packages), sorted(dirty_packages)
            )
        )

    if dirty_packages and not unreal.EditorLoadingAndSavingUtils.save_packages(
        [dirty_packages[name] for name in sorted(dirty_packages)], True
    ):
        raise RuntimeError("Failed to save exact Coin external actor package allowlist.")

    for coin_path, package_name in modified:
        unreal.log("SOTM_COIN_ID_MODIFIED | {} | {}".format(coin_path, package_name))

    unreal.log(
        "SOTM Coin ID migration complete: total={} modified={} prior_duplicates={}".format(
            len(coins), len(modified), len(duplicates)
        )
    )
    return modified


if __name__ == "__main__":
    run()
