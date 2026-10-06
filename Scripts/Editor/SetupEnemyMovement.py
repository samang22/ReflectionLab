import unreal


ASSET_PATH = "/Game/ReflectionLab/Data/Enemies/DA_EnemyMovement"
settings = unreal.load_asset(ASSET_PATH)
if not settings:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.RLEnemyMovementDataAsset)
    settings = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_EnemyMovement", "/Game/ReflectionLab/Data/Enemies",
        unreal.RLEnemyMovementDataAsset, factory)
if not settings:
    raise RuntimeError("Could not create enemy movement settings")
unreal.EditorAssetLibrary.save_loaded_asset(settings)

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for path in unreal.EditorAssetLibrary.list_assets("/Game/ReflectionLab", recursive=True):
    asset = unreal.load_asset(path)
    if not isinstance(asset, unreal.Blueprint):
        continue
    changed = False
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(asset):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, asset)
        if isinstance(component, unreal.RLEnemyMovementComponent):
            component.set_editor_property("movement_settings", settings)
            changed = True
    if changed:
        defaults = unreal.get_default_object(asset.generated_class())
        defaults.set_editor_property("ai_controller_class", unreal.RLEnemyAIController)
        defaults.set_editor_property("auto_possess_ai", unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        unreal.log("[EnemyMovement] Assigned settings: " + path)

unreal.log("[EnemyMovement] Settings ready. Main must have a built NavMesh covering the arena.")
