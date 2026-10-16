"""Register acceleration on round 4 and refraction on round 7; preserve other experiments."""
import unreal

lib = unreal.EditorAssetLibrary
root = '/Game/ReflectionLab/Data/Experiments'
run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
material = unreal.load_asset('/Game/ReflectionLab/Art/Materials/World/M_LabWhiteTrim')
if not run or not material:
    raise RuntimeError('Missing run or field material')
rounds = dict(run.get_editor_property('round_experiments'))
for number, name, kind, label, centers, extent in [
    (4, 'DA_AccelerationField', unreal.RLArenaExperimentType.ACCELERATION_FIELD, 'BALLISTIC ACCELERATION',
     [unreal.Vector2D(x=-650, y=0), unreal.Vector2D(x=650, y=0)], unreal.Vector2D(x=180, y=1100)),
    (7, 'DA_RefractionGate', unreal.RLArenaExperimentType.REFRACTION_GATE, 'BALLISTIC REFRACTION',
     [unreal.Vector2D(x=0, y=0)], unreal.Vector2D(x=220, y=600))]:
    path = root + '/' + name
    data = unreal.load_asset(path) if lib.does_asset_exist(path) else None
    if not data:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.RLArenaExperimentDataAsset)
        data = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, root, unreal.RLArenaExperimentDataAsset, factory)
    if not data:
        raise RuntimeError('Cannot create ' + name)
    data.set_editor_property('type', kind)
    data.set_editor_property('display_name', label)
    data.set_editor_property('field_centers', centers)
    data.set_editor_property('field_half_extent', extent)
    data.set_editor_property('field_speed_multiplier', 1.5)
    data.set_editor_property('refraction_angle_degrees', 35.0)
    data.set_editor_property('shutter_material', material)
    if not lib.save_loaded_asset(data, False):
        raise RuntimeError('Cannot save ' + name)
    # The newer mirror arena replaces round seven; replaying this older setup
    # must not silently restore the retired default refraction encounter.
    if number != 7 or not rounds.get(7) or rounds[7].get_editor_property('type') != unreal.RLArenaExperimentType.MIRROR_ARENA:
        rounds[number] = data
run.set_editor_property('round_experiments', rounds)
if not lib.save_loaded_asset(run, False):
    raise RuntimeError('Cannot save run')
unreal.log('BALLISTICS_EXPERIMENTS_ASSIGNED acceleration=4 refraction=7')
