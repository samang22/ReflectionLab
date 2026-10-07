"""Change only firing duration on existing boss/minion laser tuning assets."""
import unreal

paths = [
    '/Game/ReflectionLab/Data/Enemies/DA_RobotBossLaser',
    '/Game/ReflectionLab/Data/Enemies/DA_RobotMinionLaser',
]
for path in paths:
    settings = unreal.load_asset(path)
    if settings is None:
        if path.endswith('DA_RobotBossLaser'):
            raise RuntimeError('Boss laser settings missing')
        unreal.log('[ShortLaser] Minion laser not created yet; setup defaults to 0.2s')
        continue
    settings.set_editor_property('firing_duration', 0.2)
    if not unreal.EditorAssetLibrary.save_loaded_asset(settings):
        raise RuntimeError('Could not save: ' + path)
    if abs(settings.get_editor_property('firing_duration') - 0.2) > 0.0001:
        raise RuntimeError('Firing duration verification failed: ' + path)
    unreal.log('[ShortLaser] {}: firing duration 0.2s'.format(path))
