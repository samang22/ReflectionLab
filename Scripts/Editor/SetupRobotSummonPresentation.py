"""After rebuild/restart, add cyan summon markers and minion assembly flash only."""
import unreal

root = '/Game/ReflectionLab'
lib = unreal.EditorAssetLibrary
editing = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
settings = unreal.load_asset(root + '/Data/Enemies/DA_RobotBossSummon')
minion = unreal.load_asset(root + '/Gameplay/Enemies/BP_RLRobotMinion')
base_spawn = unreal.load_asset(root + '/Art/Materials/Characters/M_EnemySpawn')
if not settings or not minion or not base_spawn:
    raise RuntimeError('Run SetupRobotBossSummon.py and configure the existing enemy spawn material first')
# Fail before editing any asset when the new native fields are unavailable.
settings.get_editor_property('telegraph_duration')


def save(asset):
    if not lib.save_loaded_asset(asset):
        raise RuntimeError('Could not save: ' + asset.get_path_name())


def node(material, cls):
    result = editing.create_material_expression(material, cls)
    if not result:
        raise RuntimeError('Could not create expression: ' + str(cls))
    return result


def connect(source, target, pin):
    names = list(editing.get_material_expression_input_names(target))
    actual = pin if pin in names else (names[0] if len(names) == 1 else None)
    if actual is None or not editing.connect_material_expressions(source, '', target, actual):
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


marker_path = root + '/Art/Materials/Enemies/M_RobotSummonMarker'
marker = unreal.load_asset(marker_path)
if marker is None:
    marker = tools.create_asset('M_RobotSummonMarker', root + '/Art/Materials/Enemies',
                                unreal.Material, unreal.MaterialFactoryNew())
    if not marker:
        raise RuntimeError('Could not create summon marker')
    marker.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
    marker.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    uv = node(marker, unreal.MaterialExpressionTextureCoordinate)
    center = node(marker, unreal.MaterialExpressionConstant2Vector)
    center.set_editor_property('r', 0.5)
    center.set_editor_property('g', 0.5)
    offset = binary(marker, unreal.MaterialExpressionSubtract, uv, center)
    square = binary(marker, unreal.MaterialExpressionDotProduct, offset, offset)
    distance = unary(marker, unreal.MaterialExpressionSquareRoot, square)
    progress = scalar(marker, 'MarkerProgress', 0.0)
    shrinking = binary(marker, unreal.MaterialExpressionMultiply, progress, constant=-0.2)
    radius = binary(marker, unreal.MaterialExpressionAdd, shrinking, constant=0.45)
    delta = binary(marker, unreal.MaterialExpressionSubtract, distance, radius)
    absolute = unary(marker, unreal.MaterialExpressionAbs, delta)
    thickness = binary(marker, unreal.MaterialExpressionMultiply, absolute, constant=80.0)
    ring = unary(marker, unreal.MaterialExpressionSaturate,
                 unary(marker, unreal.MaterialExpressionOneMinus, thickness))
    lines = []
    for channel in ('r', 'g'):
        mask = node(marker, unreal.MaterialExpressionComponentMask)
        for name in ('r', 'g', 'b', 'a'):
            mask.set_editor_property(name, name == channel)
        connect(offset, mask, 'Input')
        width = binary(marker, unreal.MaterialExpressionMultiply,
                       unary(marker, unreal.MaterialExpressionAbs, mask), constant=100.0)
        lines.append(unary(marker, unreal.MaterialExpressionSaturate,
                           unary(marker, unreal.MaterialExpressionOneMinus, width)))
    cross = binary(marker, unreal.MaterialExpressionMax, lines[0], lines[1])
    clipping = unary(marker, unreal.MaterialExpressionSaturate,
                     unary(marker, unreal.MaterialExpressionOneMinus,
                           binary(marker, unreal.MaterialExpressionMultiply, distance, constant=2.2)))
    cross = binary(marker, unreal.MaterialExpressionMultiply, cross, clipping)
    coverage = binary(marker, unreal.MaterialExpressionMax, ring, cross)
    opacity = binary(marker, unreal.MaterialExpressionMultiply, coverage,
                     scalar(marker, 'MarkerOpacity', 1.0))
    opacity = binary(marker, unreal.MaterialExpressionMultiply, opacity, constant=0.7)
    color = node(marker, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(0.05, 2.0, 2.4))
    emissive = binary(marker, unreal.MaterialExpressionMultiply, color, opacity)
    for expression, prop in ((emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR),
                             (opacity, unreal.MaterialProperty.MP_OPACITY)):
        if not editing.connect_material_property(expression, '', prop):
            raise RuntimeError('Could not connect summon marker output')
    editing.recompile_material(marker)
    save(marker)

spawn_path = root + '/Art/Materials/Characters/M_RobotMinionSpawn'
spawn = unreal.load_asset(spawn_path)
if spawn is None:
    spawn = lib.duplicate_asset(base_spawn.get_path_name().split('.')[0], spawn_path)
    if not isinstance(spawn, unreal.Material):
        raise RuntimeError('Could not duplicate the assembly material')
    # Keep the original height/dither graph, adding an optional completion flash.
    existing = editing.get_material_property_input_node(spawn, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not existing:
        raise RuntimeError('Base spawn emissive graph is missing')
    flash = scalar(spawn, 'SpawnFlash', 0.0)
    flash_color = node(spawn, unreal.MaterialExpressionConstant3Vector)
    flash_color.set_editor_property('constant', unreal.LinearColor(0.2, 5.0, 6.0))
    flash_emissive = binary(spawn, unreal.MaterialExpressionMultiply, flash_color, flash)
    combined = binary(spawn, unreal.MaterialExpressionAdd, existing, flash_emissive)
    if not editing.connect_material_property(combined, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError('Could not connect assembly flash')
    editing.recompile_material(spawn)
    save(spawn)

settings.set_editor_property('marker_material', marker)
settings.set_editor_property('pulse_material', spawn)
sound = unreal.load_asset(root + '/Audio/SFX/Combat/Parry/Combo/SFX_ParryCombo_01')
settings.set_editor_property('summon_sound', sound)
save(settings)
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
found = False
for handle in subsystem.k2_gather_subobject_data_for_blueprint(minion):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, minion)
    if isinstance(component, unreal.RLEnemySpawnVisualComponent):
        component.set_editor_property('spawn_material', spawn)
        component.set_editor_property('spawn_duration', 0.5)
        component.set_editor_property('override_spawn_color', True)
        component.set_editor_property('spawn_color', unreal.LinearColor(0.02, 0.8, 1.0))
        component.set_editor_property('completion_flash_duration', 0.08)
        found = True
if not found:
    raise RuntimeError('Minion spawn component missing')
unreal.BlueprintEditorLibrary.compile_blueprint(minion)
save(minion)
defaults = unreal.get_default_object(minion.generated_class())
spawn_component = defaults.get_component_by_class(unreal.RLEnemySpawnVisualComponent)
if spawn_component.get_editor_property('spawn_material') != spawn:
    raise RuntimeError('Minion assembly material assignment was not retained')
unreal.log('[SummonPresentation] SUCCESS: cyan marker, assembly, completion flash and sound')
