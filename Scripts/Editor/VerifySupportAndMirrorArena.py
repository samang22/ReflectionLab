"""Read-only verification of the new round six/seven configuration."""
import unreal

run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
rounds = dict(run.get_editor_property('round_experiments'))
if not rounds[6].get_editor_property('support_grants_invulnerability'):
    raise RuntimeError('Source shield not enabled')
if rounds[7].get_editor_property('type') != unreal.RLArenaExperimentType.MIRROR_ARENA:
    raise RuntimeError('Round seven still using legacy refraction')
if not rounds[7].get_editor_property('projectile_definition'):
    raise RuntimeError('Mirror encounter needs authored basic projectile')
if abs(rounds[7].get_editor_property('lane_barrier_visual_height')-45.0)>0.001 or rounds[7].get_editor_property('max_mirror_bounces')!=4:
    raise RuntimeError('Mirror readability tuning missing')
if dict(run.get_editor_property('round_enemy_counts')).get(7) != 8:
    raise RuntimeError('Mirror attacker count incorrect')
for number in [3,4,8]:
    if not rounds.get(number):
        raise RuntimeError('Previous experiment lost')
unreal.log('SUPPORT_AND_MIRROR_VERIFIED round6=source-shields round7=mirror normal-only enemies=8; start='+str(run.get_editor_property('starting_round_number')))
