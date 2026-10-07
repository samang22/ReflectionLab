import unreal

ROOT = "/Game/ReflectionLab/Art/Characters/Robot/robot/SkeletalMeshes/"
options = unreal.AnimPoseEvaluationOptions()


def values(transform):
    location = transform.translation
    rotation = transform.rotation
    scale = transform.scale3d
    return (location.x, location.y, location.z, rotation.x, rotation.y, rotation.z,
            rotation.w, scale.x, scale.y, scale.z)

for name in ("robotiddle", "robotwalking", "robotattackminiguns"):
    animation = unreal.load_asset(ROOT + name)
    if not isinstance(animation, unreal.AnimSequence):
        raise RuntimeError("Animation missing: " + name)
    poses = [unreal.AnimPoseExtensions.get_anim_pose_at_time(
        animation, animation.get_play_length() * fraction, options)
        for fraction in (0.1, 0.35, 0.7)]
    bones = unreal.AnimPoseExtensions.get_bone_names(poses[0])
    changed = []
    for bone in bones:
        transforms = [unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.LOCAL)
                      for pose in poses]
        if any(any(abs(a - b) > 0.001 for a, b in zip(values(transforms[0]), values(transform)))
               for transform in transforms[1:]):
            changed.append(str(bone))
    if not changed:
        raise RuntimeError("No animated pose changes detected: " + name)
    unreal.log("[RobotPose] {} evaluated; moving bones={}".format(name, changed))
unreal.log("[RobotPose] SUCCESS: Idle/Walk/Shoot produce changing bone poses")
