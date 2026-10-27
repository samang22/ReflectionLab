"""Preserve current disk values, consolidate round settings, save/reload and compare.

Run using UnrealEditor-Cmd -run=pythonscript -script=<this file> with the editor closed.
Original .uasset files are copied to Saved/Migrations before any save, including user overrides.
"""
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
backup = project / 'Saved/Migrations' / ('ExplicitRounds_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
backup.mkdir(parents=True)
paths = [
    '/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun',
    '/Game/ReflectionLab/Data/Difficulty/DA_TestRun_60s',
]


def wave_text(schedule):
    return [wave.export_text() for wave in schedule.get_editor_property('waves')]


for path in paths:
    asset = unreal.load_asset(path)
    if not isinstance(asset, unreal.RLRunDefinitionDataAsset):
        raise RuntimeError('Missing run definition: ' + path)
    if asset.get_editor_property('explicit_round_layout'):
        unreal.log('EXPLICIT_ROUNDS_ALREADY_MIGRATED ' + path)
        continue
    source = project / 'Content' / (path.removeprefix('/Game/') + '.uasset')
    shutil.copy2(source, backup / source.name)
    starting_round = asset.get_editor_property('starting_round_number')
    before = []
    for row in asset.get_editor_property('rounds'):
        waves = wave_text(row.get_editor_property('difficulty_schedule'))
        split = (len(waves) + 1) // 2
        if row.get_editor_property('is_tutorial') or len(waves) < 2:
            before.append(waves)
        else:
            before.extend([waves[:split], waves[split:]])
    (backup / (source.stem + '_before.json')).write_text(
        json.dumps({'starting_round': starting_round, 'round_waves': before}, ensure_ascii=False, indent=2), encoding='utf-8')
    if not asset.migrate_legacy_round_layout():
        raise RuntimeError('Migration rejected ' + path)
    after = [wave_text(row.get_editor_property('difficulty_schedule')) for row in asset.get_editor_property('rounds')]
    if after != before or asset.get_editor_property('starting_round_number') != starting_round:
        raise RuntimeError('Migration changed wave data or start override; asset not saved: ' + path)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Save failed; backup available: ' + str(backup))
    package = asset.get_outermost()
    unreal.EditorLoadingAndSavingUtils.reload_packages([package], unreal.ReloadPackagesInteractionMode.ASSUME_POSITIVE)
    asset = unreal.load_asset(path)
    after = [wave_text(row.get_editor_property('difficulty_schedule')) for row in asset.get_editor_property('rounds')]
    if after != before or not asset.get_editor_property('explicit_round_layout'):
        raise RuntimeError('Reload validation failed; backup available: ' + str(backup))
    unreal.log('EXPLICIT_ROUNDS_MIGRATED ' + json.dumps({
        'asset': path, 'rows': len(after), 'starting_round': starting_round, 'backup': str(backup)
    }))
