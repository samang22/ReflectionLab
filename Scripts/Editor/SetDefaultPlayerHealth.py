import unreal

asset = unreal.load_asset('/Game/ReflectionLab/Data/Player/DA_PlayerStats')
if not asset:
    raise RuntimeError('Player stats asset not found')
asset.set_editor_property('max_health', 10.0)
if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
    raise RuntimeError('Could not save player stats')
assert asset.get_editor_property('max_health') == 10.0
unreal.log('Verified default player maximum health: 10')
