"""Enable source-bound shields on round 6 and replace round 7 refraction with a mirror arena."""
import unreal

lib = unreal.EditorAssetLibrary
root = '/Game/ReflectionLab/Data/Experiments'
run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
support = unreal.load_asset(root+'/DA_SupportNetwork')
material = unreal.load_asset('/Game/ReflectionLab/Art/Materials/World/M_LabWhiteTrim')
normal = unreal.load_asset('/Game/ReflectionLab/Data/Projectiles/DA_Projectile_Normal')
if not all([run,support,material,normal]):
    raise RuntimeError('Required support/mirror settings missing')
support.set_editor_property('support_grants_invulnerability',True)
support.set_editor_property('support_shield_color',unreal.LinearColor(r=0.65,g=0.15,b=1,a=1))
path = root+'/DA_MirrorArena'
mirror = unreal.load_asset(path) if lib.does_asset_exist(path) else None
if not mirror:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class',unreal.RLArenaExperimentDataAsset)
    mirror = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_MirrorArena',root,unreal.RLArenaExperimentDataAsset,factory)
if not mirror:
    raise RuntimeError('Cannot create mirror settings')
mirror.set_editor_property('type',unreal.RLArenaExperimentType.MIRROR_ARENA)
mirror.set_editor_property('display_name','MIRROR BALLISTICS')
mirror.set_editor_property('mirror_distance',1600.0)
mirror.set_editor_property('mirror_half_width',1800.0)
mirror.set_editor_property('lane_half_width',550.0)
mirror.set_editor_property('lane_half_length',300.0)
mirror.set_editor_property('lane_barrier_visual_height',45.0)
mirror.set_editor_property('max_mirror_bounces',4)
mirror.set_editor_property('shutter_material',material)
mirror.set_editor_property('projectile_definition',normal)
rounds = dict(run.get_editor_property('round_experiments'))
rounds[7] = mirror
run.set_editor_property('round_experiments',rounds)
# Eight spaced attackers keep the dedicated reflection lanes legible.
counts = dict(run.get_editor_property('round_enemy_counts'))
counts[7] = 8
run.set_editor_property('round_enemy_counts',counts)
for asset in [support,mirror,run]:
    if not lib.save_loaded_asset(asset,False):
        raise RuntimeError('Cannot save '+asset.get_path_name())
unreal.log('SUPPORT_AND_MIRROR_ASSIGNED round6=source-shields round7=mirror normal-only enemies=8')
