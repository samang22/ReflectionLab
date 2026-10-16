"""Import and wire enemy-only launch feedback, independent of reflection and impact audio."""
from pathlib import Path
import unreal

lib = unreal.EditorAssetLibrary
root = '/Game/ReflectionLab/Audio/SFX/Combat/Projectile'
task = unreal.AssetImportTask()
task.set_editor_property('filename', str((Path(unreal.Paths.project_dir()) / 'SourceArt/Audio/SFX/EnemyShot.wav').resolve()))
task.set_editor_property('destination_path', root)
task.set_editor_property('destination_name', 'SFX_EnemyShot')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = unreal.load_asset(root + '/SFX_EnemyShot')
if not isinstance(sound, unreal.SoundWave):
    raise RuntimeError('Enemy shot import failed')
path = root + '/SC_EnemyShot'
group = unreal.load_asset(path) if lib.does_asset_exist(path) else None
if not group:
    group = unreal.AssetToolsHelpers.get_asset_tools().create_asset('SC_EnemyShot', root, unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
settings = group.get_editor_property('concurrency')
settings.set_editor_property('max_count', 2)
settings.set_editor_property('limit_to_owner', True)
settings.set_editor_property('retrigger_time', 0.05)
settings.set_editor_property('resolution_rule', unreal.MaxConcurrentResolutionRule.PREVENT_NEW)
group.set_editor_property('concurrency', settings)
sound.set_editor_property('override_concurrency', False)
sound.set_editor_property('concurrency_set', [group])
for asset in [sound, group]:
    if not lib.save_loaded_asset(asset, False):
        raise RuntimeError('Cannot save shot audio')
for kind in ['Normal', 'Explosive', 'DelayedExplosive', 'Rally', 'Guard', 'Fake', 'ParrySplit']:
    data = unreal.load_asset('/Game/ReflectionLab/Data/Projectiles/DA_Projectile_' + kind)
    if not data:
        raise RuntimeError('Missing projectile ' + kind)
    data.set_editor_property('launch_sound', sound)
    data.set_editor_property('launch_sound_volume', 0.18)
    data.set_editor_property('launch_sound_pitch', 1.4)
    if not lib.save_loaded_asset(data, False):
        raise RuntimeError('Cannot save ' + kind)
boss = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBoss')
minion = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotMinion')
if not boss or not minion:
    raise RuntimeError('Robot settings missing')
for data in [boss.get_editor_property('overload').get_editor_property('projectile_definition'),
             boss.get_editor_property('charge').get_editor_property('projectile_definition'),
             minion.get_editor_property('projectile_definition')]:
    if not data:
        raise RuntimeError('Robot projectile missing')
    data.set_editor_property('launch_sound', sound)
    data.set_editor_property('launch_sound_volume', 0.18)
    data.set_editor_property('launch_sound_pitch', 1.4)
    if not lib.save_loaded_asset(data, False):
        raise RuntimeError('Cannot save robot projectile')
    unreal.log('ROBOT_SHOT_AUDIO_ASSIGNED ' + data.get_path_name())
unreal.log('ENEMY_SHOT_AUDIO_ASSIGNED pitch=1.4 volume=0.18 max_per_shooter=2 retrigger=0.05')
