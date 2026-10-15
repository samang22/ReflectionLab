"""Import the low projectile pop and assign pool-return feedback on each projectile definition."""
from pathlib import Path
import unreal

root = '/Game/ReflectionLab/Audio/SFX/Combat/Projectile'
path = root + '/SFX_ProjectileImpactLow'
lib = unreal.EditorAssetLibrary
source = Path(unreal.Paths.project_dir()) / 'SourceArt/Audio/SFX/ProjectileImpactLow.wav'
if not source.is_file():
    raise RuntimeError('Missing projectile impact source')
sound = unreal.load_asset(path) if lib.does_asset_exist(path) else None
if not sound:
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source.resolve()))
    task.set_editor_property('destination_path', root)
    task.set_editor_property('destination_name', 'SFX_ProjectileImpactLow')
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(path)
if not isinstance(sound, unreal.SoundWave):
    raise RuntimeError('Sound import failed')
concurrency_path = root + '/SC_ProjectileImpact'
concurrency = unreal.load_asset(concurrency_path) if lib.does_asset_exist(concurrency_path) else None
if not concurrency:
    concurrency = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'SC_ProjectileImpact', root, unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
if not concurrency:
    raise RuntimeError('Could not create impact concurrency limit')
settings = concurrency.get_editor_property('concurrency')
settings.set_editor_property('max_count', 6)
settings.set_editor_property('resolution_rule', unreal.MaxConcurrentResolutionRule.PREVENT_NEW)
concurrency.set_editor_property('concurrency', settings)
sound.set_editor_property('override_concurrency', False)
sound.set_editor_property('concurrency_set', [concurrency])
for audio_asset in [concurrency, sound]:
    if not lib.save_loaded_asset(audio_asset, False):
        raise RuntimeError('Could not save audio settings')
for kind in ['Normal', 'Explosive', 'DelayedExplosive', 'Rally', 'Guard', 'Fake', 'ParrySplit']:
    asset = unreal.load_asset('/Game/ReflectionLab/Data/Projectiles/DA_Projectile_' + kind)
    if not asset:
        raise RuntimeError('Missing projectile definition ' + kind)
    asset.set_editor_property('pool_return_sound', sound)
    asset.set_editor_property('pool_return_sound_volume', 0.30)
    if not lib.save_loaded_asset(asset, False):
        raise RuntimeError('Could not save ' + kind)
    unreal.log('PROJECTILE_IMPACT_AUDIO_ASSIGNED ' + kind)
