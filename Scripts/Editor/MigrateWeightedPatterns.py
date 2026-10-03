"""After a full rebuild/restart, run Tools > Execute Python Script once.

Convert the five campaign schedules to weighted selection. Tutorial and all
combat/spawn tuning stay untouched. Reruns preserve already migrated weights.
Reward Count -> Weight is handled by the project's CoreRedirects on load.
"""
import unreal

ROOT = "/Game/ReflectionLab/Data/Difficulty"
schedules = []
for number in range(1, 6):
    path = f"{ROOT}/DA_Round{number:02d}_Difficulty"
    asset = unreal.load_asset(path)
    if not isinstance(asset, unreal.RLDifficultyScheduleDataAsset):
        raise RuntimeError(f"Campaign schedule is missing: {path}")
    waves = list(asset.get_editor_property("waves"))
    # Preflight reflection availability before any writes.
    for wave in waves:
        wave.get_editor_property("use_weighted_patterns")
        wave.get_editor_property("default_projectile_weight")
        wave.get_editor_property("ring_attack").get_editor_property("minimum_interval_seconds")
    schedules.append((asset, waves))

for asset, waves in schedules:
    changed = False
    for wave in waves:
        if wave.get_editor_property("use_weighted_patterns"):
            continue
        wave.set_editor_property("default_projectile_weight", 12)
        rules = list(wave.get_editor_property("projectile_rules"))
        for rule in rules:
            enabled = (rule.get_editor_property("projectile_definition") is not None
                       and rule.get_editor_property("shot_interval") > 0)
            rule.set_editor_property("weight", 3 if enabled else 0)
        wave.set_editor_property("projectile_rules", rules)
        ring = wave.get_editor_property("ring_attack")
        ring.set_editor_property("weight", 1 if ring.get_editor_property("shot_interval") > 0 else 0)
        ring.set_editor_property("minimum_interval_seconds", 20.0)
        wave.set_editor_property("ring_attack", ring)
        wave.set_editor_property("use_weighted_patterns", True)
        changed = True
    if changed:
        asset.set_editor_property("waves", waves)
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
            raise RuntimeError(f"Could not save {asset.get_path_name()}")
    if not all(wave.get_editor_property("use_weighted_patterns")
               for wave in asset.get_editor_property("waves")):
        raise RuntimeError(f"Migration verification failed: {asset.get_path_name()}")
    unreal.log(f"WEIGHTED_PATTERNS_READY: {asset.get_path_name()}")

# Persist redirected reward values without resetting custom weights.
for path in unreal.EditorAssetLibrary.list_assets("/Game/ReflectionLab/Data/Rewards", True, False):
    reward = unreal.load_asset(path)
    if isinstance(reward, unreal.RLRunRewardDataAsset):
        weight = reward.get_editor_property("weight")
        if not unreal.EditorAssetLibrary.save_loaded_asset(reward, False):
            raise RuntimeError(f"Could not save {path}")
        if reward.get_editor_property("weight") != weight:
            raise RuntimeError(f"Reward weight changed while saving: {path}")
unreal.log("WEIGHTED_PATTERN_MIGRATION_COMPLETE")
