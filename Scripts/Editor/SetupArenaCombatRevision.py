"""Apply only the requested round 3/4/7 layouts, with backups and reload comparison."""
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
backup = project / 'Saved/Migrations' / ('ArenaCombatRevision_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
backup.mkdir(parents=True)
root = '/Game/ReflectionLab/Data/Experiments/'
plans = [('DA_CrossfireExperiment', 'crossfire'), ('DA_AccelerationField', 'acceleration'), ('DA_MirrorArena', 'mirror')]
for name, group in plans:
    path = root + name
    asset = unreal.load_asset(path)
    if not asset or not asset.get_editor_property('grouped_settings'):
        raise RuntimeError('Expected a migrated arena asset: ' + path)
    source = project / 'Content' / (path.removeprefix('/Game/') + '.uasset')
    shutil.copy2(source, backup / source.name)
    settings = asset.get_editor_property(group)
    before = settings.export_text()
    if group == 'crossfire':
        walls = []
        for x, y, sx, sy, phase in [
            (-550, -650, 90, 650, 0), (-550, 650, 90, 650, 1),
            (550, -650, 90, 650, 1), (550, 650, 90, 650, 0),
            (-1250, 0, 450, 90, 1), (1250, 0, 450, 90, 0),
            (0, -1200, 400, 90, 0), (0, 1200, 400, 90, 1),
        ]:
            wall = unreal.RLCrossfireMazeWall()
            wall.set_editor_property('center', unreal.Vector2D(x, y))
            wall.set_editor_property('size', unreal.Vector(sx, sy, 180))
            wall.set_editor_property('phase_group', phase)
            walls.append(wall)
        settings.set_editor_property('maze_walls', walls)
        settings.set_editor_property('team_distance', 1550.0)
        settings.set_editor_property('team_spread', 1100.0)
    elif group == 'acceleration':
        settings.set_editor_property('enable_return_barriers', True)
        settings.set_editor_property('return_barrier_centers', [unreal.Vector2D(-950, 0), unreal.Vector2D(950, 0)])
        settings.set_editor_property('return_barrier_size', unreal.Vector(60, 4000, 220))
    else:
        settings.set_editor_property('shot_barrier_length', 480.0)
        settings.set_editor_property('diagonal_mirror_center', unreal.Vector2D(950, 1100))
        settings.set_editor_property('mirror_camera_arm_length', 4600.0)
    asset.set_editor_property(group, settings)
    expected = settings.export_text()
    (backup / (name + '.json')).write_text(json.dumps({'before': before, 'after': expected}, ensure_ascii=False, indent=2), encoding='utf-8')
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Save failed: ' + path)
    result = unreal.EditorLoadingAndSavingUtils.reload_packages([asset.get_outermost()], unreal.ReloadPackagesInteractionMode.ASSUME_NEGATIVE)
    if not result[0] or str(result[1]):
        raise RuntimeError('Reload failed: ' + str(result))
    asset = unreal.load_asset(path)
    if asset.get_editor_property(group).export_text() != expected:
        raise RuntimeError('Reload values changed: ' + path)
    unreal.log('ARENA_COMBAT_REVISION_APPLIED ' + json.dumps({'asset': path, 'backup': str(backup)}))
