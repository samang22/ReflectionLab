"""Adjust only the existing mirror arena's visual wall height and ricochet budget."""
import unreal

lib = unreal.EditorAssetLibrary
data = unreal.load_asset('/Game/ReflectionLab/Data/Experiments/DA_MirrorArena')
if not data:
    raise RuntimeError('Mirror arena data missing')
data.set_editor_property('lane_barrier_visual_height',45.0)
data.set_editor_property('max_mirror_bounces',4)
if not lib.save_loaded_asset(data,False):
    raise RuntimeError('Could not save mirror tuning')
unreal.log('MIRROR_READABILITY_SAVED visibleHeight=45 collision=unchanged mirrors=4 maxBounces=4')
