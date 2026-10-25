"""Update only round seven's mirror layout; preserve run/debug settings."""
import unreal

asset = unreal.load_asset('/Game/ReflectionLab/Data/Experiments/DA_MirrorArena')
if not asset:
    raise RuntimeError('DA_MirrorArena is missing')
asset.set_editor_property('diagonal_mirror_half_length', 650.0)
asset.set_editor_property('diagonal_mirror_center', unreal.Vector2D(950.0, 800.0))
asset.set_editor_property('mirror_camera_arm_length', 4300.0)
if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
    raise RuntimeError('Cannot save DA_MirrorArena')
unreal.log('DIAGONAL_MIRROR_READY: four diagonal mirrors, centered camera arm=4300')
