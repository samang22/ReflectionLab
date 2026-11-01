"""Disable round-four rings and frame the round-seven mirrors without changing the run."""
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal
from rl_asset_changes import editor_service

project = Path(unreal.Paths.project_dir()).resolve()
run_file = project / 'Content/ReflectionLab/Data/Difficulty/DA_DefaultRun.uasset'
run_hash = hashlib.sha256(run_file.read_bytes()).hexdigest()
backup = project / 'Saved/Migrations' / ('RoundFourMirrorVisibility_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
backup.mkdir(parents=True)
for name in ('DA_AccelerationField', 'DA_MirrorArena'):
    shutil.copy2(project / ('Content/ReflectionLab/Data/Experiments/' + name + '.uasset'), backup / (name + '.uasset'))

service = editor_service()
plan = service.preview({'changes': [
    {'asset': '/Game/ReflectionLab/Data/Experiments/DA_AccelerationField',
     'property': 'allow_ring_attacks', 'value': False},
    {'asset': '/Game/ReflectionLab/Data/Experiments/DA_MirrorArena',
     'property': 'mirror.mirror_camera_arm_length', 'value': 5800.0},
]})
result = service.apply(plan['id'])
if not result['matches']:
    raise RuntimeError('Round tuning save/reload verification failed')
if hashlib.sha256(run_file.read_bytes()).hexdigest() != run_hash:
    raise RuntimeError('Unrelated run asset was modified')
unreal.log('ROUND_VISIBILITY_TUNING_APPLIED ' + json.dumps({
    'plan_id': plan['id'], 'changes': plan['changes'], 'backup': str(backup),
    'run_unchanged': True,
}))
