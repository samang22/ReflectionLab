import unreal


FLOOR_MATERIAL = "/Game/ReflectionLab/Art/Materials/World/MI_ArenaFloor"
WALL_MATERIAL = "/Game/ReflectionLab/Art/Materials/World/MI_ArenaWall"
MAPS = [
    "/Game/ReflectionLab/Maps/Main",
    "/Game/ReflectionLab/Maps/MainMenu",
]


def set_vector_parameter(material, name, color):
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        material, name, unreal.LinearColor(*color))


def set_scalar_parameter(material, name, value):
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        material, name, value)


def configure_world_materials():
    floor = unreal.load_asset(FLOOR_MATERIAL)
    wall = unreal.load_asset(WALL_MATERIAL)

    if not floor or not wall:
        raise RuntimeError("Arena world materials could not be loaded")

    # Sterile, low-saturation tiles establish a cleanroom. The two nearby
    # values retain the grid without turning it into a decorative neon floor.
    set_vector_parameter(floor, "ColorA", (0.105, 0.125, 0.155, 1.0))
    set_vector_parameter(floor, "ColorB", (0.205, 0.240, 0.295, 1.0))
    # Matte epoxy floor: bright enough for a clinical space without glare.
    set_scalar_parameter(floor, "Roughness", 0.64)
    set_scalar_parameter(floor, "GridSize", 200.0)

    set_vector_parameter(wall, "ColorA", (0.175, 0.195, 0.225, 1.0))
    set_vector_parameter(wall, "ColorB", (0.340, 0.385, 0.455, 1.0))
    set_scalar_parameter(wall, "Roughness", 0.52)

    unreal.EditorAssetLibrary.save_loaded_asset(floor)
    unreal.EditorAssetLibrary.save_loaded_asset(wall)


def get_light_component(actor):
    for component in actor.get_components_by_class(unreal.LightComponent):
        return component
    return None


def configure_existing_lights():
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        component = get_light_component(actor)
        if not component:
            continue

        if isinstance(actor, unreal.DirectionalLight):
            component.set_editor_property("intensity", 1.20)
            component.set_light_color(unreal.LinearColor(0.78, 0.86, 1.00, 1.0))
        elif isinstance(actor, unreal.SkyLight):
            component.set_editor_property("intensity", 0.45)
            component.set_light_color(unreal.LinearColor(0.48, 0.58, 0.72, 1.0))


def find_actor_by_label(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def add_neon_key_light(label, location, color):
    actor = find_actor_by_label(label)
    if not actor:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PointLight,
            unreal.Vector(*location),
            unreal.Rotator())
        actor.set_actor_label(label)

    component = get_light_component(actor)
    component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity", 700.0)
    component.set_editor_property("attenuation_radius", 1900.0)
    component.set_editor_property("source_radius", 75.0)
    component.set_light_color(unreal.LinearColor(*color))


def configure_map(map_path):
    unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    configure_existing_lights()
    add_neon_key_light("NeonKey_Cyan", (-1050.0, 950.0, 650.0), (0.55, 0.75, 1.00, 1.0))
    add_neon_key_light("NeonKey_Magenta", (1050.0, -950.0, 650.0), (0.72, 0.82, 1.00, 1.0))
    unreal.EditorLoadingAndSavingUtils.save_current_level()


configure_world_materials()
for map_path in MAPS:
    configure_map(map_path)

unreal.log("[NeonArena] Applied neon club presentation to arena materials and maps")
