"""Shared editor helpers for the consolidated robot-boss settings asset."""
import unreal

PATH = '/Game/ReflectionLab/Data/Enemies/DA_RobotBoss'
SECTIONS = {
    'DA_RobotBossOverload': 'overload', 'DA_RobotBossCharge': 'charge',
    'DA_RobotBossLaser': 'laser', 'DA_RobotBossSummon': 'summon',
    'DA_RobotBossPaperBurn': 'paper_burn',
}


def load_asset(path):
    section = SECTIONS.get(str(path).rsplit('/', 1)[-1])
    if section:
        boss = unreal.load_asset(PATH)
        if not boss:
            raise RuntimeError('Run ConsolidateRobotBossSettings.py first')
        return boss.get_editor_property(section)
    return unreal.load_asset(path)


def save_loaded_asset(asset, only_if_is_dirty=True):
    outer = asset.get_outer()
    if isinstance(outer, unreal.RLRobotBossDataAsset):
        asset = outer
    return unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty)
