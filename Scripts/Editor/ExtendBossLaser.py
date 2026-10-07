"""Set the existing boss laser to three times its original 2200 cm length."""
import unreal

settings = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBossLaser')
if settings is None:
    raise RuntimeError('DA_RobotBossLaser is missing')
settings.set_editor_property('length', 6600.0)
if not unreal.EditorAssetLibrary.save_loaded_asset(settings):
    raise RuntimeError('Could not save boss laser length')
if settings.get_editor_property('length') != 6600.0:
    raise RuntimeError('Boss laser length verification failed')
unreal.log('[BossLaser] Length set to 6600 cm; beam and decal share this length')
