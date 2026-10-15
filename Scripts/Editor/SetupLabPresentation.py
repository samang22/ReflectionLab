"""Original clean-room art pass. Decorative actors never participate in combat collision."""
import unreal

ROOT = '/Game/ReflectionLab/Art/Materials/World'
MAPS = ['/Game/ReflectionLab/Maps/Main', '/Game/ReflectionLab/Maps/MainMenu']
lib = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def expression(material, kind):
    return lib.create_material_expression(material, kind)


def scalar(material, name, value):
    node = expression(material, unreal.MaterialExpressionScalarParameter)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    return node


def vector(material, name, values):
    node = expression(material, unreal.MaterialExpressionVectorParameter)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', unreal.LinearColor(*values, 1.0))
    return node


def connect(source, target, pin):
    if not lib.connect_material_expressions(source, '', target, pin):
        raise RuntimeError('Cannot connect material pin ' + pin)


def new_material(name):
    path = ROOT + '/' + name
    material = unreal.load_asset(path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Cannot create ' + path)
    lib.delete_all_material_expressions(material)
    return material


def save_material(material):
    lib.recompile_material(material)
    if not assets.save_loaded_asset(material, False):
        raise RuntimeError('Cannot save ' + material.get_path_name())


def panel_material(name, u_axis, v_axis, color, seam, size):
    material = new_material(name)
    position = expression(material, unreal.MaterialExpressionWorldPosition)
    panel_size = scalar(material, 'PanelSize', size)
    seam_width = scalar(material, 'SeamWidth', 2.0)
    width = expression(material, unreal.MaterialExpressionDivide)
    connect(seam_width, width, 'A')
    connect(panel_size, width, 'B')
    masks = []
    for index, axis in enumerate([u_axis, v_axis]):
        direction = vector(material, 'Projection' + str(index), axis)
        dot = expression(material, unreal.MaterialExpressionDotProduct)
        connect(position, dot, 'A')
        connect(direction, dot, 'B')
        divide = expression(material, unreal.MaterialExpressionDivide)
        connect(dot, divide, 'A')
        connect(panel_size, divide, 'B')
        fraction = expression(material, unreal.MaterialExpressionFrac)
        connect(divide, fraction, '')
        edge = expression(material, unreal.MaterialExpressionIf)
        connect(fraction, edge, 'A')
        connect(width, edge, 'B')
        connect(scalar(material, 'PanelMask' + str(index), 0.0), edge, 'A > B')
        connect(scalar(material, 'SeamMask' + str(index), 1.0), edge, 'A < B')
        connect(scalar(material, 'SeamEqual' + str(index), 1.0), edge, 'A == B')
        masks.append(edge)
    mask = expression(material, unreal.MaterialExpressionMax)
    connect(masks[0], mask, 'A')
    connect(masks[1], mask, 'B')
    blend = expression(material, unreal.MaterialExpressionLinearInterpolate)
    connect(vector(material, 'PanelColor', color), blend, 'A')
    connect(vector(material, 'SeamColor', seam), blend, 'B')
    connect(mask, blend, 'Alpha')
    lib.connect_material_property(blend, '', unreal.MaterialProperty.MP_BASE_COLOR)
    lib.connect_material_property(scalar(material, 'Roughness', 0.85), '', unreal.MaterialProperty.MP_ROUGHNESS)
    save_material(material)
    return material


def solid_material(name, color, roughness=0.85):
    material = new_material(name)
    lib.connect_material_property(vector(material, 'SurfaceColor', color), '', unreal.MaterialProperty.MP_BASE_COLOR)
    lib.connect_material_property(scalar(material, 'Roughness', roughness), '', unreal.MaterialProperty.MP_ROUGHNESS)
    save_material(material)
    return material


def box(label, position, dimensions, material):
    actor = next((item for item in actors.get_all_level_actors() if item.get_actor_label() == label), None)
    if not actor:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*position))
    if not actor:
        raise RuntimeError('Cannot spawn ' + label)
    actor.set_actor_label(label)
    actor.set_folder_path('Presentation/Lab')
    actor.set_actor_location(unreal.Vector(*position), False, False)
    actor.set_actor_scale3d(unreal.Vector(*(value / 100.0 for value in dimensions)))
    component = actor.static_mesh_component
    actor.modify()
    component.modify()
    component.set_static_mesh(cube)
    component.set_material(0, material)
    component.set_collision_profile_name('NoCollision')
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.set_actor_enable_collision(False)
    component.set_editor_property('cast_shadow', False)
    return actor


def sign(label, text, position, size, floor=False):
    actor = next((item for item in actors.get_all_level_actors() if item.get_actor_label() == label), None)
    if not actor:
        actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*position))
    actor.set_actor_label(label)
    actor.set_folder_path('Presentation/Lab')
    actor.set_actor_location(unreal.Vector(*position), False, False)
    actor.set_actor_rotation(unreal.Rotator(pitch=90, yaw=-135, roll=0) if floor
                             else unreal.Rotator(pitch=0, yaw=-90, roll=0), False)
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    actor.modify()
    component.modify()
    component.set_text(text)
    component.set_world_size(size)
    component.set_text_render_color(unreal.Color(r=30, g=65, b=70, a=255))
    component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.set_actor_enable_collision(False)


assets.make_directory(ROOT)
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
floor_material = panel_material('M_LabFloorPanels', (1, 0, 0), (0, 1, 0), (0.25, 0.27, 0.29), (0.13, 0.15, 0.17), 400.0)
wall_material = panel_material('M_LabWallPanels', (1, 1, 0), (0, 0, 1), (0.55, 0.57, 0.59), (0.28, 0.30, 0.32), 400.0)
dark = solid_material('M_LabObservation', (0.012, 0.020, 0.026), 0.55)
frame = solid_material('M_LabFrame', (0.08, 0.10, 0.12))
accent = solid_material('M_LabGuideCyan', (0.06, 0.35, 0.40))
white = solid_material('M_LabWhiteTrim', (0.65, 0.67, 0.69))
neutral_cube = unreal.load_asset('/Game/ReflectionLab/Art/Textures/World/T_AmbientNeutral')
if not cube or not neutral_cube:
    raise RuntimeError('Required cube mesh/neutral environment missing')
for map_path in MAPS:
    if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
        raise RuntimeError('Cannot load ' + map_path)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.StaticMeshActor) and actor.get_actor_label() == 'Floor':
            actor.modify()
            actor.static_mesh_component.modify()
            actor.static_mesh_component.set_material(0, floor_material)
        elif isinstance(actor, unreal.StaticMeshActor):
            component = actor.static_mesh_component
            if any(component.get_material(i) and component.get_material(i).get_path_name().startswith(ROOT + '/MI_ArenaWall') for i in range(component.get_num_materials())):
                actor.modify()
                component.modify()
                component.set_material(0, wall_material)
        elif isinstance(actor, unreal.DirectionalLight):
            light = actor.get_component_by_class(unreal.DirectionalLightComponent)
            light.set_light_color(unreal.LinearColor(1, 1, 1, 1))
            light.set_editor_property('intensity', 0.30)
    sky = next((actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.SkyLight)), None)
    if not sky:
        sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
        sky.set_actor_label('SkyLight_AmbientFill')
    light = sky.get_component_by_class(unreal.SkyLightComponent)
    light.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    light.set_editor_property('source_type', unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    light.set_editor_property('cubemap', neutral_cube)
    light.set_light_color(unreal.LinearColor(1, 1, 1, 1))
    light.set_editor_property('intensity', 0.35)
    light.set_editor_property('cast_shadows', False)
    light.set_editor_property('lower_hemisphere_is_black', False)
    for y in [-1800, 1800]:
        box('Lab_GuideY_' + str(y), (0, y, 0.5), (3600, 6, 1), accent)
    for x in [-1800, 1800]:
        box('Lab_GuideX_' + str(x), (x, 0, 0.5), (6, 3600, 1), accent)
    # A one-way observation surface, flush inside the existing northern boundary.
    box('Lab_ObservationPane', (0, 1945, 285), (1100, 8, 230), dark)
    for x in [-565, 565]:
        box('Lab_ObservationSide_' + str(x), (x, 1940, 285), (25, 20, 270), frame)
    for z in [155, 415]:
        box('Lab_ObservationRail_' + str(z), (0, 1940, z), (1155, 20, 25), frame)
    box('Lab_ObservationStatus', (0, 1928, 145), (1100, 4, 4), accent)
    box('Lab_ServiceHatch', (1450, 1944, 240), (240, 12, 330), frame)
    box('Lab_ServiceHatchInset', (1450, 1936, 240), (205, 3, 285), wall_material)
    for x in [-1200, 1200]:
        box('Lab_LightHousing_' + str(x), (x, 1930, 475), (500, 35, 15), white)
    sign('Lab_ExperimentFloor', 'RL / 01\nREFLECTION VALIDATION', (-650, 750, 1.5), 50, True)
    sign('Lab_BoundaryFloor', 'AUTHORIZED TEST AREA', (650, -750, 1.5), 28, True)
    sign('Lab_ObservationLabel', 'OBSERVATION / 01', (0, 1923, 470), 32)
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError('Cannot save ' + map_path)
    unreal.log('LAB_PRESENTATION_SAVED ' + map_path)
