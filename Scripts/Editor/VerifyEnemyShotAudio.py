"""Read-only check of saved launch cues and mix limits."""
import unreal

for kind in ['Normal', 'Explosive', 'DelayedExplosive', 'Rally', 'Guard', 'Fake', 'ParrySplit']:
    data = unreal.load_asset('/Game/ReflectionLab/Data/Projectiles/DA_Projectile_' + kind)
    sound = data.get_editor_property('launch_sound')
    if not sound or sound.get_name() != 'SFX_EnemyShot':
        raise RuntimeError('Missing launch sound: ' + kind)
    if abs(data.get_editor_property('launch_sound_volume') - 0.18) > 0.001:
        raise RuntimeError('Incorrect volume: ' + kind)
    groups = list(sound.get_editor_property('concurrency_set'))
    if len(groups) != 1:
        raise RuntimeError('Missing concurrency')
    settings = groups[0].get_editor_property('concurrency')
    if settings.get_editor_property('max_count') != 2 or not settings.get_editor_property('limit_to_owner') or abs(settings.get_editor_property('retrigger_time') - 0.05) > 0.001:
        raise RuntimeError('Incorrect shot concurrency')
    if abs(data.get_editor_property('launch_sound_pitch') - 1.4) > 0.001:
        raise RuntimeError('Incorrect launch pitch')
boss = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBoss')
minion = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotMinion')
for data in [boss.get_editor_property('overload').get_editor_property('projectile_definition'),
             boss.get_editor_property('charge').get_editor_property('projectile_definition'),
             minion.get_editor_property('projectile_definition')]:
    if not data or not data.get_editor_property('launch_sound'):
        raise RuntimeError('Robot launch sound missing')
    unreal.log('ROBOT_SHOT_AUDIO_VERIFIED ' + data.get_path_name())
unreal.log('ENEMY_SHOT_AUDIO_VERIFIED pitch=1.4 max_per_shooter=2')
