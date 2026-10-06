import unreal


for path in (
    "/Game/ReflectionLab/Gameplay/Enemies/BP_RLEnemyCharacter",
    "/Game/ReflectionLab/Gameplay/Player/BP_RLPlayerCharacter",
):
    bp = unreal.load_asset(path)
    if not bp:
        continue
    defaults = unreal.get_default_object(bp.generated_class())
    mesh = defaults.get_component_by_class(unreal.SkeletalMeshComponent)
    capsule = defaults.get_component_by_class(unreal.CapsuleComponent)
    unreal.log("[LocomotionInspect] {} mesh_location={} mesh_scale={} capsule_half_height={}".format(
        path, mesh.get_editor_property("relative_location"),
        mesh.get_editor_property("relative_scale3d"), capsule.get_unscaled_capsule_half_height()))

blend = unreal.load_asset("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run")
for path in (
    "/Game/ReflectionLab/Gameplay/Enemies/Animations/A_Enemy_Idle",
    "/Game/ReflectionLab/Gameplay/Enemies/Animations/A_Enemy_Shoot",
):
    clip = unreal.load_asset(path)
    for bone in ("root", "pelvis"):
        pose = unreal.AnimationLibrary.get_bone_pose_for_time(clip, bone, 0.0, False)
        unreal.log("[LocomotionInspect] {} {}={} lock={} mode={}".format(
            path, bone, pose.translation, clip.get_editor_property("force_root_lock"),
            clip.get_editor_property("root_motion_root_lock")))
for index in range(2):
    axis = blend.get_editor_property("blend_parameters")[index]
    unreal.log("[LocomotionInspect] axis{}={}".format(index, axis))
for sample in blend.get_editor_property("sample_data"):
    clip = sample.get_editor_property("animation")
    if not clip:
        continue
    root = unreal.AnimationLibrary.get_bone_pose_for_time(clip, "root", 0.0, False)
    unreal.log("[LocomotionInspect] {} sample={} root={} force_root_lock={}".format(
        clip.get_path_name(), sample.get_editor_property("sample_value"),
        root.translation, clip.get_editor_property("force_root_lock")))
