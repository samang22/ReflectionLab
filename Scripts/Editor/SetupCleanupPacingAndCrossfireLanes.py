"""Tune cleanup pacing and paired crossfire lanes, preserving run entry and other rounds."""
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal
from rl_asset_changes import editor_service

project = Path(unreal.Paths.project_dir()).resolve()
backup = project / 'Saved/Migrations' / ('CleanupAndLanes_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
backup.mkdir(parents=True)
run_path = '/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun'
arena_path = '/Game/ReflectionLab/Data/Experiments/DA_CrossfireExperiment'
for path in (run_path, arena_path):
    source = project / ('Content/' + path.removeprefix('/Game/') + '.uasset')
    shutil.copy2(source, backup / source.name)

run = unreal.load_asset(run_path)
starting_round = run.get_editor_property('starting_round_number')
rounds_before = [row.export_text() for row in run.get_editor_property('rounds')]
service = editor_service()
plan = service.preview({'changes': [
    {'asset': run_path, 'property': 'cleanup_pacing.enabled', 'value': True},
    {'asset': run_path, 'property': 'cleanup_pacing.remaining_enemy_threshold', 'value': 3},
    {'asset': run_path, 'property': 'cleanup_pacing.threshold_attack_rate', 'value': 1.4},
    {'asset': run_path, 'property': 'cleanup_pacing.last_enemy_attack_rate', 'value': 2.2},
    {'asset': run_path, 'property': 'cleanup_pacing.no_threat_max_wait', 'value': 0.7},
]})
if not service.apply(plan['id'])['matches']:
    raise RuntimeError('Cleanup pacing reload validation failed')
run = unreal.load_asset(run_path)
if run.get_editor_property('starting_round_number') != starting_round or [row.export_text() for row in run.get_editor_property('rounds')] != rounds_before:
    raise RuntimeError('Unrelated run fields changed')

arena = unreal.load_asset(arena_path)
settings = arena.get_editor_property('crossfire')
before = settings.export_text()
settings.set_editor_property('alternating_firing_lanes', True)
settings.set_editor_property('lane_center_offset', 700.0)
settings.set_editor_property('keep_last_occupied_lane_open', True)
settings.set_editor_property('team_distance', 1450.0)
settings.set_editor_property('team_spread', 150.0)
walls = []
for x, y, width, length, group, permanent in [
    (-600, -1600, 100, 1200, 0, True),
    (-600, 900, 100, 2600, 0, True),
    (600, -900, 100, 2600, 1, True),
    (600, 1600, 100, 1200, 1, True),
    (-600, -700, 100, 600, 0, False),
    (600, 700, 100, 600, 1, False),
]:
    wall = unreal.RLCrossfireMazeWall()
    wall.set_editor_property('center', unreal.Vector2D(x, y))
    wall.set_editor_property('size', unreal.Vector(width, length, 180))
    wall.set_editor_property('phase_group', group)
    wall.set_editor_property('always_closed', permanent)
    walls.append(wall)
settings.set_editor_property('maze_walls', walls)
arena.set_editor_property('crossfire', settings)
expected = settings.export_text()
if not unreal.EditorAssetLibrary.save_loaded_asset(arena, False):
    raise RuntimeError('Crossfire save failed')
result = unreal.EditorLoadingAndSavingUtils.reload_packages([arena.get_outermost()], unreal.ReloadPackagesInteractionMode.ASSUME_NEGATIVE)
if not result[0] or str(result[1]) or unreal.load_asset(arena_path).get_editor_property('crossfire').export_text() != expected:
    raise RuntimeError('Crossfire reload verification failed')
(backup / 'changes.json').write_text(json.dumps({'before': before, 'after': expected, 'plan_id': plan['id'], 'starting_round': starting_round}, indent=2), encoding='utf-8')
unreal.log('CLEANUP_AND_LANES_APPLIED ' + json.dumps({'backup': str(backup), 'plan_id': plan['id'], 'starting_round_preserved': starting_round}))
