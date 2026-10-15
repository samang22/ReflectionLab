"""Enable the first crossfire/shutter experiment on displayed round three only."""
import unreal

lib = unreal.EditorAssetLibrary
root = '/Game/ReflectionLab/Data/Experiments'
path = root + '/DA_CrossfireExperiment'
lib.make_directory(root)
data = unreal.load_asset(path) if lib.does_asset_exist(path) else None
if not data:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.RLArenaExperimentDataAsset)
    data = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_CrossfireExperiment', root, unreal.RLArenaExperimentDataAsset, factory)
if not data:
    raise RuntimeError('Could not create experiment settings')
data.set_editor_property('phase_duration', 6.0)
data.set_editor_property('attack_frequency_multiplier', 1.5)
data.set_editor_property('warning_duration', 1.0)
data.set_editor_property('shutter_move_duration', 0.4)
data.set_editor_property('team_distance', 1200.0)
data.set_editor_property('team_spread', 900.0)
data.set_editor_property('shutter_distance', 500.0)
data.set_editor_property('shutter_size', unreal.Vector(x=70, y=700, z=180))
material = unreal.load_asset('/Game/ReflectionLab/Art/Materials/World/M_LabWhiteTrim')
projectile = unreal.load_asset('/Game/ReflectionLab/Data/Projectiles/DA_Projectile_Normal')
run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
if not all([material, projectile, run]):
    raise RuntimeError('Required experiment assets missing')
data.set_editor_property('shutter_material', material)
data.set_editor_property('projectile_definition', projectile)
rounds = dict(run.get_editor_property('round_experiments'))
rounds[3] = data
run.set_editor_property('round_experiments', rounds)
for asset in [data, run]:
    if not lib.save_loaded_asset(asset, False):
        raise RuntimeError('Could not save ' + asset.get_path_name())
unreal.log('CROSSFIRE_EXPERIMENT_ASSIGNED round=3; starting round override unchanged')
