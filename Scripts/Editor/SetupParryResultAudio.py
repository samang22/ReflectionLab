"""Assign stage impacts and perfect critical strike; -PerfectAudioOnly preserves stage tuning."""
from pathlib import Path
import unreal

lib = unreal.EditorAssetLibrary
stats = lib.load_asset('/Game/ReflectionLab/Data/Player/DA_PlayerStats')
if not stats:
    raise RuntimeError('Player stats missing')
stats.get_editor_property('perfect_parry_sound')  # Validate the rebuilt DLL first.
folder = Path(unreal.Paths.project_dir()) / 'Saved/AudioImports/ParryResult'
perfect_only = '-PerfectAudioOnly' in unreal.SystemLibrary.get_command_line()
names = ['SFX_ParryStageImpact_0' + str(index) for index in range(1, 5)]
names.append('SFX_PerfectParryCriticalStrike')
if perfect_only:
    names = names[-1:]
destination = '/Game/ReflectionLab/Audio/SFX/Combat/Parry/Results'
if not all((folder / (name + '.wav')).is_file() for name in names):
    raise RuntimeError('GenerateParryResultAudio.py outputs missing')
tasks = []
for name in names:
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str((folder / (name + '.wav')).resolve()))
    task.set_editor_property('destination_path', destination)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
sounds = [lib.load_asset(destination + '/' + name) for name in names]
if not all(isinstance(sound, unreal.SoundWave) for sound in sounds):
    raise RuntimeError('Parry result audio import failed')
if not perfect_only:
    stages = list(stats.get_editor_property('combo_impact_sounds'))
    while len(stages) < 8:
        stages.append(None)
    for index, sound in zip((0, 2, 4, 7), sounds[:4]):
        stages[index] = sound
    stats.set_editor_property('combo_impact_sounds', stages)
    stats.set_editor_property('perfect_parry_sound_volume', 0.8)
    stats.set_editor_property('perfect_impact_volume_multiplier', 0.4)
stats.set_editor_property('perfect_parry_sound', sounds[-1])
if not lib.save_loaded_asset(stats):
    raise RuntimeError('Could not save player stats')
configured = stats.get_editor_property('combo_impact_sounds')
if not perfect_only and any(configured[index] != sound for index, sound in zip((0, 2, 4, 7), sounds[:4])):
    raise RuntimeError('Stage impact assignment mismatch')
if stats.get_editor_property('perfect_parry_sound') != sounds[-1]:
    raise RuntimeError('Perfect sound assignment mismatch')
unreal.log('PARRY_RESULT_AUDIO_SUCCESS: perfect critical strike assigned; perfect-only=' + str(perfect_only))
