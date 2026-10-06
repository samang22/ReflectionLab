import unreal


MAPS = [
    "/Game/ReflectionLab/Maps/Main",
    "/Game/ReflectionLab/Maps/MainMenu",
]


for map_path in MAPS:
    if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
        raise RuntimeError("Could not load map: " + map_path)

    changed = 0
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        for component in actor.get_components_by_class(unreal.LightComponent):
            if isinstance(actor, unreal.DirectionalLight):
                intensity = 0.30
            elif isinstance(actor, unreal.SkyLight):
                intensity = 0.12
            elif actor.get_actor_label() in ("NeonKey_Cyan", "NeonKey_Magenta"):
                intensity = 175.0
            else:
                continue

            previous = component.get_editor_property("intensity")
            component.set_editor_property("intensity", intensity)
            unreal.log("[DimArena] {} {}: {} -> {}".format(
                map_path, actor.get_actor_label(), previous, intensity))
            changed += 1

    if changed == 0:
        raise RuntimeError("No arena lights found in: " + map_path)
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError("Could not save map: " + map_path)
    unreal.log("[DimArena] Saved {} ({} lights)".format(map_path, changed))
