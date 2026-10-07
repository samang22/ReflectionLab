"""Run after native rebuild/restart. Create minion assets and link the boss summon component."""
import unreal

for name in ('RLRobotMinionCharacter', 'RLRobotMinionDataAsset', 'RLBossSummonComponent',
             'RLBossSummonDataAsset', 'RLBossLaserDataAsset'):
    if not hasattr(unreal, name):
        raise RuntimeError('Rebuild and restart Unreal before running this script: ' + name)
root = '/Game/ReflectionLab'
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
boss = unreal.load_asset(root + '/Gameplay/Enemies/BP_RLRobotBoss')
projectile_class = lib.load_blueprint_class(root + '/Gameplay/Projectiles/BP_RLProjectile')
projectile = unreal.load_asset(root + '/Data/Projectiles/DA_Projectile_Normal')
laser_material = unreal.load_asset(root + '/Art/Materials/Enemies/M_BossLaserDecal')
beam_material = unreal.load_asset(root + '/Art/Materials/Projectiles/MI_Projectile_Hostile')
if not all((boss, projectile_class, projectile, laser_material, beam_material)):
    raise RuntimeError('Existing boss, normal projectile and laser materials are required')


def data_asset(name, cls):
    asset = unreal.load_asset(root + '/Data/Enemies/' + name)
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', cls.static_class())
        asset = tools.create_asset(name, root + '/Data/Enemies', cls, factory)
    if not isinstance(asset, cls):
        raise RuntimeError('Wrong or missing asset type: ' + name)
    return asset


def save(asset):
    if not lib.save_loaded_asset(asset):
        raise RuntimeError('Could not save: ' + asset.get_path_name())


laser_path = root + '/Data/Enemies/DA_RobotMinionLaser'
new_laser = unreal.load_asset(laser_path) is None
laser = data_asset('DA_RobotMinionLaser', unreal.RLBossLaserDataAsset)
if new_laser:
    for name, value in (('preparation_duration', 1.5), ('firing_duration', 0.2),
                        ('cooldown', 6.0), ('length', 1800.0), ('width', 35.0)):
        laser.set_editor_property(name, value)
laser.set_editor_property('beam_material', beam_material)
laser.set_editor_property('decal_material', laser_material)
save(laser)
stats = data_asset('DA_RobotMinion', unreal.RLRobotMinionDataAsset)
stats.set_editor_property('projectile_definition', projectile)
stats.set_editor_property('laser_settings', laser)
save(stats)
movement = data_asset('DA_RobotMinionMovement', unreal.RLEnemyMovementDataAsset)
movement.set_editor_property('moving_enemy_ratio', 0.0)
save(movement)
path = root + '/Gameplay/Enemies/BP_RLRobotMinion'
minion = unreal.load_asset(path)
if minion is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.RLRobotMinionCharacter.static_class())
    minion = tools.create_asset('BP_RLRobotMinion', root + '/Gameplay/Enemies', unreal.Blueprint, factory)
if not isinstance(minion, unreal.Blueprint):
    raise RuntimeError('Could not create minion blueprint')
defaults = unreal.get_default_object(minion.generated_class())
defaults.set_editor_property('projectile_class', projectile_class)
defaults.set_editor_property('settings', stats)
boss_defaults = unreal.get_default_object(boss.generated_class())
original_mesh = boss_defaults.get_component_by_class(unreal.SkeletalMeshComponent)
original_spawn = boss_defaults.get_component_by_class(unreal.RLEnemySpawnVisualComponent)
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def components(blueprint):
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        yield unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)


for component in components(minion):
    if isinstance(component, unreal.SkeletalMeshComponent):
        component.set_skeletal_mesh_asset(original_mesh.get_editor_property('skeletal_mesh_asset'))
        component.set_editor_property('animation_mode', unreal.AnimationMode.ANIMATION_BLUEPRINT)
        component.set_anim_instance_class(unreal.RLRobotEnemyAnimInstance)
        scale = original_mesh.get_editor_property('relative_scale3d')
        component.set_editor_property('relative_scale3d', unreal.Vector(scale.x * 0.45, scale.y * 0.45, scale.z * 0.45))
        component.set_editor_property('relative_rotation', original_mesh.get_editor_property('relative_rotation'))
        component.set_editor_property('relative_location', unreal.Vector(0, 0, -44.0))
    elif isinstance(component, unreal.CapsuleComponent):
        component.set_capsule_size(24.0, 44.0)
    elif isinstance(component, unreal.RLEnemyMovementComponent):
        component.set_editor_property('movement_settings', movement)
    elif isinstance(component, unreal.RLEnemySpawnVisualComponent):
        # Preserve the optional summon presentation on subsequent setup runs.
        presentation_material = unreal.load_asset(root + '/Art/Materials/Characters/M_RobotMinionSpawn')
        if presentation_material:
            component.set_editor_property('spawn_material', presentation_material)
        else:
            for name in ('spawn_material', 'spawn_duration'):
                component.set_editor_property(name, original_spawn.get_editor_property(name))
    elif isinstance(component, unreal.RLBossLaserComponent):
        component.set_editor_property('serialize_with_peers', True)
unreal.BlueprintEditorLibrary.compile_blueprint(minion)
save(minion)
settings = data_asset('DA_RobotBossSummon', unreal.RLBossSummonDataAsset)
settings.set_editor_property('minion_class', minion.generated_class())
save(settings)
found = False
for component in components(boss):
    if isinstance(component, unreal.RLBossSummonComponent):
        component.set_editor_property('settings', settings)
        found = True
if not found:
    raise RuntimeError('Boss summon component missing; rebuild/restart required')
unreal.BlueprintEditorLibrary.compile_blueprint(boss)
save(boss)
boss_defaults = unreal.get_default_object(boss.generated_class())
if boss_defaults.get_component_by_class(unreal.RLBossSummonComponent).get_editor_property('settings') != settings:
    raise RuntimeError('Summon settings were not retained')
defaults = unreal.get_default_object(minion.generated_class())
if defaults.get_editor_property('settings') != stats:
    raise RuntimeError('Minion settings were not retained')
mesh = defaults.get_component_by_class(unreal.SkeletalMeshComponent)
if not mesh.does_socket_exist('muzzle') or not mesh.does_socket_exist('laser'):
    raise RuntimeError('Save robot muzzle and laser sockets before testing')
unreal.log('[BossSummon] SUCCESS: initial 5s, repeat 12s/8s, 2 per summon, max 3 alive')
