"""Assign the original powerup only to power-level increases; preserve all parry audio."""
from pathlib import Path
import unreal

lib = unreal.EditorAssetLibrary
stats = lib.load_asset('/Game/ReflectionLab/Data/Player/DA_PlayerStats')
if not stats:
    raise RuntimeError('Player stats missing')
stats.get_editor_property('power_level_up_sound')
before_stages = list(stats.get_editor_property('combo_impact_sounds'))
before_perfect = stats.get_editor_property('perfect_parry_sound')
path = '/Game/ReflectionLab/Audio/SFX/Combat/Parry/Results/SFX_PowerLevelUp'
sound = lib.load_asset(path) if lib.does_asset_exist(path) else None
if not sound:
    source = Path(unreal.Paths.project_dir()) / 'Saved/AudioImports/Combo5_PowerUp.wav'
    if not source.is_file():
        raise RuntimeError('Original Combo5_PowerUp.wav missing')
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source.resolve()))
    task.set_editor_property('destination_path', '/Game/ReflectionLab/Audio/SFX/Combat/Parry/Results')
    task.set_editor_property('destination_name', 'SFX_PowerLevelUp')
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = lib.load_asset('/Game/ReflectionLab/Audio/SFX/Combat/Parry/Results/SFX_PowerLevelUp')
if not isinstance(sound, unreal.SoundWave):
    raise RuntimeError('Power-level sound import failed')
stats.set_editor_property('power_level_up_sound', sound)
if not lib.save_loaded_asset(stats):
    raise RuntimeError('Could not save player stats')
if list(stats.get_editor_property('combo_impact_sounds')) != before_stages or stats.get_editor_property('perfect_parry_sound') != before_perfect:
    raise RuntimeError('Parry sounds unexpectedly changed')
if stats.get_editor_property('power_level_up_sound') != sound:
    raise RuntimeError('Power-level sound assignment mismatch')
unreal.log('POWER_LEVEL_AUDIO_SUCCESS: powerup assigned; stage/perfect audio unchanged')
