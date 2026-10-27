"""Back up six arena assets, preserve every migrated field, save/reload and compare.

Legacy assets are converted in PostLoad; this persists and verifies the conversion.
No asset rename/deletion or Run modification is performed.
"""
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal
from rl_arena_settings_schema import GROUPS
from rl_asset_changes import UnrealBackend, equivalent

backend = UnrealBackend()
project = Path(backend.project)
backup = project / 'Saved/Migrations' / ('ArenaSettings_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
backup.mkdir(parents=True)
names = ('DA_CrossfireExperiment', 'DA_AccelerationField', 'DA_RefractionGate',
         'DA_SupportNetwork', 'DA_AlternatingZones', 'DA_MirrorArena')
for name in names:
    path = '/Game/ReflectionLab/Data/Experiments/' + name
    source = project / 'Content' / (path.removeprefix('/Game/') + '.uasset')
    shutil.copy2(source, backup / source.name)
    asset = backend.asset(path)
    expected = {group + '.' + prop: backend.encode(asset.get_editor_property(prop))
                for group, props in GROUPS.items() for prop in props}
    expected['surface_material'] = backend.encode(asset.get_editor_property('shutter_material'))
    # Do not rerun this migration against assets already tuned using grouped settings.
    mismatches = [prop for prop, value in expected.items() if not equivalent(backend.read(path, prop), value)]
    if mismatches:
        raise RuntimeError('Legacy/grouped value conflict; no save: ' + path + ' ' + str(mismatches))
    common = {prop: str(asset.get_editor_property(prop)) for prop in
              ('type', 'display_name', 'attack_frequency_multiplier', 'projectile_definition')}
    (backup / (name + '.json')).write_text(json.dumps({'expected': expected, 'common': common}, ensure_ascii=False, indent=2), encoding='utf-8')
    backend.save(path)
    backend.reload([path])
    if not all(equivalent(backend.read(path, prop), value) for prop, value in expected.items()):
        raise RuntimeError('Reload mismatch: ' + path)
    asset = backend.asset(path)
    if common != {prop: str(asset.get_editor_property(prop)) for prop in common}:
        raise RuntimeError('Common property mismatch: ' + path)
    if not asset.get_editor_property('grouped_settings'):
        raise RuntimeError('Migration flag missing after reload: ' + path)
    unreal.log('ARENA_SETTINGS_MIGRATED ' + json.dumps({'asset': path, 'fields': len(expected), 'backup': str(backup)}))
