"""Read-only snapshot using the pre-refactor DLL. Does not save assets."""
import json
import unreal

def encode(value):
    if isinstance(value, unreal.Object):
        return {"asset": value.get_path_name()}
    if isinstance(value, unreal.LinearColor):
        return {"color": [value.r, value.g, value.b, value.a]}
    return value

blueprint = unreal.load_asset("/Game/ReflectionLab/Gameplay/Player/BP_RLPlayerCharacter")
if not blueprint:
    raise RuntimeError("Player blueprint missing")
defaults = unreal.get_default_object(blueprint.generated_class())
names = ["ParryMontage","MirroredParryMontage","HitFlashMaterial","HitSound","HitSoundVolume","HitSoundPitchMin","HitSoundPitchMax","HitFlashInterval","ParryRangeIndicatorMaterial","bShowParryRangeIndicator","ParryAvailableIndicatorColor","PerfectParryAvailableIndicatorColor","ParryCooldownIndicatorColor","PerfectParryCooldownIndicatorColor","ParrySuccessIndicatorColor","PerfectParrySuccessIndicatorColor","ParrySuccessIndicatorDuration","ParryImpactSound"]
values = {name: encode(defaults.get_editor_property(name)) for name in names}
unreal.log("PLAYER_COMPONENT_BASELINE " + json.dumps(values, sort_keys=True))

# Inspect member-variable nodes before removing reflected Character fields.
for path in (
    "/Game/ReflectionLab/Gameplay/Player/BP_RLPlayerCharacter",
    "/Game/ReflectionLab/Gameplay/Player/Animations/ABP_RLPlayerCharacter",
    "/Game/ReflectionLab/Gameplay/Player/Animations/Notifies/BP_ANS_ParryWindow",
):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Blueprint missing: " + path)
    for graph in unreal.BlueprintEditorLibrary.list_graphs(asset):
        for node in unreal.ObjectIterator(unreal.K2Node_Variable):
            if node.get_outer() == graph:
                for pin in unreal.BlueprintEditorLibrary.list_all_pins(node):
                    unreal.log("PLAYER_COMPONENT_BP_MEMBER " + path + " " + str(pin.get_pin_name()))

