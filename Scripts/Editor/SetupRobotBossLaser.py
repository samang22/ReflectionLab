"""Run after native rebuild/restart; adds only laser assets/settings to the existing boss."""
import unreal

if not hasattr(unreal, 'RLBossLaserComponent'):
    raise RuntimeError('Rebuild and restart Unreal before running this script')
root = '/Game/ReflectionLab'
tools = unreal.AssetToolsHelpers.get_asset_tools()
editing = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
path = root + '/Art/Materials/Enemies/M_BossLaserDecal'
material = unreal.load_asset(path)
if material is None:
    material = tools.create_asset('M_BossLaserDecal', root + '/Art/Materials/Enemies', unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Could not create laser decal material')
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
    material.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    color = editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(1.5, 0.03, 0.01))
    opacity = editing.create_material_expression(material, unreal.MaterialExpressionConstant)
    opacity.set_editor_property('r', 0.5)
    if not editing.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR) or not editing.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY):
        raise RuntimeError('Could not connect laser decal outputs')
    editing.recompile_material(material)
    if not lib.save_loaded_asset(material):
        raise RuntimeError('Could not save laser decal material')
settings = unreal.load_asset(root + '/Data/Enemies/DA_RobotBossLaser')
if settings is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.RLBossLaserDataAsset.static_class())
    settings = tools.create_asset('DA_RobotBossLaser', root + '/Data/Enemies', unreal.RLBossLaserDataAsset, factory)
beam_material = unreal.load_asset(root + '/Art/Materials/Projectiles/MI_Projectile_Hostile')
boss = unreal.load_asset(root + '/Gameplay/Enemies/BP_RLRobotBoss')
if not settings or not beam_material or not boss:
    raise RuntimeError('Laser tuning, hostile material or existing boss BP missing')
settings.set_editor_property('beam_material', beam_material)
settings.set_editor_property('decal_material', material)
if not lib.save_loaded_asset(settings):
    raise RuntimeError('Could not save laser tuning')
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
found = False
for handle in subsystem.k2_gather_subobject_data_for_blueprint(boss):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, boss)
    if isinstance(component, unreal.RLBossLaserComponent):
        component.set_editor_property('settings', settings)
        found = True
if not found:
    raise RuntimeError('Boss laser component missing; rebuild/restart required')
unreal.BlueprintEditorLibrary.compile_blueprint(boss)
if not lib.save_loaded_asset(boss):
    raise RuntimeError('Could not save boss laser settings')
defaults = unreal.get_default_object(boss.generated_class())
if defaults.get_component_by_class(unreal.RLBossLaserComponent).get_editor_property('settings') != settings:
    raise RuntimeError('Boss laser settings were not retained')
mesh = defaults.get_component_by_class(unreal.SkeletalMeshComponent)
if not mesh.does_socket_exist('laser'):
    raise RuntimeError("Boss mesh has no saved 'laser' socket; save it before testing")
unreal.log('[BossLaser] SUCCESS: 2s aiming, 3s fixed laser, 10s cooldown')
