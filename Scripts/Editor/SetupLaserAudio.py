"""Import laser cues and assign both boss-inline and minion laser settings."""
from pathlib import Path
import unreal

lib = unreal.EditorAssetLibrary
root = '/Game/ReflectionLab/Audio/SFX/Combat/Laser'
sounds = []
for name in ['LaserCharge', 'LaserFire']:
    source = Path(unreal.Paths.project_dir()) / 'SourceArt/Audio/SFX/Laser' / (name + '.wav')
    if not source.is_file():
        raise RuntimeError('Missing ' + str(source))
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source.resolve()))
    task.set_editor_property('destination_path', root)
    task.set_editor_property('destination_name', 'SFX_' + name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(root + '/SFX_' + name)
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError('Import failed: ' + name)
    path = root + '/SC_' + name
    concurrency = unreal.load_asset(path) if lib.does_asset_exist(path) else None
    if not concurrency:
        concurrency = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            'SC_' + name, root, unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
    settings = concurrency.get_editor_property('concurrency')
    settings.set_editor_property('max_count', 3)
    settings.set_editor_property('resolution_rule', unreal.MaxConcurrentResolutionRule.PREVENT_NEW)
    concurrency.set_editor_property('concurrency', settings)
    sound.set_editor_property('override_concurrency', False)
    sound.set_editor_property('concurrency_set', [concurrency])
    for asset in [sound, concurrency]:
        if not lib.save_loaded_asset(asset, False):
            raise RuntimeError('Cannot save audio')
    sounds.append(sound)

boss = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBoss')
minion = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotMinionLaser')
if not boss or not minion:
    raise RuntimeError('Laser settings missing')
for settings in [boss.get_editor_property('laser'), minion]:
    settings.set_editor_property('charge_sound', sounds[0])
    settings.set_editor_property('fire_sound', sounds[1])
    settings.set_editor_property('charge_volume', 0.45)
    settings.set_editor_property('fire_volume', 0.6)
for asset in [boss, minion]:
    if not lib.save_loaded_asset(asset, False):
        raise RuntimeError('Cannot save laser settings')
unreal.log('LASER_AUDIO_ASSIGNED boss and minion; charge/fire separate; concurrency=3 each')
