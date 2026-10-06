import unreal


WIDGET_PATH = "/Game/ReflectionLab/UI/WBP_OffscreenEnemy"
CONTROLLER_PATH = "/Game/ReflectionLab/Framework/Controllers/BP_RLPlayerController"


def setup():
    controller = unreal.load_asset(CONTROLLER_PATH)
    if not isinstance(controller, unreal.Blueprint):
        raise RuntimeError("Player controller Blueprint not found: " + CONTROLLER_PATH)
    controller_defaults = unreal.get_default_object(controller.generated_class())
    # Validate the rebuilt native property before creating or saving assets.
    controller_defaults.get_editor_property("offscreen_enemy_widget_class")

    widget = unreal.load_asset(WIDGET_PATH)
    created = widget is None
    if created:
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", unreal.RLOffscreenEnemyWidget)
        widget = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "WBP_OffscreenEnemy", "/Game/ReflectionLab/UI",
            unreal.WidgetBlueprint, factory)
    if not isinstance(widget, unreal.WidgetBlueprint):
        raise RuntimeError("Could not create or load widget Blueprint: " + WIDGET_PATH)
    unreal.BlueprintEditorLibrary.compile_blueprint(widget)
    widget_class = widget.generated_class()
    if not widget_class:
        raise RuntimeError("Widget Blueprint has no generated class")
    defaults = unreal.get_default_object(widget_class)
    if not isinstance(defaults, unreal.RLOffscreenEnemyWidget):
        raise RuntimeError("Widget must inherit from RLOffscreenEnemyWidget")
    if created:
        defaults.set_editor_property("arrow_size", 24.0)
        defaults.set_editor_property("arrow_thickness", 4.0)
    if not unreal.EditorAssetLibrary.save_loaded_asset(widget):
        raise RuntimeError("Could not save widget Blueprint")

    previous_class = controller_defaults.get_editor_property("offscreen_enemy_widget_class")
    try:
        controller_defaults.set_editor_property("offscreen_enemy_widget_class", widget_class)
        unreal.BlueprintEditorLibrary.compile_blueprint(controller)
        actual_class = unreal.get_default_object(controller.generated_class()).get_editor_property(
            "offscreen_enemy_widget_class")
        if actual_class != widget_class:
            raise RuntimeError("Controller widget assignment was not retained")
        if not unreal.EditorAssetLibrary.save_loaded_asset(controller):
            raise RuntimeError("Could not save controller Blueprint")
    except Exception:
        unreal.get_default_object(controller.generated_class()).set_editor_property(
            "offscreen_enemy_widget_class", previous_class)
        unreal.BlueprintEditorLibrary.compile_blueprint(controller)
        raise
    unreal.log("[OffscreenEnemy] Ready. Adjust Enemy Indicators in WBP_OffscreenEnemy Class Defaults.")


setup()
