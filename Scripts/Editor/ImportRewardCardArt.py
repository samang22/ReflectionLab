from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
SOURCE_DIRECTORY = PROJECT_ROOT / "SourceArt" / "RewardCards"
DESTINATION_PATH = "/Game/ReflectionLab/UI/Rewards/Textures"


def import_reward_card_art() -> None:
    source_files = sorted(SOURCE_DIRECTORY.glob("T_Reward_*.jpg"))
    if not source_files:
        raise RuntimeError(f"No reward card art found in {SOURCE_DIRECTORY}")

    tasks: list[unreal.AssetImportTask] = []
    for source_file in source_files:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source_file))
        task.set_editor_property("destination_path", DESTINATION_PATH)
        task.set_editor_property("destination_name", source_file.stem)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("replace_existing_settings", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    imported_assets: list[str] = []
    for task in tasks:
        imported_assets.extend(task.get_editor_property("imported_object_paths"))

    if len(imported_assets) != len(source_files):
        raise RuntimeError(
            f"Imported {len(imported_assets)} of {len(source_files)} reward card textures"
        )

    for asset_path in imported_assets:
        texture = unreal.load_asset(asset_path)
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Imported asset is not a Texture2D: {asset_path}")

        texture.set_editor_property("srgb", True)
        texture.set_editor_property("never_stream", True)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
        unreal.log(f"Reward card texture ready: {asset_path}")


if __name__ == "__main__":
    import_reward_card_art()
