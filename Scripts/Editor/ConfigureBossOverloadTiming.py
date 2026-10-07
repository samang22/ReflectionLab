"""Run after rebuild/restart; changes only the two boss overload durations."""
import unreal

settings = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBossOverload')
if not settings:
    raise RuntimeError('Boss overload tuning asset missing')
# Verify the new native property exists before changing either value.
settings.get_editor_property('vulnerable_duration')
settings.set_editor_property('duration', 5.0)
settings.set_editor_property('vulnerable_duration', 5.0)
if not unreal.EditorAssetLibrary.save_loaded_asset(settings, False):
    raise RuntimeError('Could not save boss overload durations')
unreal.log('[BossOverloadTiming] SUCCESS: absorption 5s, vulnerable firing 5s')
