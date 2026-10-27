"""Reduce round-four field size and distribute six fields; preserve barrier and speed tuning."""
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal
from rl_asset_changes import editor_service

project = Path(unreal.Paths.project_dir()).resolve()
path = '/Game/ReflectionLab/Data/Experiments/DA_AccelerationField'
backup = project / 'Saved/Migrations' / ('AccelerationLayout_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
backup.mkdir(parents=True)
shutil.copy2(project / 'Content/ReflectionLab/Data/Experiments/DA_AccelerationField.uasset', backup / 'DA_AccelerationField.uasset')
service = editor_service()
plan = service.preview({'changes': [
    {'asset': path, 'property': 'field_layout.field_centers', 'value': [
        {'$type': 'Vector2D', 'x': x, 'y': y}
        for x in (-600, 600) for y in (-900, 0, 900)
    ]},
    {'asset': path, 'property': 'field_layout.field_half_extent',
     'value': {'$type': 'Vector2D', 'x': 120, 'y': 220}},
]})
result = service.apply(plan['id'])
if not result['matches']:
    raise RuntimeError('Acceleration layout verification failed')
unreal.log('ACCELERATION_LAYOUT_APPLIED ' + json.dumps({
    'plan_id': plan['id'], 'changes': plan['changes'], 'backup': str(backup)
}))
