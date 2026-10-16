"""Read-only verification of the saved playtest tuning."""
import unreal

field = unreal.load_asset('/Game/ReflectionLab/Data/Experiments/DA_AccelerationField')
zone = unreal.load_asset('/Game/ReflectionLab/Data/Experiments/DA_AlternatingZones')
boss = unreal.load_asset('/Game/ReflectionLab/Data/Enemies/DA_RobotBoss')
def check(condition, message):
    if not condition:
        raise RuntimeError(message)
check(abs(field.get_editor_property('baseline_hostile_speed_multiplier')-0.65)<0.001,'Baseline speed')
check(field.get_editor_property('acceleration_sound') is not None,'Acceleration audio')
extent = zone.get_editor_property('zone_half_extent')
check(abs(extent.x-450)<0.001 and abs(extent.y-700)<0.001,'Zone size')
check(abs(zone.get_editor_property('phase_duration')-4.5)<0.001,'Zone period')
check(abs(zone.get_editor_property('warning_duration')-1.5)<0.001,'Zone warning')
check(boss.get_editor_property('overload').get_editor_property('max_health')>=60,'Boss health')
unreal.log('PLAYTEST_ADJUSTMENTS_VERIFIED baseline=0.65 audio=assigned zone=900x1400 period=4.5 bossHP='+str(boss.get_editor_property('overload').get_editor_property('max_health')))
