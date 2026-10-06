import unreal


SOURCE = "C:/ProgramData/Epic/EpicGamesLauncher/VaultCache/FabLibrary/Baseball_Bat-3359cd97/glb/converted/baseball_bat.glb"
DESTINATION = "/Game/ReflectionLab/Art/Weapons/BaseballBat"

if unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
    existing = unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True)
    if existing:
        raise RuntimeError("Destination already contains assets; refusing to overwrite: " + DESTINATION)

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE)
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", False)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

meshes = []
for path in unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True):
    asset = unreal.load_asset(path)
    if isinstance(asset, unreal.StaticMesh):
        meshes.append(asset)
        unreal.log("[BatImport] Mesh={} bounds={} materials={}".format(
            path, asset.get_bounds(), len(asset.get_editor_property("static_materials"))))
    unreal.log("[BatImport] Asset=" + path)
if not meshes:
    raise RuntimeError("Import did not produce a static mesh")
unreal.log("[BatImport] SUCCESS: {} static mesh(es)".format(len(meshes)))
