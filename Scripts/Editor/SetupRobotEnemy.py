import unreal

# Run after rebuilding and restarting the editor. Existing enemy BPs are untouched.
BASE = "/Game/ReflectionLab/Gameplay/Enemies/BP_RLEnemyCharacter"
DEST = "/Game/ReflectionLab/Gameplay/Enemies/BP_RLRobotEnemy"
MESH = "/Game/ReflectionLab/Art/Characters/Robot/robot/SkeletalMeshes/robot"
robot_anim_class = unreal.RLRobotEnemyAnimInstance
mesh = unreal.load_asset(MESH)
parent = unreal.EditorAssetLibrary.load_blueprint_class(BASE)
if not mesh or not parent:
    raise RuntimeError("Imported robot mesh or base enemy BP missing")
asset = unreal.load_asset(DEST)
if asset is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_RLRobotEnemy", "/Game/ReflectionLab/Gameplay/Enemies", unreal.Blueprint, factory)
if not isinstance(asset, unreal.Blueprint):
    raise RuntimeError("Could not create robot enemy BP")
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
configured = False
for handle in subsystem.k2_gather_subobject_data_for_blueprint(asset):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, asset)
    if isinstance(component, unreal.SkeletalMeshComponent):
        component.set_skeletal_mesh_asset(mesh)
        component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
        component.set_anim_instance_class(robot_anim_class)
        component.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
        component.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))
        component.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -88.0))
        configured = True
if not configured:
    raise RuntimeError("Robot enemy skeletal mesh component missing")
unreal.BlueprintEditorLibrary.compile_blueprint(asset)
defaults = unreal.get_default_object(asset.generated_class())
configured_mesh = defaults.get_component_by_class(unreal.SkeletalMeshComponent)
if configured_mesh.get_editor_property("anim_class") != robot_anim_class.static_class():
    raise RuntimeError("Robot animation class assignment was not retained")
if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
    raise RuntimeError("Could not save robot enemy BP")
unreal.log("[RobotSetup] SUCCESS: BP_RLRobotEnemy uses robot Idle/Walk/Shoot. Existing waves unchanged.")
