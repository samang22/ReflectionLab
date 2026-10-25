"""Narrow MCP surface: no arbitrary script or console execution."""
import json
import unreal
import toolset_registry
from rl_asset_changes import editor_service


@unreal.uclass()
class RLAssetToolset(unreal.ToolsetDefinition):
    """Review, apply, verify and undo existing project DataAsset property edits."""

    @toolset_registry.tool_call
    @staticmethod
    def preview(request_json: str) -> str:
        """Read-only asset preview. Saves a reviewed change plan, but never saves assets.

        Args:
            request_json: JSON with changes:[{asset:/Game/... package,property:snake_case,value:JSON,expected:optional current JSON}].
        """
        return json.dumps(editor_service().preview(json.loads(request_json)),ensure_ascii=False)

    @toolset_registry.tool_call
    @staticmethod
    def apply(plan_id: str) -> str:
        """Apply exactly a reviewed preview; save and reload assets, then verify. Reject dirty/stale targets.

        Args:
            plan_id: ID returned by preview. Call only for user-authorized changes.
        """
        return json.dumps(editor_service().apply(plan_id),ensure_ascii=False)

    @toolset_registry.tool_call
    @staticmethod
    def verify(plan_id: str) -> str:
        """Read current properties and compare with a plan's expected final values.

        Args:
            plan_id: ID returned by preview.
        """
        return json.dumps(editor_service().verify(plan_id),ensure_ascii=False)

    @toolset_registry.tool_call
    @staticmethod
    def rollback(plan_id: str) -> str:
        """Restore a completed plan's previous property values; refuse conflicting subsequent edits.

        Args:
            plan_id: ID of an applied plan. Call only when undo is authorized.
        """
        return json.dumps(editor_service().rollback(plan_id),ensure_ascii=False)


def register():
    if not unreal.ToolsetRegistry.is_toolset_class_registered(RLAssetToolset):
        unreal.ToolsetRegistry.register_toolset_class(RLAssetToolset)
    unreal.log('RL_ASSET_TOOLS_REGISTERED: preview/apply/verify/rollback')
