"""Run in Unreal after native rebuild/restart. Creates the round-five boss assets."""
import unreal

# Fail before touching assets if the editor still has the previous DLL loaded.
for required in ("RLRobotBossCharacter", "RLBossOverloadComponent", "RLBossOverloadDataAsset"):
    if not hasattr(unreal, required):
        raise RuntimeError("Rebuild and restart Unreal first; native class missing: " + required)
lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
editing = unreal.MaterialEditingLibrary
root = "/Game/ReflectionLab"
material_path = root + "/Art/Materials/Enemies/M_BossAbsorption"
material = unreal.load_asset(material_path)
if material is None:
    material = tools.create_asset("M_BossAbsorption", root + "/Art/Materials/Enemies", unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create absorption decal material")
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
    uv = editing.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate)
    center = editing.create_material_expression(material, unreal.MaterialExpressionConstant2Vector)
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)
    mask = editing.create_material_expression(material, unreal.MaterialExpressionSphereMask)
    mask.set_editor_property("attenuation_radius", 0.5)
    mask.set_editor_property("hardness_percent", 95.0)
    if not editing.connect_material_expressions(uv, "", mask, "A") or not editing.connect_material_expressions(center, "", mask, "B"):
        raise RuntimeError("Could not connect decal circle mask")
    opacity = editing.create_material_expression(material, unreal.MaterialExpressionMultiply)
    opacity.set_editor_property("const_b", 0.45)
    if not editing.connect_material_expressions(mask, "", opacity, "A"):
        raise RuntimeError("Could not connect decal opacity")
    color = editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property("constant", unreal.LinearColor(1.8, 0.06, 3.0))
    if not editing.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY) or not editing.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect absorption decal outputs")
    editing.recompile_material(material)
    if not lib.save_loaded_asset(material):
        raise RuntimeError("Could not save decal material")

# Repair the unsupported legacy blend mode on previously generated assets too.
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
emissive = editing.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
opacity = editing.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY)
if isinstance(emissive, unreal.MaterialExpressionConstant3Vector) and opacity:
    premultiplied = editing.create_material_expression(material, unreal.MaterialExpressionMultiply)
    if not editing.connect_material_expressions(emissive, "", premultiplied, "A") or not editing.connect_material_expressions(opacity, "", premultiplied, "B") or not editing.connect_material_property(premultiplied, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect premultiplied decal emission")
editing.recompile_material(material)
if not lib.save_loaded_asset(material, False):
    raise RuntimeError("Could not save repaired absorption material")

settings_path = root + "/Data/Enemies/DA_RobotBossOverload"
settings = unreal.load_asset(settings_path)
if settings is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.RLBossOverloadDataAsset.static_class())
    settings = tools.create_asset("DA_RobotBossOverload", root + "/Data/Enemies", unreal.RLBossOverloadDataAsset, factory)
    if not settings:
        raise RuntimeError("Could not create overload tuning")
projectile = unreal.load_asset(root + "/Data/Projectiles/DA_Projectile_Normal")
if not projectile:
    raise RuntimeError("Normal projectile definition missing")
settings.set_editor_property("projectile_definition", projectile)
settings.set_editor_property("duration", 5.0)
settings.set_editor_property("vulnerable_duration", 5.0)
settings.set_editor_property("decal_material", material)
if not lib.save_loaded_asset(settings):
    raise RuntimeError("Could not save overload tuning")

boss_path = root + "/Gameplay/Enemies/BP_RLRobotBoss"
boss = unreal.load_asset(boss_path)
if boss is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.RLRobotBossCharacter.static_class())
    boss = tools.create_asset("BP_RLRobotBoss", root + "/Gameplay/Enemies", unreal.Blueprint, factory)
mesh = unreal.load_asset(root + "/Art/Characters/Robot/robot/SkeletalMeshes/robot")
projectile_class = lib.load_blueprint_class(root + "/Gameplay/Projectiles/BP_RLProjectile")
if not boss or not mesh or not projectile_class:
    raise RuntimeError("Boss, robot mesh or projectile BP missing")
defaults = unreal.get_default_object(boss.generated_class())
defaults.set_editor_property("projectile_class", projectile_class)
robot_class = lib.load_blueprint_class(root + "/Gameplay/Enemies/BP_RLRobotEnemy")
if not robot_class:
    raise RuntimeError("Existing robot enemy BP missing")
robot_defaults = unreal.get_default_object(robot_class)
defaults.set_editor_property("combat_config", robot_defaults.get_editor_property("combat_config"))
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
configured_mesh = False
configured_overload = False
for handle in subsystem.k2_gather_subobject_data_for_blueprint(boss):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, boss)
    if isinstance(component, unreal.SkeletalMeshComponent):
        component.set_skeletal_mesh_asset(mesh)
        component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
        component.set_anim_instance_class(unreal.RLRobotEnemyAnimInstance)
        component.set_editor_property("relative_rotation", unreal.Rotator(0, 0, 0))
        component.set_editor_property("relative_location", unreal.Vector(0, 0, -88))
        configured_mesh = True
    elif isinstance(component, unreal.RLBossOverloadComponent):
        component.set_editor_property("settings", settings)
        configured_overload = True
    elif isinstance(component, unreal.RLEnemyMovementComponent):
        component.set_editor_property("movement_settings", robot_defaults.get_component_by_class(unreal.RLEnemyMovementComponent).get_editor_property("movement_settings"))
    elif isinstance(component, unreal.RLEnemySpawnVisualComponent):
        original = robot_defaults.get_component_by_class(unreal.RLEnemySpawnVisualComponent)
        component.set_editor_property("spawn_material", original.get_editor_property("spawn_material"))
        component.set_editor_property("spawn_duration", original.get_editor_property("spawn_duration"))
if not configured_mesh or not configured_overload:
    raise RuntimeError("Boss mesh or overload component template missing")
unreal.BlueprintEditorLibrary.compile_blueprint(boss)
if not lib.save_loaded_asset(boss):
    raise RuntimeError("Could not save boss blueprint")
run = unreal.load_asset(root + "/Data/Difficulty/DA_DefaultRun")
if not run:
    raise RuntimeError("Default run missing")
classes = dict(run.get_editor_property("round_enemy_classes"))
counts = dict(run.get_editor_property("round_enemy_counts"))
single_waves = set(run.get_editor_property("single_wave_rounds"))
classes[5] = boss.generated_class()
counts[5] = 1
single_waves.add(5)
run.set_editor_property("round_enemy_classes", classes)
run.set_editor_property("round_enemy_counts", counts)
run.set_editor_property("single_wave_rounds", single_waves)
if not lib.save_loaded_asset(run):
    raise RuntimeError("Could not save round-five boss assignment")
defaults = unreal.get_default_object(boss.generated_class())
if defaults.get_component_by_class(unreal.RLBossOverloadComponent).get_editor_property("settings") != settings:
    raise RuntimeError("Boss overload setting was not retained")
if run.get_editor_property("round_enemy_counts")[5] != 1:
    raise RuntimeError("Boss encounter count was not retained")
unreal.log("[RobotBoss] SUCCESS: round 5, one boss, 20 HP, overload tuning assigned")
