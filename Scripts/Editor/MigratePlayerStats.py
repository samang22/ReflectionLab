"""Merge legacy tuning assets without touching animation data.
Run again with -VerifyPlayerStats, then -CleanupLegacyTuning after verification.
"""
import json
import unreal

STATS_PATH = '/Game/ReflectionLab/Data/Player/DA_PlayerStats'
PARRY_PATH = '/Game/ReflectionLab/Data/Player/DA_ParryTuning'
ROLL_PATH = '/Game/ReflectionLab/Data/Player/DA_DodgeRollTuning'
BP_PATH = '/Game/ReflectionLab/Gameplay/Player/BP_RLPlayerCharacter'
PARRY_PROPERTIES = [
    "failed_parry_cooldown",
    "successful_parry_cooldown",
    "perfect_parry_outer_band_width",
    "perfect_split_projectile_count",
    "perfect_split_angle_degrees",
    "perfect_hit_stop_duration_multiplier",
    "base_pierce_count",
    "max_reflected_speed_multiplier",
    "base_reflected_projectile_scale",
    "close_range_threshold",
    "close_range_pierce_count",
    "close_range_projectile_scale",
    "combo_speed_milestone",
    "combo_extra_projectile_milestone",
    "enhancement_stage2_combo",
    "enhancement_stage3_combo",
    "enhancement_stage4_combo",
    "combo_extra_projectile_spread_angle",
    "overdrive_combo_threshold",
    "overdrive_projectile_count",
    "overdrive_spread_angle_degrees",
    "overdrive_projectile_scale",
    "overdrive_pierce_count",
    "overdrive_hit_stop_duration_multiplier",
    "parry_range",
    "parry_half_angle_degrees",
    "indicator_idle_opacity",
    "indicator_active_opacity",
    "indicator_success_opacity",
    "indicator_unavailable_opacity",
    "impact_sound_volume",
    "swing_sound",
    "swing_sound_volume",
    "combo_impact_sounds",
    "combo_impact_sound_volumes",
    "hit_stop_duration",
    "hit_stop_time_dilation",
    "overdrive_aura_vfx",
    "overdrive_aura_scale",
    "enhancement_aura_stage2_scale_multiplier",
    "enhancement_aura_stage3_scale_multiplier",
    "enhancement_aura_stage4_scale_multiplier"
]
HEALTH_PROPERTIES = ['max_health', 'max_walk_speed', 'hit_recovery_duration', 'hit_recovery_movement_speed_multiplier']
ROLL_PROPERTIES = ['roll_play_rate', 'roll_cooldown']

def normalized(value):
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    if isinstance(value, (unreal.Array, list, tuple)):
        return [normalized(item) for item in value]
    return value

def values(asset, properties):
    return {name: normalized(asset.get_editor_property(name)) for name in properties}

stats = unreal.load_asset(STATS_PATH)
blueprint = unreal.load_asset(BP_PATH)
if not stats or not blueprint:
    raise RuntimeError('Player stats or blueprint missing')
defaults = unreal.get_default_object(blueprint.generated_class())
component = defaults.get_component_by_class(unreal.RLDodgeRollComponent)
if not component:
    raise RuntimeError('Roll component missing')
command_line = unreal.SystemLibrary.get_command_line()
verify_only = '-VerifyPlayerStats' in command_line
cleanup = '-CleanupLegacyTuning' in command_line
if not verify_only and not cleanup:
    parry = unreal.load_asset(PARRY_PATH)
    roll = unreal.load_asset(ROLL_PATH)
    if not parry or not roll:
        raise RuntimeError('Legacy tuning assets missing; do not rerun migration after cleanup')
    expected = values(stats, HEALTH_PROPERTIES)
    expected.update(values(parry, PARRY_PROPERTIES))
    expected.update(values(roll, ROLL_PROPERTIES))
    expected['roll_montage'] = normalized(roll.get_editor_property('roll_montage'))
    if not expected['roll_montage']:
        raise RuntimeError('Legacy roll montage missing')
    unreal.log('PLAYER_STATS_BASELINE ' + json.dumps(expected, sort_keys=True))
    for name in PARRY_PROPERTIES:
        stats.set_editor_property(name, parry.get_editor_property(name))
    for name in ROLL_PROPERTIES:
        stats.set_editor_property(name, roll.get_editor_property(name))
    component.set_editor_property('roll_montage', roll.get_editor_property('roll_montage'))
    defaults.set_editor_property('player_stats_data', stats)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(stats, False):
        raise RuntimeError('Stats save failed')
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        raise RuntimeError('Blueprint save failed')

if defaults.get_editor_property('player_stats_data') != stats:
    raise RuntimeError('Player blueprint stats assignment was not retained')
actual = values(stats, HEALTH_PROPERTIES + PARRY_PROPERTIES + ROLL_PROPERTIES)
actual['roll_montage'] = normalized(component.get_editor_property('roll_montage'))
if not actual['roll_montage']:
    raise RuntimeError('Player roll montage assignment missing')
unreal.log('PLAYER_STATS_VERIFIED ' + json.dumps(actual, sort_keys=True))
if not verify_only and not cleanup and actual != expected:
    raise RuntimeError('Player stats migration changed existing values')
if cleanup:
    for path in (PARRY_PATH, ROLL_PATH):
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            continue
        referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(path, True)
        if referencers:
            raise RuntimeError('Legacy asset still referenced: {} by {}'.format(path, referencers))
    for path in (PARRY_PATH, ROLL_PATH):
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            if not unreal.EditorAssetLibrary.delete_asset(path):
                raise RuntimeError('Legacy asset removal failed: ' + path)
            unreal.log('PLAYER_STATS_REMOVED_LEGACY ' + path)
