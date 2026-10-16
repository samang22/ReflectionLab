"""Verify saved boss/minion cue references and concurrency after reload."""
import unreal

boss = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBoss')
minion = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotMinionLaser')
for label, data in [('boss', boss.get_editor_property('laser')), ('minion', minion)]:
    for prop, name in [('charge_sound', 'LaserCharge'), ('fire_sound', 'LaserFire')]:
        sound = data.get_editor_property(prop)
        if not sound or sound.get_name() != 'SFX_' + name:
            raise RuntimeError(label + ': incorrect ' + prop)
        groups = list(sound.get_editor_property('concurrency_set'))
        if len(groups) != 1 or groups[0].get_editor_property('concurrency').get_editor_property('max_count') != 3:
            raise RuntimeError('Incorrect concurrency for ' + name)
    unreal.log('LASER_AUDIO_VERIFIED ' + label + ': charge/fire saved separately, concurrency=3')
