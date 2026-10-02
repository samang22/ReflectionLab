"""One-time BP defaults migration, AFTER building the new C++ module.
Run via Tools > Execute Python Script. Does not modify animation/data assets.
Pass -VerifyPlayerComponents in a fresh commandlet to verify without saving.
"""
import json
import unreal

BP_PATH = "/Game/ReflectionLab/Gameplay/Player/BP_RLPlayerCharacter"
BASELINE = json.loads(r'''{
  "HitFlashInterval": 0.07999999821186066,
  "HitFlashMaterial": {
    "asset": "/Game/ReflectionLab/Art/Materials/Characters/Instances/MI_HitFlash.MI_HitFlash"
  },
  "HitSound": {
    "asset": "/Game/ReflectionLab/Audio/SFX/Combat/PlayerHit/SFX_PlayerHit.SFX_PlayerHit"
  },
  "HitSoundPitchMax": 1.0299999713897705,
  "HitSoundPitchMin": 0.9700000286102295,
  "HitSoundVolume": 0.800000011920929,
  "MirroredParryMontage": {
    "asset": "/Game/ReflectionLab/Gameplay/Player/Animations/AM_Parry_Mirrored.AM_Parry_Mirrored"
  },
  "ParryAvailableIndicatorColor": {
    "color": [
      0.05000000074505806,
      1,
      0.15000000596046448,
      1
    ]
  },
  "ParryCooldownIndicatorColor": {
    "color": [
      1,
      0.05000000074505806,
      0.029999999329447746,
      1
    ]
  },
  "ParryImpactSound": {
    "asset": "/Game/ReflectionLab/Audio/SFX/Combat/Parry/SC_ParryImpact.SC_ParryImpact"
  },
  "ParryMontage": {
    "asset": "/Game/ReflectionLab/Gameplay/Player/Animations/AM_Parry.AM_Parry"
  },
  "ParryRangeIndicatorMaterial": {
    "asset": "/Game/ReflectionLab/Art/Materials/M_ParryRangeIndicator.M_ParryRangeIndicator"
  },
  "ParrySuccessIndicatorColor": {
    "color": [
      0.019999999552965164,
      0.44999998807907104,
      1,
      1
    ]
  },
  "ParrySuccessIndicatorDuration": 0.18000000715255737,
  "PerfectParryAvailableIndicatorColor": {
    "color": [
      0.05000000074505806,
      1,
      0.44999998807907104,
      1
    ]
  },
  "PerfectParryCooldownIndicatorColor": {
    "color": [
      1,
      0.25,
      0.029999999329447746,
      1
    ]
  },
  "PerfectParrySuccessIndicatorColor": {
    "color": [
      0.019999999552965164,
      0.75,
      1,
      1
    ]
  },
  "bShowParryRangeIndicator": true
}''')
HIT_FIELDS = {
    "HitFlashMaterial": "FlashMaterial", "HitSound": "HitSound",
    "HitSoundVolume": "SoundVolume", "HitSoundPitchMin": "SoundPitchMin",
    "HitSoundPitchMax": "SoundPitchMax", "HitFlashInterval": "FlashInterval",
}
PARRY_FIELDS = ("ParryMontage", "MirroredParryMontage")
FEEDBACK_FIELDS = tuple(name for name in BASELINE if name not in HIT_FIELDS and name not in PARRY_FIELDS)


def decode(value):
    if isinstance(value, dict) and "asset" in value:
        asset = unreal.load_asset(value["asset"])
        if not asset:
            raise RuntimeError("Missing asset: " + value["asset"])
        return asset
    if isinstance(value, dict) and "color" in value:
        return unreal.LinearColor(*value["color"])
    return value


def encode(value):
    if isinstance(value, unreal.Object):
        return {"asset": value.get_path_name()}
    if isinstance(value, unreal.LinearColor):
        return {"color": [value.r, value.g, value.b, value.a]}
    return value


def components(blueprint):
    defaults = unreal.get_default_object(blueprint.generated_class())
    result = (
        defaults.get_component_by_class(unreal.RLHitRecoveryComponent),
        defaults.get_component_by_class(unreal.RLParryComponent),
        defaults.get_component_by_class(unreal.RLParryFeedbackComponent),
    )
    if not all(result):
        raise RuntimeError("Build the new module and restart the editor before migration")
    return result


blueprint = unreal.load_asset(BP_PATH)
if not blueprint:
    raise RuntimeError("Player blueprint missing")
hit, parry, feedback = components(blueprint)
verify_only = "-VerifyPlayerComponents" in unreal.SystemLibrary.get_command_line()
if not verify_only:
    # Decode and validate every asset BEFORE changing defaults.
    decoded = {name: decode(value) for name, value in BASELINE.items()}
    settings = hit.get_editor_property("Settings")
    for old_name, new_name in HIT_FIELDS.items():
        settings.set_editor_property(new_name, decoded[old_name])
    hit.set_editor_property("Settings", settings)
    for name in PARRY_FIELDS:
        parry.set_editor_property(name, decoded[name])
    for name in FEEDBACK_FIELDS:
        feedback.set_editor_property(name, decoded[name])
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if "error" in str(blueprint.get_editor_property("Status")).lower():
        raise RuntimeError("Player blueprint compilation failed; blueprint was NOT saved")
    hit, parry, feedback = components(blueprint)

actual = {}
settings = hit.get_editor_property("Settings")
for old_name, new_name in HIT_FIELDS.items():
    actual[old_name] = encode(settings.get_editor_property(new_name))
for name in PARRY_FIELDS:
    actual[name] = encode(parry.get_editor_property(name))
for name in FEEDBACK_FIELDS:
    actual[name] = encode(feedback.get_editor_property(name))
if actual != BASELINE:
    raise RuntimeError("Component defaults differ from the preserved baseline; blueprint was NOT saved")
if not verify_only:
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        raise RuntimeError("Player blueprint save failed")
unreal.log("PLAYER_COMPONENT_DEFAULTS_VERIFIED " + json.dumps(actual, sort_keys=True))

