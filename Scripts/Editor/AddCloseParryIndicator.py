import unreal


material = unreal.load_asset('/Game/ReflectionLab/Art/Materials/M_ParryRangeIndicator')
if not material:
    raise RuntimeError('Parry indicator material not found')
library = unreal.MaterialEditingLibrary
expressions = library.get_material_expressions(material)
if any(isinstance(node, unreal.MaterialExpressionVectorParameter)
       and str(node.get_editor_property('parameter_name')) == 'CloseIndicatorColor'
       for node in expressions):
    unreal.log('Close parry indicator already configured')
else:
    outputs = []
    for prop in (unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        node = library.get_material_property_input_node(material, prop)
        if node:
            outputs.append((prop, node, library.get_material_property_input_node_output_name(material, prop)))
    if not outputs:
        raise RuntimeError('No existing color output to extend')
    def create(cls):
        return library.create_material_expression(material, cls)
    uv = create(unreal.MaterialExpressionTextureCoordinate)
    center = create(unreal.MaterialExpressionConstant2Vector)
    center.set_editor_property('r', 0.5)
    center.set_editor_property('g', 0.5)
    radius = create(unreal.MaterialExpressionScalarParameter)
    radius.set_editor_property('parameter_name', 'CloseRadiusUV')
    radius.set_editor_property('default_value', 0.15)
    mask = create(unreal.MaterialExpressionSphereMask)
    mask.set_editor_property('hardness_percent', 100.0)
    color = create(unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name', 'CloseIndicatorColor')
    color.set_editor_property('default_value', unreal.LinearColor(0.75, 0.12, 1.0, 1.0))
    library.connect_material_expressions(uv, '', mask, 'A')
    library.connect_material_expressions(center, '', mask, 'B')
    library.connect_material_expressions(radius, '', mask, 'Radius')
    for prop, original, output in outputs:
        blend = create(unreal.MaterialExpressionLinearInterpolate)
        library.connect_material_expressions(original, output, blend, 'A')
        library.connect_material_expressions(color, '', blend, 'B')
        library.connect_material_expressions(mask, '', blend, 'Alpha')
        library.connect_material_property(blend, '', prop)
    library.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log('Added close parry color region; existing opacity and perfect region preserved')
