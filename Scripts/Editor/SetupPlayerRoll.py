import unreal

source = unreal.load_asset('/Game/ReflectionLab/Art/Characters/Mixamo/Source/Sprinting_Forward_Roll')
source_mesh = unreal.load_asset('/Game/ReflectionLab/Art/Characters/Mixamo/Source/T-Pose')
target_mesh = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
retargeter = unreal.load_asset('/Game/ReflectionLab/Gameplay/Enemies/Animations/RTG_Enemy_MixamoToManny')
destination = '/Game/ReflectionLab/Gameplay/Player/Animations'
if not all((source, source_mesh, target_mesh, retargeter)):
    raise RuntimeError('Roll animation or retargeting assets missing')
path = destination + '/A_Player_Roll'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    if source.get_editor_property('skeleton') != source_mesh.get_editor_property('skeleton'):
        raise RuntimeError('Roll source skeleton does not match Mixamo rig')
    inputs = unreal.IKRetargetBatchOperationInputs()
    inputs.set_editor_property('assets_to_retarget', [unreal.EditorAssetLibrary.find_asset_data(source.get_path_name())])
    inputs.set_editor_property('source_mesh', source_mesh)
    inputs.set_editor_property('target_mesh', target_mesh)
    inputs.set_editor_property('ik_retarget_asset', retargeter)
    inputs.set_editor_property('target_path', destination)
    inputs.set_editor_property('prefix', 'A_Player_')
    inputs.set_editor_property('search', source.get_name())
    inputs.set_editor_property('replace', 'Roll')
    inputs.set_editor_property('include_referenced_assets', False)
    if len(unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)) != 1:
        raise RuntimeError('Roll retarget failed')
clip = unreal.load_asset(path)
# Prefer the user's manually retargeted animation, without altering it.
base_clip = unreal.load_asset(destination + '/Sprinting_Forward_Roll') or clip
root_motion_path = destination + '/A_Player_Roll_RootMotion'
root_clip = unreal.load_asset(root_motion_path)
if not root_clip:
    root_clip = unreal.EditorAssetLibrary.duplicate_asset(base_clip.get_path_name(), root_motion_path)
if not root_clip:
    raise RuntimeError('Root-motion clip creation failed')

# Mixamo's root carries the body's somersault. Keep a planar, non-rotating
# locomotion root and compensate pelvis transforms to preserve the animated pose.
model = base_clip.get_editor_property('data_model_interface')
key_count = model.get_number_of_keys()
length = unreal.AnimationLibrary.get_sequence_length(base_clip)
if key_count < 2 or length <= 0.0:
    raise RuntimeError('Roll animation has insufficient keys or duration')
root_positions, root_rotations, root_scales = [], [], []
pelvis_positions, pelvis_rotations, pelvis_scales = [], [], []
initial_root = unreal.AnimationLibrary.get_bone_pose_for_time(base_clip, 'root', 0.0, False)
for index in range(key_count):
    time = length * index / (key_count - 1)
    root = unreal.AnimationLibrary.get_bone_pose_for_time(base_clip, 'root', time, False)
    pelvis = unreal.AnimationLibrary.get_bone_pose_for_time(base_clip, 'pelvis', time, False)
    position = root.translation
    # Manny's mesh forward axis is +Y; lateral sway belongs to the body, not the capsule.
    planar_root = unreal.Transform(location=unreal.Vector(initial_root.translation.x, position.y, 0.0))
    original_pelvis = unreal.MathLibrary.compose_transforms(pelvis, root)
    adjusted_pelvis = unreal.MathLibrary.make_relative_transform(original_pelvis, planar_root)
    root_positions.append(planar_root.translation)
    root_rotations.append(planar_root.rotation)
    root_scales.append(planar_root.scale3d)
    pelvis_positions.append(adjusted_pelvis.translation)
    pelvis_rotations.append(adjusted_pelvis.rotation)
    pelvis_scales.append(adjusted_pelvis.scale3d)

controller = root_clip.get_editor_property('controller')
controller.open_bracket('Prepare player roll root motion', False)
try:
    if not controller.set_bone_track_keys('root', root_positions, root_rotations, root_scales, False):
        raise RuntimeError('Root track update failed')
    if not controller.set_bone_track_keys('pelvis', pelvis_positions, pelvis_rotations, pelvis_scales, False):
        raise RuntimeError('Pelvis track update failed')
finally:
    controller.close_bracket(False)
root_clip.set_editor_property('enable_root_motion', True)
root_clip.set_editor_property('force_root_lock', False)
root_clip.set_editor_property('root_motion_root_lock', unreal.RootMotionRootLock.ANIM_FIRST_FRAME)
if not unreal.EditorAssetLibrary.save_loaded_asset(root_clip):
    raise RuntimeError('Root-motion clip save failed')

for index in (0, key_count // 2, key_count - 1):
    time = length * index / (key_count - 1)
    original_root = unreal.AnimationLibrary.get_bone_pose_for_time(base_clip, 'root', time, False)
    original_pelvis = unreal.AnimationLibrary.get_bone_pose_for_time(base_clip, 'pelvis', time, False)
    updated_root = unreal.AnimationLibrary.get_bone_pose_for_time(root_clip, 'root', time, False)
    updated_pelvis = unreal.AnimationLibrary.get_bone_pose_for_time(root_clip, 'pelvis', time, False)
    original_pose = unreal.MathLibrary.compose_transforms(original_pelvis, original_root)
    updated_pose = unreal.MathLibrary.compose_transforms(updated_pelvis, updated_root)
    if unreal.MathLibrary.vector_distance(original_pose.translation, updated_pose.translation) > 0.25:
        raise RuntimeError('Root-motion conversion changed the pelvis position')
    a, b = original_pose.rotation, updated_pose.rotation
    if abs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w) < 0.9999:
        raise RuntimeError('Root-motion conversion changed the pelvis rotation')
unreal.log('PLAYER_ROLL_POSE_PRESERVED')
montage_path = destination + '/AM_Player_Roll'
montage = unreal.load_asset(montage_path)
if not montage:
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property('target_skeleton', root_clip.get_editor_property('skeleton'))
    factory.set_editor_property('source_animation', root_clip)
    montage = unreal.AssetToolsHelpers.get_asset_tools().create_asset('AM_Player_Roll', destination, unreal.AnimMontage, factory)
if not montage:
    raise RuntimeError('Roll montage creation failed')
tracks = montage.get_editor_property('slot_anim_tracks')
if len(tracks) != 1:
    raise RuntimeError('Expected one roll montage slot track')
track = tracks[0].get_editor_property('anim_track')
segments = track.get_editor_property('anim_segments')
if len(segments) != 1:
    raise RuntimeError('Expected one roll montage segment')
segment = segments[0]
segment.set_editor_property('anim_reference', root_clip)
segment.set_editor_property('anim_start_time', 0.0)
segment.set_editor_property('anim_end_time', length)
segment.set_editor_property('anim_play_rate', 1.0)
segments[0] = segment
track.set_editor_property('anim_segments', segments)
slot = tracks[0]
slot.set_editor_property('anim_track', track)
tracks[0] = slot
montage.set_editor_property('slot_anim_tracks', tracks)
montage.set_editor_property('rate_scale', 1.0)
if not unreal.EditorAssetLibrary.save_loaded_asset(montage, False):
    raise RuntimeError('Roll montage save failed')
bp = unreal.load_asset(destination + '/ABP_RLPlayerCharacter')
defaults = unreal.get_default_object(bp.generated_class())
defaults.set_editor_property('root_motion_mode', unreal.RootMotionMode.ROOT_MOTION_FROM_MONTAGES_ONLY)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
unreal.log('PLAYER_ROLL_ROOT_MOTION_READY ' + montage.get_path_name() + ' length=' + str(length))
