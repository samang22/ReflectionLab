"""Run after rebuild/restart; changes only the two boss overload durations."""
import unreal
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from RobotBossSettings import load_asset as load_boss_asset, save_loaded_asset as save_boss_asset

settings = load_boss_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBossOverload')
if not settings:
    raise RuntimeError('Boss overload tuning asset missing')
# Verify the new native property exists before changing either value.
settings.get_editor_property('vulnerable_duration')
settings.set_editor_property('duration', 5.0)
settings.set_editor_property('vulnerable_duration', 5.0)
if not save_boss_asset(settings, False):
    raise RuntimeError('Could not save boss overload durations')
unreal.log('[BossOverloadTiming] SUCCESS: absorption 5s, vulnerable firing 5s')
