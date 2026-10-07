import unreal

SOURCE = "C:/ProgramData/Epic/EpicGamesLauncher/VaultCache/FabLibrary/Robot-3a36bb9e/glb/converted/robot.glb"
DESTINATION = "/Game/ReflectionLab/Art/Characters/Robot"

existing = unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True)
if not existing:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

meshes = []
animations = []
for path in unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True):
    asset = unreal.load_asset(path)
    if isinstance(asset, unreal.SkeletalMesh):
        meshes.append(asset)
        unreal.log("[RobotImport] SkeletalMesh=" + path)
    elif isinstance(asset, unreal.AnimSequence):
        animations.append(asset)
        unreal.log("[RobotImport] Animation={} duration={}".format(path, asset.get_play_length()))
if len(meshes) != 1 or len(animations) != 11:
    raise RuntimeError("Expected 1 skeletal mesh and 11 animations; got {} and {}".format(
        len(meshes), len(animations)))
for animation in animations:
    if animation.get_editor_property("skeleton") != meshes[0].get_editor_property("skeleton"):
        raise RuntimeError("Animation skeleton mismatch: " + animation.get_path_name())
unreal.log("[RobotImport] SUCCESS: mesh and 11 matching animation sequences")
