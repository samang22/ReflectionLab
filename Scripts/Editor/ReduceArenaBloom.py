"""Override only bloom intensity in the gameplay map; safe to rerun."""
import unreal

MAP = '/Game/ReflectionLab/Maps/Main'
LABEL = 'PP_BloomTuning'
INTENSITY = 0.3375

if not unreal.EditorLoadingAndSavingUtils.load_map(MAP):
    raise RuntimeError('Could not load ' + MAP)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
volume = next((actor for actor in actors.get_all_level_actors()
               if isinstance(actor, unreal.PostProcessVolume) and actor.get_actor_label() == LABEL), None)
if not volume:
    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    if not volume:
        raise RuntimeError('Could not create bloom tuning volume')
    volume.set_actor_label(LABEL)
settings = volume.get_editor_property('settings')
settings.set_editor_property('override_bloom_intensity', True)
settings.set_editor_property('bloom_intensity', INTENSITY)
volume.set_editor_property('settings', settings)
volume.set_editor_property('unbound', True)
volume.set_editor_property('blend_weight', 1.0)
if not unreal.EditorLoadingAndSavingUtils.save_current_level():
    raise RuntimeError('Could not save ' + MAP)
unreal.log('BLOOM_REDUCED_TO {}'.format(INTENSITY))
