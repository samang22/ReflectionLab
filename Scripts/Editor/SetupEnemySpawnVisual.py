import unreal


PATH = "/Game/ReflectionLab/Art/Materials/Characters/M_EnemySpawn"
ENEMIES = [
    "/Game/ReflectionLab/Gameplay/Enemies/BP_RLEnemyCharacter",
    "/Game/ReflectionLab/Gameplay/Enemies/BP_RLEnemyBurstShooter",
]
lib = unreal.MaterialEditingLibrary


def node(cls):
    return lib.create_material_expression(material, cls)


def scalar(name, value):
    result = node(unreal.MaterialExpressionScalarParameter)
    result.set_editor_property("parameter_name", name)
    result.set_editor_property("default_value", value)
    return result


def connect(source, target, pin, output=""):
    inputs = list(lib.get_material_expression_input_names(target))
    actual_pin = next((name for name in inputs
                       if name.replace(" ", "").lower() == pin.replace(" ", "").lower()), None)
    # Unary expressions such as ComponentMask expose an unnamed input in UE 5.8.
    if actual_pin is None and len(inputs) == 1:
        actual_pin = inputs[0]
    if actual_pin is None or not lib.connect_material_expressions(source, output, target, actual_pin):
        raise RuntimeError("Could not connect {} input '{}'; available: {}".format(
            target.get_class().get_name(), pin, inputs))


# Requires a native rebuild and editor restart before running.
spawn_class = unreal.RLEnemySpawnVisualComponent
material = unreal.load_asset(PATH)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_EnemySpawn", "/Game/ReflectionLab/Art/Materials/Characters",
        unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create spawn material")
if not isinstance(material, unreal.Material):
    raise RuntimeError("Spawn material path contains the wrong asset type")
# Rebuild only an incomplete generated graph left behind by a failed setup.
if any(lib.get_material_property_input_node(material, prop) is None for prop in (
        unreal.MaterialProperty.MP_OPACITY_MASK, unreal.MaterialProperty.MP_BASE_COLOR,
        unreal.MaterialProperty.MP_EMISSIVE_COLOR)):
    lib.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    lib.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    world = node(unreal.MaterialExpressionWorldPosition)
    z = node(unreal.MaterialExpressionComponentMask)
    for channel in ("r", "g", "a"):
        z.set_editor_property(channel, False)
    z.set_editor_property("b", True)
    connect(world, z, "Input")
    bottom = scalar("SpawnBottom", 0.0)
    height = scalar("SpawnHeight", 180.0)
    progress = scalar("SpawnProgress", 1.0)
    offset = node(unreal.MaterialExpressionSubtract)
    connect(z, offset, "A")
    connect(bottom, offset, "B")
    normalized = node(unreal.MaterialExpressionDivide)
    connect(offset, normalized, "A")
    connect(height, normalized, "B")
    distance = node(unreal.MaterialExpressionSubtract)
    connect(progress, distance, "A")
    connect(normalized, distance, "B")
    # Narrow soft band: invisible above, dithered transition, solid below.
    gain = node(unreal.MaterialExpressionMultiply)
    gain.set_editor_property("const_b", 16.0)
    connect(distance, gain, "A")
    opacity = node(unreal.MaterialExpressionSaturate)
    connect(gain, opacity, "Input")
    dither = node(unreal.MaterialExpressionMaterialFunctionCall)
    dither_function = unreal.load_asset(
        "/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA")
    if not dither_function or not dither.set_material_function(dither_function):
        raise RuntimeError("Could not load DitherTemporalAA material function")
    connect(opacity, dither, "Alpha Threshold")
    lib.connect_material_property(dither, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    color = node(unreal.MaterialExpressionVectorParameter)
    color.set_editor_property("parameter_name", "SpawnColor")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.04, 0.015, 1.0))
    lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    absolute = node(unreal.MaterialExpressionAbs)
    connect(distance, absolute, "Input")
    edge_scale = node(unreal.MaterialExpressionMultiply)
    edge_scale.set_editor_property("const_b", 20.0)
    connect(absolute, edge_scale, "A")
    invert = node(unreal.MaterialExpressionOneMinus)
    connect(edge_scale, invert, "Input")
    edge = node(unreal.MaterialExpressionSaturate)
    connect(invert, edge, "Input")
    glow = node(unreal.MaterialExpressionMultiply)
    glow.set_editor_property("const_b", 12.0)
    connect(edge, glow, "A")
    tinted = node(unreal.MaterialExpressionMultiply)
    connect(color, tinted, "A")
    connect(glow, tinted, "B")
    lib.connect_material_property(tinted, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(material)
if not isinstance(material, unreal.Material):
    raise RuntimeError("Spawn material path contains the wrong asset type")
if not unreal.EditorAssetLibrary.save_loaded_asset(material):
    raise RuntimeError("Could not save spawn material")

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for path in ENEMIES:
    asset = unreal.load_asset(path)
    if not isinstance(asset, unreal.Blueprint):
        raise RuntimeError("Enemy Blueprint missing: " + path)
    assigned = False
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(asset):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, asset)
        if isinstance(component, spawn_class):
            component.set_editor_property("spawn_material", material)
            assigned = True
    if not assigned:
        raise RuntimeError("Spawn component missing; rebuild and restart editor: " + path)
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError("Could not save enemy Blueprint: " + path)
unreal.log("[EnemySpawn] Bottom-to-top spawn effect configured.")
