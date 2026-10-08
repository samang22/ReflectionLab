"""After rebuild/restart, configure dedicated fade materials for boss/minion lasers."""
import unreal

root = '/Game/ReflectionLab'
lib = unreal.EditorAssetLibrary
editing = unreal.MaterialEditingLibrary
settings_list = [unreal.load_asset(root + '/Data/Enemies/' + name)
                 for name in ('DA_RobotBossLaser', 'DA_RobotMinionLaser')]
settings_list = [settings for settings in settings_list if settings]
if not settings_list:
    raise RuntimeError('Laser settings assets missing')
for settings in settings_list:
    settings.get_editor_property('fade_out_duration')


def save(asset):
    if not lib.save_loaded_asset(asset):
        raise RuntimeError('Could not save: ' + asset.get_path_name())


def fade_material(source, name, decal):
    if not source:
        raise RuntimeError('Laser material missing')
    path = root + '/Art/Materials/Enemies/' + name
    result = unreal.load_asset(path)
    if result:
        return result
    base = source
    while isinstance(base, unreal.MaterialInstanceConstant):
        base = base.get_editor_property('parent')
    if not isinstance(base, unreal.Material):
        raise RuntimeError('Unsupported laser material parent')
    material = lib.duplicate_asset(base.get_path_name().split('.')[0], path + '_Base')
    if not isinstance(material, unreal.Material):
        raise RuntimeError('Could not duplicate laser material')
    if not decal:
        material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    alpha = editing.create_material_expression(material, unreal.MaterialExpressionScalarParameter)
    alpha.set_editor_property('parameter_name', 'LaserOpacity')
    alpha.set_editor_property('default_value', 1.0)
    # Fade both opacity and light intensity, retaining all other material outputs.
    for prop in (unreal.MaterialProperty.MP_OPACITY, unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        existing = editing.get_material_property_input_node(material, prop)
        output = editing.get_material_property_input_node_output_name(material, prop) if existing else ''
        if not existing:
            if prop == unreal.MaterialProperty.MP_EMISSIVE_COLOR:
                continue
            existing = editing.create_material_expression(material, unreal.MaterialExpressionConstant)
            existing.set_editor_property('r', 1.0)
        multiply = editing.create_material_expression(material, unreal.MaterialExpressionMultiply)
        if not (editing.connect_material_expressions(existing, output, multiply, 'A')
                and editing.connect_material_expressions(alpha, '', multiply, 'B')
                and editing.connect_material_property(multiply, '', prop)):
            raise RuntimeError('Could not connect laser fade output')
    editing.recompile_material(material)
    save(material)
    # Duplicate the original instance to preserve its color/texture parameter overrides.
    if isinstance(source, unreal.MaterialInstanceConstant):
        result = lib.duplicate_asset(source.get_path_name().split('.')[0], path)
        editing.set_material_instance_parent(result, material)
        editing.update_material_instance(result)
    else:
        result = lib.duplicate_asset(material.get_path_name().split('.')[0], path)
    if not result:
        raise RuntimeError('Could not create laser fade material')
    save(result)
    return result


for index, settings in enumerate(settings_list):
    for prop, suffix, decal in (('beam_material', 'Beam', False), ('decal_material', 'Decal', True)):
        source = settings.get_editor_property(prop)
        material = fade_material(source, 'M_LaserFade_' + suffix + '_' + str(index), decal)
        settings.set_editor_property(prop, material)
    save(settings)
unreal.log('[LaserFade] SUCCESS: dedicated boss/minion laser fade materials configured')
