"""Integration test uses only a unique temporary DataAsset, never gameplay assets."""
import json
import uuid
import unreal
from rl_asset_changes import editor_service, AssetChangeError

name = 'AssetTool_' + uuid.uuid4().hex
root = '/Game/ReflectionLab/AutomationTests'
path = root + '/' + name
factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class',unreal.RLArenaExperimentDataAsset)
asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,root,unreal.RLArenaExperimentDataAsset,factory)
if not asset:
    raise RuntimeError('Cannot create temporary test asset')
try:
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset,False):
        raise RuntimeError('Cannot save temporary test asset')
    service = editor_service()
    plan = service.preview({'changes':[
        {'asset':path,'property':'lane_barrier_visual_height','value':55.0,'expected':45.0},
        {'asset':path,'property':'support_grants_invulnerability','value':True},
        {'asset':path,'property':'diagonal_mirror_center','value':{'$type':'Vector2D','x':900.0,'y':750.0}},
        {'asset':path,'property':'field_centers','value':[{'$type':'Vector2D','x':100.0,'y':200.0}]},
        {'asset':path,'property':'type','value':{'$type':'Enum','class':'RLArenaExperimentType','value':'MIRROR_ARENA'}},
    ]})
    assert service.verify(plan['id'])['matches']
    assert service.apply(plan['id'])['matches']
    assert service.apply(plan['id'])['matches']
    assert service.rollback(plan['id'])['matches']
    unreal.log('RL_ASSET_CHANGES_INTEGRATION_PASS ' + json.dumps({'plan_id':plan['id'],'asset':path}))
finally:
    asset = None
    if not unreal.EditorAssetLibrary.delete_asset(path):
        raise RuntimeError('Temporary asset cleanup failed: ' + path)
