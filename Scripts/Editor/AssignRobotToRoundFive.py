import unreal

# Run after rebuilding and restarting. Preserves every other run setting.
run = unreal.load_asset("/Game/ReflectionLab/Data/Difficulty/DA_DefaultRun")
robot = unreal.EditorAssetLibrary.load_blueprint_class(
    "/Game/ReflectionLab/Gameplay/Enemies/BP_RLRobotEnemy")
if not run or not robot:
    raise RuntimeError("Default run or robot enemy Blueprint missing")
classes = dict(run.get_editor_property("round_enemy_classes"))
classes[5] = robot
run.set_editor_property("round_enemy_classes", classes)
if run.get_editor_property("round_enemy_classes")[5] != robot:
    raise RuntimeError("Round-five robot assignment was not retained")
if not unreal.EditorAssetLibrary.save_loaded_asset(run):
    raise RuntimeError("Could not save default run")
unreal.log("[RobotRound] SUCCESS: displayed round 5 uses BP_RLRobotEnemy; other rounds unchanged")
