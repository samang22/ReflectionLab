"""Update only the generated laser decal: long sides fill toward the centerline."""
import unreal

path = '/Game/ReflectionLab/Art/Materials/Enemies/M_BossLaserDecal'
material = unreal.load_asset(path)
if not isinstance(material, unreal.Material):
    raise RuntimeError('Run SetupRobotBossLaser.py first; laser decal material missing')
editing = unreal.MaterialEditingLibrary


def node(cls):
    result = editing.create_material_expression(material, cls)
    if not result:
        raise RuntimeError('Could not create material expression')
    return result


def connect(source, target, pin):
    names = list(editing.get_material_expression_input_names(target))
    actual = pin if pin in names else (names[0] if len(names) == 1 else None)
    if actual is None or not editing.connect_material_expressions(source, '', target, actual):
        raise RuntimeError('Could not connect material input: ' + pin)


# Preserve existing expressions: deleting rooted expressions can assert in commandlets.
# Reconnect the outputs to the new graph instead.
material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
material.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
uv = node(unreal.MaterialExpressionTextureCoordinate)
# Deferred decals map UV = local Z,Y. DecalSize.Y is the laser width,
# so V (green) spans its narrow axis; U must not influence the fill.
mask = node(unreal.MaterialExpressionComponentMask)
for name in ('r', 'g', 'b', 'a'):
    mask.set_editor_property(name, name == 'g')
connect(uv, mask, 'Input')
offset = node(unreal.MaterialExpressionSubtract)
offset.set_editor_property('const_b', 0.5)
connect(mask, offset, 'A')
absolute = node(unreal.MaterialExpressionAbs)
connect(offset, absolute, 'Input')
distance = node(unreal.MaterialExpressionMultiply)
distance.set_editor_property('const_b', 2.0)
connect(absolute, distance, 'A')
progress = node(unreal.MaterialExpressionScalarParameter)
progress.set_editor_property('parameter_name', 'FillProgress')
progress.set_editor_property('default_value', 0.0)
threshold = node(unreal.MaterialExpressionOneMinus)
connect(progress, threshold, 'Input')
fill_distance = node(unreal.MaterialExpressionSubtract)
connect(distance, fill_distance, 'A')
connect(threshold, fill_distance, 'B')
soft_fill = node(unreal.MaterialExpressionMultiply)
soft_fill.set_editor_property('const_b', 100.0)
connect(fill_distance, soft_fill, 'A')
fill_bias = node(unreal.MaterialExpressionAdd)
fill_bias.set_editor_property('const_b', 1.0)
connect(soft_fill, fill_bias, 'A')
fill = node(unreal.MaterialExpressionSaturate)
connect(fill_bias, fill, 'Input')
# Persistent long-side borders mark the danger width at progress zero.
border_distance = node(unreal.MaterialExpressionSubtract)
border_distance.set_editor_property('const_b', 0.96)
connect(distance, border_distance, 'A')
border_gain = node(unreal.MaterialExpressionMultiply)
border_gain.set_editor_property('const_b', 100.0)
connect(border_distance, border_gain, 'A')
border = node(unreal.MaterialExpressionSaturate)
connect(border_gain, border, 'Input')
coverage = node(unreal.MaterialExpressionMax)
connect(fill, coverage, 'A')
connect(border, coverage, 'B')
opacity = node(unreal.MaterialExpressionMultiply)
opacity.set_editor_property('const_b', 0.65)
connect(coverage, opacity, 'A')
color = node(unreal.MaterialExpressionConstant3Vector)
color.set_editor_property('constant', unreal.LinearColor(2.0, 0.04, 0.015))
emissive = node(unreal.MaterialExpressionMultiply)
connect(color, emissive, 'A')
connect(opacity, emissive, 'B')
for expression, prop in ((emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR),
                         (opacity, unreal.MaterialProperty.MP_OPACITY)):
    if not editing.connect_material_property(expression, '', prop):
        raise RuntimeError('Could not connect laser decal output')
editing.recompile_material(material)
if not unreal.EditorAssetLibrary.save_loaded_asset(material):
    raise RuntimeError('Could not save laser decal')
unreal.log('[BossLaserDecal] SUCCESS: FillProgress long-sides-to-centerline fill')
