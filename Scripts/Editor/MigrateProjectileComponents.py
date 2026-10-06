"""One-time Projectile BP defaults migration AFTER rebuilding and restarting UE.

Run via Tools > Execute Python Script. Only BP_RLProjectile is saved.
Use -VerifyProjectileComponents in a commandlet for read-only verification.
The baseline was captured from the existing BP before its settings moved.
"""
import json
import unreal

BP_PATH = "/Game/ReflectionLab/Gameplay/Projectiles/BP_RLProjectile"
BASELINE = json.loads(r'''{
  "DelayedExplosivePauseDuration": 0.550000011920929,
  "DelayedExplosiveResumeSpeedMultiplier": 1.2999999523162842,
  "DelayedExplosiveTriggerDistance": 320,
  "ExplosionDamage": 1,
  "ExplosionRadius": 180,
  "ExplosionSound": {
    "asset": "/Game/ReflectionLab/Audio/SFX/Combat/Explosion/SFX_ExplosiveDetonation.SFX_ExplosiveDetonation"
  },
  "ExplosionSoundVolume": 0.800000011920929,
  "ExplosiveBaseColor": {
    "color": [
      1,
      0,
      0,
      1
    ]
  },
  "ExplosiveBaseEmissiveIntensity": 12,
  "ExplosiveBlinkEndInterval": 0.05999999865889549,
  "ExplosiveBlinkStartInterval": 0.44999998807907104,
  "ExplosiveDecalColor": {
    "color": [
      1,
      0,
      0,
      1
    ]
  },
  "ExplosiveFuseDuration": 5,
  "ExplosiveSpeedMultiplier": 0.5,
  "ExplosiveVisualScale": 2,
  "ExplosiveWarningColor": {
    "color": [
      1,
      0.8500000238418579,
      0.07999999821186066,
      1
    ]
  },
  "ExplosiveWarningEmissiveIntensity": 40,
  "FadeOutDuration": 0.20000000298023224,
  "FakeColor": {
    "color": [
      0.11999999731779099,
      0.014999999664723873,
      0.2199999988079071,
      1
    ]
  },
  "FakeOpacity": 0.3799999952316284,
  "FakeRealSpeedMultiplier": 1.350000023841858,
  "FakeRevealDelay": 0.4000000059604645,
  "FakeTriggerDistance": 280,
  "GuardColor": {
    "color": [
      0.699999988079071,
      0.11999999731779099,
      1,
      1
    ]
  },
  "HostileMaterial": {
    "asset": "/Game/ReflectionLab/Art/Materials/Projectiles/MI_Projectile_Hostile.MI_Projectile_Hostile"
  },
  "ParrySplitColor": {
    "color": [
      1,
      0.3199999928474426,
      0.019999999552965164,
      1
    ]
  },
  "ParrySplitFragmentCount": 2,
  "ParrySplitFragmentScale": 0.75,
  "ParrySplitFragmentSpeedMultiplier": 0.699999988079071,
  "ParrySplitSpreadAngle": 70,
  "RallyArrivalRadius": 48,
  "RallyMaxSpeedMultiplier": 3,
  "RallyVisualScale": 1.25,
  "ReflectedAfterimageCount": 4,
  "ReflectedAfterimageSampleInterval": 0.03500000014901161,
  "ReflectedAfterimageScaleFalloff": 0.11999999731779099,
  "ReflectedMaterial": {
    "asset": "/Game/ReflectionLab/Art/Materials/Projectiles/MI_Projectile_Reflected.MI_Projectile_Reflected"
  }
}''')
GROUPS = {
    "Visual": [
        "FadeOutDuration",
        "HostileMaterial",
        "ReflectedMaterial",
        "ReflectedAfterimageCount",
        "ReflectedAfterimageSampleInterval",
        "ReflectedAfterimageScaleFalloff"
    ],
    "Rally": [
        "RallyVisualScale",
        "RallyMaxSpeedMultiplier",
        "RallyArrivalRadius"
    ],
    "Special": [
        "ExplosiveSpeedMultiplier",
        "ExplosiveVisualScale",
        "ExplosiveFuseDuration",
        "ExplosionRadius",
        "ExplosionDamage",
        "ExplosionSound",
        "ExplosionSoundVolume",
        "ExplosiveBlinkStartInterval",
        "ExplosiveBlinkEndInterval",
        "ExplosiveBaseColor",
        "ExplosiveDecalColor",
        "ExplosiveWarningColor",
        "ExplosiveBaseEmissiveIntensity",
        "ExplosiveWarningEmissiveIntensity",
        "DelayedExplosiveTriggerDistance",
        "DelayedExplosivePauseDuration",
        "DelayedExplosiveResumeSpeedMultiplier",
        "FakeTriggerDistance",
        "FakeRevealDelay",
        "FakeRealSpeedMultiplier",
        "FakeColor",
        "FakeOpacity",
        "GuardColor",
        "ParrySplitColor",
        "ParrySplitFragmentCount",
        "ParrySplitSpreadAngle",
        "ParrySplitFragmentSpeedMultiplier",
        "ParrySplitFragmentScale"
    ]
}


def encode(value):
    if isinstance(value, unreal.Object):
        return {"asset": value.get_path_name()}
    if isinstance(value, unreal.LinearColor):
        return {"color": [value.r, value.g, value.b, value.a]}
    return value


def decode(value):
    if isinstance(value, dict) and "asset" in value:
        asset = unreal.load_asset(value["asset"])
        if not asset:
            raise RuntimeError("Missing asset: " + value["asset"])
        return asset
    if isinstance(value, dict) and "color" in value:
        return unreal.LinearColor(*value["color"])
    return value


def components(blueprint):
    defaults = unreal.get_default_object(blueprint.generated_class())
    result = {}
    for group in GROUPS:
        component_class = getattr(unreal, "RLProjectile" + group + "Component", None)
        component = defaults.get_component_by_class(component_class) if component_class else None
        if not component:
            raise RuntimeError("Rebuild and restart the editor before running migration: " + group)
        result[group] = component
    return result


def read_values(blueprint):
    values = {}
    for group, component in components(blueprint).items():
        settings = component.get_editor_property("Settings")
        for name in GROUPS[group]:
            values[name] = encode(settings.get_editor_property(name))
    return values


blueprint = unreal.load_asset(BP_PATH)
if not blueprint:
    raise RuntimeError("Projectile blueprint missing: " + BP_PATH)
verify_only = "-VerifyProjectileComponents" in unreal.SystemLibrary.get_command_line()
if not verify_only:
    # Resolve all referenced assets and settings BEFORE changing the blueprint.
    decoded = {name: decode(value) for name, value in BASELINE.items()}
    owners = components(blueprint)
    previous = read_values(blueprint)
    try:
        for group, component in owners.items():
            settings = component.get_editor_property("Settings")
            for name in GROUPS[group]:
                settings.set_editor_property(name, decoded[name])
            component.set_editor_property("Settings", settings)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        if "error" in str(blueprint.get_editor_property("Status")).lower():
            raise RuntimeError("Projectile blueprint compilation failed")
        if read_values(blueprint) != BASELINE:
            raise RuntimeError("Migrated component defaults differ from the captured baseline")
    except Exception:
        for group, component in components(blueprint).items():
            settings = component.get_editor_property("Settings")
            for name in GROUPS[group]:
                settings.set_editor_property(name, decode(previous[name]))
            component.set_editor_property("Settings", settings)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        raise
if read_values(blueprint) != BASELINE:
    raise RuntimeError("Component defaults differ from the baseline; blueprint was NOT saved")
if not verify_only and not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
    raise RuntimeError("Projectile blueprint save failed")
unreal.log("PROJECTILE_COMPONENT_DEFAULTS_VERIFIED " + json.dumps(read_values(blueprint), sort_keys=True))
