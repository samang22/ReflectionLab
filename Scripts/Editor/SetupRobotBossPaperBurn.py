"""Run after native rebuild/restart: configure boss-only paper-burn death."""
import unreal

root = '/Game/ReflectionLab'
lib = unreal.EditorAssetLibrary
editing = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
component_class = getattr(unreal, 'RLPaperBurnComponent', None)
settings_class = getattr(unreal, 'RLPaperBurnDataAsset', None)
boss = unreal.load_asset(root + '/Gameplay/Enemies/BP_RLRobotBoss')
if not component_class or not settings_class or not boss:
    raise RuntimeError('Rebuild native code and restart the editor first')
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
components = []
for handle in subsystem.k2_gather_subobject_data_for_blueprint(boss):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, boss)
    if isinstance(component, component_class):
        components.append(component)
if not components:
    raise RuntimeError('Boss paper-burn component missing; rebuild and restart')


def save(asset):
    if not lib.save_loaded_asset(asset):
        raise RuntimeError('Could not save: ' + asset.get_path_name())


def node(material, cls):
    result = editing.create_material_expression(material, cls)
    if not result:
        raise RuntimeError('Could not create expression: ' + str(cls))
    return result


def connect(source, target, pin, output=''):
    names = list(editing.get_material_expression_input_names(target))
    actual = pin if pin in names else (names[0] if len(names) == 1 else None)
    if actual is None or not editing.connect_material_expressions(source, output, target, actual):
        raise RuntimeError('Could not connect input: ' + pin)


def scalar(material, name, value):
    result = node(material, unreal.MaterialExpressionScalarParameter)
    result.set_editor_property('parameter_name', name)
    result.set_editor_property('default_value', value)
    return result


def binary(material, cls, a, b=None, constant=None):
    result = node(material, cls)
    connect(a, result, 'A')
    if b is not None:
        connect(b, result, 'B')
    else:
        result.set_editor_property('const_b', constant)
    return result


def unary(material, cls, source):
    result = node(material, cls)
    connect(source, result, 'Input')
    return result


path = root + '/Art/Materials/Enemies/M_RobotBossPaperBurn'
material = unreal.load_asset(path)
if material is None:
    material = lib.duplicate_asset(
        root + '/Art/Characters/Robot/robot/Materials/Material', path)
if not isinstance(material, unreal.Material):
    raise RuntimeError('Original robot material must be a Material asset')
if 'BurnProgress' not in [str(name) for name in editing.get_scalar_parameter_names(material)]:
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    editing.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    world = node(material, unreal.MaterialExpressionWorldPosition)
    z = node(material, unreal.MaterialExpressionComponentMask)
    z.set_editor_property('r', False)
    z.set_editor_property('g', False)
    z.set_editor_property('b', True)
    z.set_editor_property('a', False)
    connect(world, z, 'Input')
    height = binary(material, unreal.MaterialExpressionSubtract, z,
                    scalar(material, 'BurnBottom', 0.0))
    height = unary(material, unreal.MaterialExpressionSaturate,
                   binary(material, unreal.MaterialExpressionDivide, height,
                          scalar(material, 'BurnHeight', 200.0)))
    noise = node(material, unreal.MaterialExpressionNoise)
    noise.set_editor_property('scale', 0.035)
    noise.set_editor_property('levels', 1)
    noise.set_editor_property('quality', 1)
    noise.set_editor_property('output_min', 0.0)
    noise.set_editor_property('output_max', 1.0)
    connect(world, noise, 'Position')
    field = unary(material, unreal.MaterialExpressionSaturate,
                  binary(material, unreal.MaterialExpressionAdd,
                         binary(material, unreal.MaterialExpressionMultiply, height, constant=0.75),
                         binary(material, unreal.MaterialExpressionMultiply, noise, constant=0.25)))
    threshold = binary(material, unreal.MaterialExpressionAdd,
                       binary(material, unreal.MaterialExpressionMultiply,
                              scalar(material, 'BurnProgress', 0.0), constant=1.2), constant=-0.1)
    delta = binary(material, unreal.MaterialExpressionSubtract, field, threshold)
    opacity = unary(material, unreal.MaterialExpressionSaturate,
                    binary(material, unreal.MaterialExpressionMultiply, delta, constant=100.0))
    edge = unary(material, unreal.MaterialExpressionSaturate,
                 unary(material, unreal.MaterialExpressionOneMinus,
                       binary(material, unreal.MaterialExpressionDivide,
                              unary(material, unreal.MaterialExpressionAbs, delta),
                              scalar(material, 'BurnEdgeWidth', 0.05))))
    color = node(material, unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name', 'BurnEdgeColor')
    color.set_editor_property('default_value', unreal.LinearColor(1.0, 0.12, 0.02))
    emissive = binary(material, unreal.MaterialExpressionMultiply,
                      binary(material, unreal.MaterialExpressionMultiply, color, edge), constant=8.0)
    prop = unreal.MaterialProperty.MP_EMISSIVE_COLOR
    original = editing.get_material_property_input_node(material, prop)
    if original:
        output = editing.get_material_property_input_node_output_name(material, prop)
        combined = node(material, unreal.MaterialExpressionAdd)
        connect(original, combined, 'A', output)
        connect(emissive, combined, 'B')
        emissive = combined
    for expression, output_property in ((opacity, unreal.MaterialProperty.MP_OPACITY_MASK),
                                        (emissive, prop)):
        if not editing.connect_material_property(expression, '', output_property):
            raise RuntimeError('Could not connect paper-burn material output')
    editing.layout_material_expressions(material)
    editing.recompile_material(material)
    save(material)

settings = unreal.load_asset(root + '/Data/Enemies/DA_RobotBossPaperBurn')
if settings is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', settings_class)
    settings = tools.create_asset('DA_RobotBossPaperBurn', root + '/Data/Enemies',
                                  settings_class, factory)
if not settings:
    raise RuntimeError('Could not create paper-burn settings')
settings.set_editor_property('material', material)
save(settings)
for component in components:
    component.set_editor_property('settings', settings)
unreal.BlueprintEditorLibrary.compile_blueprint(boss)
save(boss)
defaults = unreal.get_default_object(boss.generated_class())
if defaults.get_component_by_class(component_class).get_editor_property('settings') != settings:
    raise RuntimeError('Paper-burn settings assignment was not retained')
unreal.log('[PaperBurn] SUCCESS: boss death material and settings configured')
