"""Apply editable first-pass playtest tuning; preserve other rounds and test-start override."""
from pathlib import Path
import unreal

lib = unreal.EditorAssetLibrary
root = '/Game/ReflectionLab/Audio/SFX/Combat/Projectile'
task = unreal.AssetImportTask()
task.set_editor_property('filename', str((Path(unreal.Paths.project_dir())/'SourceArt/Audio/SFX/Acceleration.wav').resolve()))
task.set_editor_property('destination_path', root)
task.set_editor_property('destination_name', 'SFX_Acceleration')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = unreal.load_asset(root+'/SFX_Acceleration')
if not sound:
    raise RuntimeError('Acceleration audio import failed')
path = root+'/SC_Acceleration'
group = unreal.load_asset(path) if lib.does_asset_exist(path) else None
if not group:
    group = unreal.AssetToolsHelpers.get_asset_tools().create_asset('SC_Acceleration',root,unreal.SoundConcurrency,unreal.SoundConcurrencyFactory())
settings = group.get_editor_property('concurrency')
settings.set_editor_property('max_count',4)
settings.set_editor_property('retrigger_time',0.06)
settings.set_editor_property('resolution_rule',unreal.MaxConcurrentResolutionRule.PREVENT_NEW)
group.set_editor_property('concurrency',settings)
sound.set_editor_property('override_concurrency',False)
sound.set_editor_property('concurrency_set',[group])
field = unreal.load_asset('/Game/ReflectionLab/Data/Experiments/DA_AccelerationField')
zone = unreal.load_asset('/Game/ReflectionLab/Data/Experiments/DA_AlternatingZones')
boss = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBoss')
if not all([field,zone,boss]):
    raise RuntimeError('Required playtest settings missing')
field.set_editor_property('baseline_hostile_speed_multiplier',0.65)
field.set_editor_property('field_speed_multiplier',1.5)
field.set_editor_property('acceleration_sound',sound)
field.set_editor_property('acceleration_sound_volume',0.25)
zone.set_editor_property('zone_half_extent',unreal.Vector2D(x=450,y=700))
zone.set_editor_property('zone_center_distance',750.0)
zone.set_editor_property('phase_duration',4.5)
zone.set_editor_property('warning_duration',1.5)
overload = boss.get_editor_property('overload')
previous = overload.get_editor_property('max_health')
overload.set_editor_property('max_health',max(previous,60.0))
for asset in [sound,group,field,zone,boss]:
    if not lib.save_loaded_asset(asset,False):
        raise RuntimeError('Could not save '+asset.get_path_name())
unreal.log('PLAYTEST_ADJUSTMENTS_SAVED slow=0.65 boost=1.5 zones=900x1400 period=4.5 warning=1.5 boss='+str(overload.get_editor_property('max_health'))+' previous='+str(previous))
