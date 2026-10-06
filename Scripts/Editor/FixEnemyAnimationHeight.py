import unreal


for name in ("A_Enemy_Idle", "A_Enemy_Shoot"):
    path = "/Game/ReflectionLab/Gameplay/Enemies/Animations/" + name
    clip = unreal.load_asset(path)
    if not isinstance(clip, unreal.AnimSequence):
        raise RuntimeError("Missing animation: " + path)
    clip.set_editor_property("force_root_lock", True)
    clip.set_editor_property("root_motion_root_lock", unreal.RootMotionRootLock.ANIM_FIRST_FRAME)
    if not unreal.EditorAssetLibrary.save_loaded_asset(clip, only_if_is_dirty=False):
        raise RuntimeError("Could not save: " + path)
    unreal.log("[EnemyHeight] Saved {} lock={}".format(
        path, clip.get_editor_property("root_motion_root_lock")))

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for name in ("BP_RLEnemyCharacter", "BP_RLEnemyBurstShooter"):
    path = "/Game/ReflectionLab/Gameplay/Enemies/" + name
    bp = unreal.load_asset(path)
    if not isinstance(bp, unreal.Blueprint):
        raise RuntimeError("Missing enemy blueprint: " + path)
    found = False
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
        if isinstance(component, unreal.SkeletalMeshComponent) and component.get_name() == "CharacterMesh0":
            location = component.get_editor_property("relative_location")
            component.set_editor_property("relative_location", unreal.Vector(location.x, location.y, -90.0))
            found = True
            break
    if not found:
        raise RuntimeError("Character mesh template not found: " + path)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError("Could not save: " + path)
    mesh = unreal.get_default_object(bp.generated_class()).get_component_by_class(unreal.SkeletalMeshComponent)
    unreal.log("[EnemyHeight] Saved {} mesh_location={}".format(
        path, mesh.get_editor_property("relative_location")))
