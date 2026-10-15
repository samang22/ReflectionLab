"""Add independently tuned support and zone experiments without changing boss/crossfire rounds."""
import unreal

lib = unreal.EditorAssetLibrary
root = '/Game/ReflectionLab/Data/Experiments'
run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
robot = unreal.load_class(None, '/Game/ReflectionLab/Gameplay/Enemies/BP_RLRobotMinion.BP_RLRobotMinion_C')
material = unreal.load_asset('/Game/ReflectionLab/Art/Materials/World/M_LabWhiteTrim')
if not all([run, robot, material]):
    raise RuntimeError('Required run, robot, or material missing')
rounds = dict(run.get_editor_property('round_experiments'))
for number, name, kind, label in [
    (6, 'DA_SupportNetwork', unreal.RLArenaExperimentType.SUPPORT_NETWORK, 'SUPPORT NETWORK'),
    (8, 'DA_AlternatingZones', unreal.RLArenaExperimentType.ALTERNATING_ZONES, 'ALTERNATING SECTORS')]:
    path = root + '/' + name
    data = unreal.load_asset(path)
    if not data:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.RLArenaExperimentDataAsset)
        data = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, root, unreal.RLArenaExperimentDataAsset, factory)
    if not data:
        raise RuntimeError('Cannot create ' + path)
    data.set_editor_property('type', kind)
    data.set_editor_property('display_name', label)
    data.set_editor_property('phase_duration', 8.0)
    data.set_editor_property('warning_duration', 2.5)
    data.set_editor_property('support_enemy_class', robot)
    data.set_editor_property('support_radius', 1000.0)
    data.set_editor_property('support_attack_rate', 1.5)
    data.set_editor_property('shutter_material', material)
    if not lib.save_loaded_asset(data, False):
        raise RuntimeError('Cannot save ' + path)
    rounds[number] = data
run.set_editor_property('round_experiments', rounds)
if not lib.save_loaded_asset(run, False):
    raise RuntimeError('Cannot save run')
unreal.log('ARENA_VARIATIONS_ASSIGNED support=6 zones=8; crossfire and boss unchanged')
