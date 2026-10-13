"""Skip 80ms of anticipation; blend in faster without changing notify timing/duration."""
import unreal

lib = unreal.EditorAssetLibrary
stats = lib.load_asset('/Game/ReflectionLab/Data/Player/DA_PlayerStats')
if not stats:
    raise RuntimeError('Player stats missing')
stats.get_editor_property('parry_animation_start_time')
paths = ['/Game/ReflectionLab/Gameplay/Player/Animations/AM_Parry',
         '/Game/ReflectionLab/Gameplay/Player/Animations/AM_Parry_Mirrored']
montages = [lib.load_asset(path) for path in paths]
if not all(isinstance(montage, unreal.AnimMontage) for montage in montages):
    raise RuntimeError('Parry montages missing')
for montage in montages:
    # Keep all animation segments and notify-state begin/end times untouched.
    blend = montage.get_editor_property('blend_in')
    blend.set_editor_property('blend_time', 0.03)
    montage.set_editor_property('blend_in', blend)
    if not lib.save_loaded_asset(montage):
        raise RuntimeError('Could not save montage: ' + montage.get_path_name())
    if abs(montage.get_editor_property('blend_in').get_editor_property('blend_time') - 0.03) > 0.0001:
        raise RuntimeError('Blend-in mismatch')
stats.set_editor_property('parry_animation_start_time', 0.08)
if not lib.save_loaded_asset(stats):
    raise RuntimeError('Could not save player stats')
unreal.log('SHORTEN_PARRY_SUCCESS: start offset 0.08s; blend 0.03s; notify window unchanged')
