"""Read-only verification of persisted experiment assignments."""
import unreal

run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
if not run:
    raise RuntimeError('Default run missing')
rounds = dict(run.get_editor_property('round_experiments'))
for number, kind in [(3, unreal.RLArenaExperimentType.CROSSFIRE),
                     (6, unreal.RLArenaExperimentType.SUPPORT_NETWORK),
                     (8, unreal.RLArenaExperimentType.ALTERNATING_ZONES)]:
    data = rounds.get(number)
    if not data or data.get_editor_property('type') != kind:
        raise RuntimeError('Incorrect experiment at round ' + str(number))
bosses = dict(run.get_editor_property('round_enemy_classes'))
if not bosses.get(5):
    raise RuntimeError('Round five boss missing')
unreal.log('ARENA_VARIATIONS_VERIFIED rounds=3,6,8; boss=5; startingRound=' +
           str(run.get_editor_property('starting_round_number')))
