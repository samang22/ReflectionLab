"""After a full C++ rebuild, run via Tools > Execute Python Script.

Create the eight existing rewards and register them on DA_DefaultRun.
Reruns preserve existing reward edits and previously registered rewards.
Only the reward assets and DA_DefaultRun are saved.
"""
import unreal

ROOT = "/Game/ReflectionLab/Data/Rewards"
RUN_PATH = "/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun"
ART_ROOT = "/Game/ReflectionLab/UI/Rewards/Textures"
ROWS = [
    ("WideSwing", "WIDER_ARC", 12.0, "WIDE SWING", "Parry angle +12 degrees",
     "T_Reward_WideSwing", (0.1, 0.85, 1.0)),
    ("LongReach", "EXTENDED_RANGE", 0.18, "LONG REACH", "Parry range +18%",
     "T_Reward_LongReach", (0.25, 1.0, 0.55)),
    ("PiercingReturn", "PIERCING_RETURN", 1.0, "PIERCING RETURN", "Reflected projectile pierce +1",
     "T_Reward_PiercingReturn", (0.72, 0.35, 1.0)),
    ("PerfectVolley", "PERFECT_FOCUS", 1.0, "PERFECT VOLLEY", "Perfect parry split projectile +1",
     "T_Reward_PerfectVolley", (1.0, 0.72, 0.12)),
    ("VelocityDrive", "VELOCITY_DRIVE", 0.2, "VELOCITY DRIVE", "Maximum reflected speed +0.2x",
     "T_Reward_VelocityDrive", (1.0, 0.38, 0.08)),
    ("CloseCall", "CLOSE_CALL", 12.0, "CLOSE CALL", "Close-range parry zone +12 cm",
     "T_Reward_CloseCall", (1.0, 0.12, 0.08)),
    ("Vitality", "VITALITY", 2.0, "VITALITY", "Max HP +2 and restore 2 HP",
     "max-health-heart-grid", (0.2, 1.0, 0.4)),
    ("PerfectRecovery", "PERFECT_RECOVERY", 1.0, "PERFECT RECOVERY", "Perfect parry healing +1 HP (stacks)",
     "parry-heal-robot-grid", (1.0, 0.4, 0.7)),
]

reward_class = getattr(unreal, "RLRunRewardDataAsset", None)
if reward_class is None:
    raise RuntimeError("Rebuild ReflectionLabEditor and restart the editor before running this script")
run = unreal.load_asset(RUN_PATH)
if not run:
    raise RuntimeError("DA_DefaultRun is missing")

# Validate dependencies and existing asset types before creating or modifying anything.
prepared = []
for name, effect, amount, title, description, art, color in ROWS:
    reward_type = getattr(unreal.RLRunRewardType, effect)
    texture = unreal.load_asset(f"{ART_ROOT}/{art}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Reward illustration is missing: {art}")
    path = f"{ROOT}/DA_Reward_{name}"
    asset = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if asset and not isinstance(asset, reward_class):
        raise RuntimeError(f"Unexpected asset class: {path}")
    prepared.append((path, asset, reward_type, amount, title, description, texture, color))

registered = list(run.get_editor_property("reward_pool"))
for asset in registered:
    if asset and not isinstance(asset, reward_class):
        raise RuntimeError("Reward pool contains an unexpected asset class")
registered_types = {asset.get_editor_property("reward_type") for asset in registered if asset}
tools = unreal.AssetToolsHelpers.get_asset_tools()
generated = []
for path, asset, effect, amount, title, description, texture, color in prepared:
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", reward_class)
        asset = tools.create_asset(path.rsplit("/", 1)[1], ROOT, reward_class, factory)
        if not asset:
            raise RuntimeError(f"Could not create {path}")
        asset.set_editor_property("reward_type", effect)
        asset.set_editor_property("count", 1)
        asset.set_editor_property("amount", amount)
        asset.set_editor_property("title", unreal.Text(title))
        asset.set_editor_property("description", unreal.Text(description))
        asset.set_editor_property("illustration", texture)
        asset.set_editor_property("accent_color", unreal.LinearColor(*color, 1.0))
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
            raise RuntimeError(f"Could not save {path}")
    generated.append(asset)
    actual_effect = asset.get_editor_property("reward_type")
    if actual_effect not in registered_types:
        registered.append(asset)
        registered_types.add(actual_effect)

if len(registered_types) < 3:
    raise RuntimeError("At least three distinct reward effects must be registered")
run.set_editor_property("reward_pool", registered)
if not unreal.EditorAssetLibrary.save_loaded_asset(run, False):
    raise RuntimeError("Could not save DA_DefaultRun")
actual_pool = [asset for asset in run.get_editor_property("reward_pool") if asset]
actual_paths = {asset.get_path_name() for asset in actual_pool}
actual_types = {asset.get_editor_property("reward_type") for asset in actual_pool}
for asset in generated:
    if asset.get_editor_property("reward_type") not in actual_types:
        raise RuntimeError("Reward registration verification failed")
unreal.log(f"RUN_REWARDS_REGISTERED: {len(actual_paths)} assets; existing edits preserved")
