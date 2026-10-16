"""Read-only reload verification; no test-start override changes."""
import unreal

run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
rounds = dict(run.get_editor_property('round_experiments'))
for number, kind in [(4, unreal.RLArenaExperimentType.ACCELERATION_FIELD),
                     (7, unreal.RLArenaExperimentType.MIRROR_ARENA)]:
    data = rounds.get(number)
    if not data or data.get_editor_property('type') != kind:
        raise RuntimeError('Wrong ballistics round ' + str(number))
    if abs(data.get_editor_property('field_speed_multiplier') - 1.5) > 0.001 or abs(data.get_editor_property('refraction_angle_degrees') - 35) > 0.001:
        raise RuntimeError('Wrong ballistics tuning')
    if not data.get_editor_property('field_centers'):
        raise RuntimeError('Missing field placement')
for number in [3, 6, 8]:
    if not rounds.get(number):
        raise RuntimeError('Existing experiment lost: ' + str(number))
if not dict(run.get_editor_property('round_enemy_classes')).get(5):
    raise RuntimeError('Boss assignment lost')
unreal.log('BALLISTICS_EXPERIMENTS_VERIFIED acceleration=4 speed=1.5 mirror=7; startingRound=' + str(run.get_editor_property('starting_round_number')))
