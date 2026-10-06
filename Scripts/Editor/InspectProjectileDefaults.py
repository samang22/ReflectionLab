"""Read-only snapshot of legacy Projectile BP defaults before header cleanup."""
import json
import unreal

FIELDS = ["FadeOutDuration","HostileMaterial","ReflectedMaterial","ReflectedAfterimageCount","ReflectedAfterimageSampleInterval","ReflectedAfterimageScaleFalloff","ExplosiveSpeedMultiplier","ExplosiveVisualScale","ExplosiveFuseDuration","ExplosionRadius","ExplosionDamage","ExplosionSound","ExplosionSoundVolume","ExplosiveBlinkStartInterval","ExplosiveBlinkEndInterval","ExplosiveBaseColor","ExplosiveDecalColor","ExplosiveWarningColor","ExplosiveBaseEmissiveIntensity","ExplosiveWarningEmissiveIntensity","DelayedExplosiveTriggerDistance","DelayedExplosivePauseDuration","DelayedExplosiveResumeSpeedMultiplier","FakeTriggerDistance","FakeRevealDelay","FakeRealSpeedMultiplier","FakeColor","FakeOpacity","GuardColor","ParrySplitColor","ParrySplitFragmentCount","ParrySplitSpreadAngle","ParrySplitFragmentSpeedMultiplier","ParrySplitFragmentScale","RallyVisualScale","RallyMaxSpeedMultiplier","RallyArrivalRadius"]
PATH = "/Game/ReflectionLab/Gameplay/Projectiles/BP_RLProjectile"

def encode(value):
    if isinstance(value, unreal.Object):
        return {"asset": value.get_path_name()}
    if isinstance(value, unreal.LinearColor):
        return {"color": [value.r, value.g, value.b, value.a]}
    return value

blueprint = unreal.load_asset(PATH)
if not blueprint:
    raise RuntimeError("Projectile blueprint not found: " + PATH)
defaults = unreal.get_default_object(blueprint.generated_class())
baseline = {name: encode(defaults.get_editor_property(name)) for name in FIELDS}
unreal.log("PROJECTILE_LEGACY_DEFAULTS " + json.dumps(baseline, sort_keys=True))
