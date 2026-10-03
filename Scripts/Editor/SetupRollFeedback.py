"""Run once after a full C++ build: Tools > Execute Python Script.

Enable skeletal-mesh shaders on the existing translucent projectile material.
Does not modify player tuning or animation assets.
"""
import unreal

MATERIAL_PATH = "/Game/ReflectionLab/Art/Materials/Projectiles/M_MasterProjectile"
material = unreal.load_asset(MATERIAL_PATH)
if not isinstance(material, unreal.Material):
    raise RuntimeError("Projectile master material was not found")

unreal.MaterialEditingLibrary.set_material_usage(
    material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH
)
if not material.get_editor_property("used_with_skeletal_mesh"):
    raise RuntimeError("Skeletal mesh material usage was not enabled")
unreal.MaterialEditingLibrary.recompile_material(material)
if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
    raise RuntimeError("Could not save the projectile master material")
unreal.log("ROLL_FEEDBACK_MATERIAL_READY")
