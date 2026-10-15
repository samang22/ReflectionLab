"""Add a low-intensity, global sky fill without changing existing lights/post process."""
import unreal

MAP = '/Game/ReflectionLab/Maps/Main'
LABEL = 'SkyLight_AmbientFill'
CUBEMAP = '/Game/ReflectionLab/Art/Textures/World/T_AmbientNeutral'
if not unreal.EditorLoadingAndSavingUtils.load_map(MAP):
    raise RuntimeError('Could not load ' + MAP)
cube = unreal.load_asset(CUBEMAP)
if not cube:
    unreal.EditorAssetLibrary.make_directory('/Game/ReflectionLab/Art/Textures/World')
    cube = unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineMaterials/DefaultCubemap', CUBEMAP)
if not isinstance(cube, unreal.TextureCube):
    raise RuntimeError('Runtime ambient cubemap unavailable: ' + CUBEMAP)
# A white light tint does not remove the source cubemap's warm lighting.
# Desaturate a project-owned copy, leaving engine content untouched.
cube.set_editor_property('adjust_saturation', 0.0)
if not unreal.EditorAssetLibrary.save_loaded_asset(cube, False):
    raise RuntimeError('Could not save neutral ambient cubemap')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sky = next((actor for actor in actors.get_all_level_actors()
            if isinstance(actor, unreal.SkyLight) and actor.get_actor_label() == LABEL), None)
if not sky:
    # Do not introduce a second skylight if an artist has already added one.
    existing = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.SkyLight)]
    if existing:
        raise RuntimeError('Another skylight exists; inspect it before adding a fill')
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    if not sky:
        raise RuntimeError('Could not create ambient skylight')
    sky.set_actor_label(LABEL)
component = sky.get_component_by_class(unreal.SkyLightComponent)
component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
component.set_editor_property('source_type', unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
component.set_editor_property('cubemap', cube)
component.set_editor_property('intensity', 0.35)
component.set_light_color(unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
component.set_editor_property('cast_shadows', False)
component.set_editor_property('lower_hemisphere_is_black', False)
if not unreal.EditorLoadingAndSavingUtils.save_current_level():
    raise RuntimeError('Could not save ' + MAP)
unreal.log('ARENA_AMBIENT_FILL_SAVED intensity=0.35')
