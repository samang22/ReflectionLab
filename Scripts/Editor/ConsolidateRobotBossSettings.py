"""Preserve configured values, migrate boss BP, then remove obsolete standalone settings."""
import unreal
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from RobotBossSettings import PATH, SECTIONS

lib = unreal.EditorAssetLibrary
boss = unreal.load_asset('/Game/ReflectionLab/Gameplay/Enemies/BP_RLRobotBoss')
if not boss:
    raise RuntimeError('Boss blueprint missing')
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
component_types = {
    'overload': unreal.RLBossOverloadComponent, 'charge': unreal.RLBossChargeComponent,
    'laser': unreal.RLBossLaserComponent, 'summon': unreal.RLBossSummonComponent,
    'paper_burn': unreal.RLPaperBurnComponent,
}
fields = {
    'overload': 'max_health damage_threshold duration vulnerable_duration radius heal_per_projectile spin_speed spin_shot_interval shot_interval projectiles_per_volley pulse_amplitude pulse_frequency projectile_definition decal_material',
    'charge': 'preparation_duration distance speed braking_duration recovery_duration cooldown contact_damage counter_shot_count counter_spread_degrees projectile_definition decal_material',
    'laser': 'preparation_duration firing_duration fade_out_duration cooldown length width damage damage_interval beam_material decal_material',
    'summon': 'minion_class initial_delay interval enraged_interval enraged_health_ratio minions_per_summon max_alive_minions spawn_radius min_player_distance telegraph_duration spawn_stagger marker_radius marker_material pulse_material summon_sound',
    'paper_burn': 'duration edge_width edge_color material',
}
components = {}
for handle in subsystem.k2_gather_subobject_data_for_blueprint(boss):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, boss)
    for section, cls in component_types.items():
        if isinstance(component, cls):
            components[section] = component
if set(components) != set(fields):
    raise RuntimeError('Missing boss components')
settings = unreal.load_asset(PATH) if lib.does_asset_exist(PATH) else None
if not settings:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.RLRobotBossDataAsset.static_class())
    settings = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'DA_RobotBoss', '/Game/ReflectionLab/Data/Enemies', unreal.RLRobotBossDataAsset, factory)
if not settings:
    raise RuntimeError('Could not create consolidated settings')
for section, component in components.items():
    source = component.get_editor_property('settings')
    if not source:
        source = unreal.get_default_object(boss.generated_class()).get_component_by_class(component_types[section]).get_editor_property('settings')
    destination = settings.get_editor_property(section)
    if not source:
        old_name = next(name for name, value in SECTIONS.items() if value == section)
        old_path = '/Game/ReflectionLab/Data/Enemies/' + old_name
        source = unreal.load_asset(old_path) if lib.does_asset_exist(old_path) else destination
    if section == 'paper_burn' and not source:
        # Do not enable an effect that was not configured on the existing boss.
        settings.set_editor_property(section, None)
        continue
    if not source or not destination:
        raise RuntimeError('Missing section: ' + section)
    for field in fields[section].split():
        destination.set_editor_property(field, source.get_editor_property(field))
        if destination.get_editor_property(field) != source.get_editor_property(field):
            raise RuntimeError('Value mismatch: ' + section + '.' + field)
if not lib.save_loaded_asset(settings, False):
    raise RuntimeError('Could not save consolidated settings')
defaults = unreal.get_default_object(boss.generated_class())
defaults.set_editor_property('boss_settings', settings)
# Components receive their section at runtime. Remove duplicate editor assignments.
for component in components.values():
    component.set_editor_property('settings', None)
for cls in component_types.values():
    defaults.get_component_by_class(cls).set_editor_property('settings', None)
unreal.BlueprintEditorLibrary.compile_blueprint(boss)
if not lib.save_loaded_asset(boss, False):
    raise RuntimeError('Could not save boss blueprint')
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
for name in SECTIONS:
    old = '/Game/ReflectionLab/Data/Enemies/' + name
    if lib.does_asset_exist(old):
        refs = [str(ref) for ref in (registry.get_referencers(old, options) or [])]
        if refs:
            raise RuntimeError('Obsolete asset still referenced: {} {}'.format(old, refs))
        if not lib.delete_asset(old):
            raise RuntimeError('Could not remove ' + old)
unreal.log('[BossConsolidation] SUCCESS: values copied and boss connected')
