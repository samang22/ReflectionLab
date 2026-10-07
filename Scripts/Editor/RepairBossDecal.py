"""Repair the existing boss decal without modifying the boss BP or run settings."""
import unreal

path = "/Game/ReflectionLab/Art/Materials/Enemies/M_BossAbsorption"
material = unreal.load_asset(path)
if not isinstance(material, unreal.Material):
    raise RuntimeError("Boss absorption material missing; run SetupRobotBoss.py first")
editing = unreal.MaterialEditingLibrary
material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
emissive = editing.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
opacity = editing.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY)
if not emissive or not opacity:
    raise RuntimeError("Absorption material graph incomplete")
if isinstance(emissive, unreal.MaterialExpressionConstant3Vector):
    premultiplied = editing.create_material_expression(material, unreal.MaterialExpressionMultiply)
    if not editing.connect_material_expressions(emissive, "", premultiplied, "A") or not editing.connect_material_expressions(opacity, "", premultiplied, "B") or not editing.connect_material_property(premultiplied, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not repair premultiplied emission")
editing.recompile_material(material)
if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
    raise RuntimeError("Could not save repaired decal")
unreal.log("[BossDecalRepair] SUCCESS: AlphaComposite, premultiplied circular emission")
