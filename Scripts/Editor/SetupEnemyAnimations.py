"""Retarget the imported Mixamo Idle/Gunplay clips to the enemy Manny mesh."""

import unreal


SOURCE_PATH = "/Game/ReflectionLab/Art/Characters/Mixamo/Source"
DESTINATION_PATH = "/Game/ReflectionLab/Gameplay/Enemies/Animations"


def require_asset(path, asset_type):
    asset = unreal.load_asset(path)
    if not isinstance(asset, asset_type):
        raise RuntimeError(f"Missing {asset_type.__name__}: {path}")
    return asset


def create_asset(name, asset_type, factory):
    path = f"{DESTINATION_PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return require_asset(path, asset_type)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, DESTINATION_PATH, asset_type, factory
    )
    if not asset:
        raise RuntimeError(f"Could not create {path}")
    return asset


def setup_enemy_animations():
    source_mesh = require_asset(f"{SOURCE_PATH}/T-Pose", unreal.SkeletalMesh)
    target_mesh = require_asset(
        "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple", unreal.SkeletalMesh
    )
    source_skeleton = source_mesh.get_editor_property("skeleton")
    target_skeleton = target_mesh.get_editor_property("skeleton")
    clips = [require_asset(f"{SOURCE_PATH}/{name}", unreal.AnimSequence)
             for name in ("Idle", "Gunplay")]
    for clip in clips:
        if clip.get_editor_property("skeleton") != source_skeleton:
            raise RuntimeError(f"Source skeleton mismatch: {clip.get_path_name()}")

    rigs = []
    for name, mesh in (("IK_Enemy_Mixamo", source_mesh), ("IK_Enemy_Manny", target_mesh)):
        rig = create_asset(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
        controller = unreal.IKRigController.get_controller(rig)
        if not controller.set_skeletal_mesh(mesh):
            raise RuntimeError(f"Could not assign rig mesh: {name}")
        if not controller.apply_auto_generated_retarget_definition():
            raise RuntimeError(f"Could not generate retarget chains: {name}")
        unreal.EditorAssetLibrary.save_loaded_asset(rig)
        rigs.append(rig)

    retargeter = create_asset(
        "RTG_Enemy_MixamoToManny", unreal.IKRetargeter, unreal.IKRetargetFactory()
    )
    controller = unreal.IKRetargeterController.get_controller(retargeter)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rigs[0])
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rigs[1])
    if controller.get_num_retarget_ops() == 0:
        controller.add_default_ops()
    controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    controller.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    unreal.EditorAssetLibrary.save_loaded_asset(retargeter)

    for source, destination in zip(clips, ("A_Enemy_Idle", "A_Enemy_Shoot")):
        path = f"{DESTINATION_PATH}/{destination}"
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            inputs = unreal.IKRetargetBatchOperationInputs()
            inputs.set_editor_property("assets_to_retarget", [
                unreal.EditorAssetLibrary.find_asset_data(source.get_path_name())
            ])
            inputs.set_editor_property("source_mesh", source_mesh)
            inputs.set_editor_property("target_mesh", target_mesh)
            inputs.set_editor_property("ik_retarget_asset", retargeter)
            inputs.set_editor_property("target_path", DESTINATION_PATH)
            inputs.set_editor_property("prefix", "A_Enemy_")
            inputs.set_editor_property("search", source.get_name())
            inputs.set_editor_property("replace", destination.removeprefix("A_Enemy_"))
            inputs.set_editor_property("include_referenced_assets", False)
            created = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
            if len(created) != 1:
                raise RuntimeError(f"Retarget failed: {source.get_name()}")
        clip = require_asset(path, unreal.AnimSequence)
        if clip.get_editor_property("skeleton") != target_skeleton:
            raise RuntimeError(f"Target skeleton mismatch: {path}")
        clip.set_editor_property("enable_root_motion", False)
        clip.set_editor_property("force_root_lock", True)
        unreal.EditorAssetLibrary.save_loaded_asset(clip, only_if_is_dirty=False)
        unreal.log(f"ENEMY_ANIMATION_READY {clip.get_path_name()}")


if __name__ == "__main__":
    setup_enemy_animations()
