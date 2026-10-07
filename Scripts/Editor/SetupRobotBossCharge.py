"""Run after rebuilding/restarting Unreal; configure only the boss charge pattern."""
import unreal

if not hasattr(unreal, 'RLBossChargeComponent') or not hasattr(unreal, 'RLBossChargeDataAsset'):
    raise RuntimeError('Rebuild and restart Unreal before running this script')
root = '/Game/ReflectionLab'
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
editing = unreal.MaterialEditingLibrary
boss = unreal.load_asset(root + '/Gameplay/Enemies/BP_RLRobotBoss')
overload = unreal.load_asset(root + '/Data/Enemies/DA_RobotBossOverload')
if not boss or not overload:
    raise RuntimeError('Existing boss BP and overload settings are required')
projectile = overload.get_editor_property('projectile_definition')
if not projectile:
    raise RuntimeError('Boss normal projectile definition is missing')
path = root + '/Art/Materials/Enemies/M_BossChargeDecal'
material = unreal.load_asset(path)
if material is None:
    material = tools.create_asset('M_BossChargeDecal', root + '/Art/Materials/Enemies',
                                  unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Could not create charge decal material')
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
    material.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    opacity = editing.create_material_expression(material, unreal.MaterialExpressionConstant)
    opacity.set_editor_property('r', 0.55)
    # Premultiplied amber distinguishes the charge warning from the red laser.
    color = editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(1.1, 0.4, 0.01))
    for expression, prop in ((color, unreal.MaterialProperty.MP_EMISSIVE_COLOR),
                             (opacity, unreal.MaterialProperty.MP_OPACITY)):
        if not editing.connect_material_property(expression, '', prop):
            raise RuntimeError('Could not connect charge decal output')
    editing.recompile_material(material)
    if not lib.save_loaded_asset(material):
        raise RuntimeError('Could not save charge decal')
settings = unreal.load_asset(root + '/Data/Enemies/DA_RobotBossCharge')
if settings is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.RLBossChargeDataAsset.static_class())
    settings = tools.create_asset('DA_RobotBossCharge', root + '/Data/Enemies',
                                  unreal.RLBossChargeDataAsset, factory)
if not settings:
    raise RuntimeError('Could not create charge settings')
settings.set_editor_property('projectile_definition', projectile)
settings.set_editor_property('decal_material', material)
if not lib.save_loaded_asset(settings):
    raise RuntimeError('Could not save charge settings')
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
found = False
for handle in subsystem.k2_gather_subobject_data_for_blueprint(boss):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, boss)
    if isinstance(component, unreal.RLBossChargeComponent):
        component.set_editor_property('settings', settings)
        found = True
if not found:
    raise RuntimeError('Boss charge component missing; rebuild and restart required')
unreal.BlueprintEditorLibrary.compile_blueprint(boss)
if not lib.save_loaded_asset(boss):
    raise RuntimeError('Could not save boss charge settings')
defaults = unreal.get_default_object(boss.generated_class())
component = defaults.get_component_by_class(unreal.RLBossChargeComponent)
if not component or component.get_editor_property('settings') != settings:
    raise RuntimeError('Boss charge settings were not retained')
unreal.log('[BossCharge] SUCCESS: telegraph, swept charge, muzzle counter-shot, recovery')
