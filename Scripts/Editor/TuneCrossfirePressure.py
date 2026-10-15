"""Change only crossfire firing frequency; preserve layout and other rounds."""
import unreal

run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
if not run:
    raise RuntimeError('Default run missing')
data = dict(run.get_editor_property('round_experiments')).get(3)
if not data or data.get_editor_property('type') != unreal.RLArenaExperimentType.CROSSFIRE:
    raise RuntimeError('Round three crossfire assignment missing')
data.set_editor_property('attack_frequency_multiplier', 1.5)
if not unreal.EditorAssetLibrary.save_loaded_asset(data, False):
    raise RuntimeError('Cannot save crossfire pressure')
unreal.log('CROSSFIRE_PRESSURE_SAVED round=3 frequency=1.5; layout and enemy count unchanged')
