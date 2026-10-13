"""Set the existing boss laser to three times its original 2200 cm length."""
import unreal
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from RobotBossSettings import load_asset as load_boss_asset, save_loaded_asset as save_boss_asset

settings = load_boss_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBossLaser')
if settings is None:
    raise RuntimeError('DA_RobotBossLaser is missing')
settings.set_editor_property('length', 6600.0)
if not save_boss_asset(settings):
    raise RuntimeError('Could not save boss laser length')
if settings.get_editor_property('length') != 6600.0:
    raise RuntimeError('Boss laser length verification failed')
unreal.log('[BossLaser] Length set to 6600 cm; beam and decal share this length')
