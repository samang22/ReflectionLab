"""Fix only the three existing lab text actors, preserving other map edits."""
import unreal

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for path in ['/Game/ReflectionLab/Maps/Main', '/Game/ReflectionLab/Maps/MainMenu']:
    if not unreal.EditorLoadingAndSavingUtils.load_map(path):
        raise RuntimeError('Could not load ' + path)
    indexed = {actor.get_actor_label(): actor for actor in actors.get_all_level_actors()}
    for label in ['Lab_ExperimentFloor', 'Lab_BoundaryFloor', 'Lab_ObservationLabel']:
        actor = indexed.get(label)
        if not isinstance(actor, unreal.TextRenderActor):
            raise RuntimeError('Missing text actor ' + label)
        actor.modify()
        component = actor.get_component_by_class(unreal.TextRenderComponent)
        component.modify()
        if label == 'Lab_ObservationLabel':
            actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=-90, roll=0), False)
            actor.set_actor_location(unreal.Vector(x=0, y=1923, z=470), False, False)
            component.set_world_size(32)
        else:
            actor.set_actor_rotation(unreal.Rotator(pitch=90, yaw=-135, roll=0), False)
        forward = actor.get_actor_forward_vector()
        assert forward.y < -0.99 if label == 'Lab_ObservationLabel' else forward.z > 0.99
        unreal.log('LAB_TEXT_FIXED {} {} {}'.format(path, label, actor.get_actor_rotation()))
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError('Could not save ' + path)
