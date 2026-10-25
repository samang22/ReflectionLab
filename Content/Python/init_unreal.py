"""Project editor automation bootstrap; no asset mutations at startup."""
import unreal

try:
    from rl_asset_toolset import register
    register()
except Exception as error:
    unreal.log_error('RL asset automation registration failed: ' + str(error))
