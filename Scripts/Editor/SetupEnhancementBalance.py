"""Apply the first enhancement/health balance pass without changing attack timing."""
import unreal

stats = unreal.load_asset('/Game/ReflectionLab/Data/Player/DA_PlayerStats')
run = unreal.load_asset('/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun')
boss = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBoss')
if not all([stats, run, boss]):
    raise RuntimeError('Required balance assets are missing')
stats.set_editor_property('enhancement_damage_multipliers', [1.0, 2.0, 3.0, 5.0])
mixes = {}
for number in range(1, 11):
    mix = unreal.RLEnemyHealthMix()
    mix.set_editor_property('normal_health', 2.0)
    mix.set_editor_property('reinforced_health', 4.0)
    mix.set_editor_property('heavy_health', 5.0)
    reinforced, heavy = {
        3: (0.25, 0.0), 4: (0.375, 0.0),
        6: (0.5, 0.0), 7: (0.5, 0.0),
        8: (0.4, 0.2), 9: (0.4, 0.3), 10: (0.4, 0.3),
    }.get(number, (0.0, 0.0))
    mix.set_editor_property('reinforced_ratio', reinforced)
    mix.set_editor_property('heavy_ratio', heavy)
    mixes[number] = mix
run.set_editor_property('round_enemy_health_mixes', mixes)
boss.get_editor_property('overload').set_editor_property('max_health', 40.0)
for asset in [stats, run, boss]:
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Failed to save {}'.format(asset.get_path_name()))
unreal.log('ENHANCEMENT_BALANCE_APPLIED')
