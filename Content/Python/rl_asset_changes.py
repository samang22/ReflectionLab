"""Reviewed property edits with optimistic concurrency and recovery journals.

The service is engine-independent; UnrealBackend is the editor-only adapter.
No asset creation/deletion, Python evaluation, or Blueprint CDO editing is exposed.
"""
import hashlib
import json
import math
import re
import uuid
from pathlib import Path


class AssetChangeError(RuntimeError):
    pass


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, allow_nan=False).encode()).hexdigest()


def equivalent(left, right):
    if isinstance(left, (int, float)) and not isinstance(left, bool):
        return isinstance(right, (int, float)) and not isinstance(right, bool) and math.isclose(left, right, rel_tol=1e-6, abs_tol=1e-6)
    if type(left) is not type(right):
        return False
    if isinstance(left, dict):
        return left.keys() == right.keys() and all(equivalent(left[k], right[k]) for k in left)
    if isinstance(left, list):
        return len(left) == len(right) and all(equivalent(a, b) for a, b in zip(left, right))
    return left == right


class AssetChangeService:
    def __init__(self, backend, directory, allowed_roots):
        self.backend = backend
        self.directory = Path(directory)
        self.allowed_roots = tuple(allowed_roots)

    def _write(self, plan):
        self.directory.mkdir(parents=True, exist_ok=True)
        target = self.directory / (plan['id'] + '.json')
        temporary = target.with_suffix('.pending')
        temporary.write_text(json.dumps(plan, ensure_ascii=False, indent=2, allow_nan=False), encoding='utf-8')
        temporary.replace(target)

    def _load(self, plan_id):
        if not re.fullmatch(r'[0-9a-f]{32}', plan_id):
            raise AssetChangeError('Invalid plan ID')
        plan = json.loads((self.directory / (plan_id + '.json')).read_text(encoding='utf-8'))
        if plan['project'] != self.backend.project or plan['digest'] != digest(plan['changes']):
            raise AssetChangeError('Project mismatch or modified plan: create a fresh preview')
        for item in plan['changes']:
            self._validate_target(item)
        return plan

    def _validate_target(self, item):
        asset, prop = item['asset'], item['property']
        if not re.fullmatch(r'/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+', asset):
            raise AssetChangeError('Use a canonical /Game/... package path, without object suffix')
        if not any(asset.startswith(root) for root in self.allowed_roots):
            raise AssetChangeError('Asset is outside configured allowed roots: ' + asset)
        if not re.fullmatch(r'[a-z][a-z0-9_]*', prop):
            raise AssetChangeError('Only direct snake_case editor properties are supported')

    def preview(self, request):
        self.backend.ensure_editor_idle()
        changes = request.get('changes', [])
        if not isinstance(changes, list) or not 1 <= len(changes) <= 64:
            raise AssetChangeError('Expected between 1 and 64 changes')
        result, seen = [], set()
        for item in changes:
            self._validate_target(item)
            key = (item['asset'], item['property'])
            if key in seen:
                raise AssetChangeError('Duplicate target: ' + str(key))
            seen.add(key)
            self.backend.ensure_clean(item['asset'])
            before = self.backend.read(*key)
            after = self.backend.validate_value(item['value'], before)
            if 'expected' in item and not equivalent(before, item['expected']):
                raise AssetChangeError('Expected value does not match: ' + str(key))
            result.append(dict(asset=key[0], property=key[1], before=before, after=after,
                               changed=not equivalent(before, after), fingerprint=self.backend.fingerprint(key[0])))
        plan = dict(schema=1, id=uuid.uuid4().hex, project=self.backend.project,
                    status='preview', changes=result, digest=digest(result))
        self._write(plan)
        return plan

    def verify(self, plan_id):
        plan = self._load(plan_id)
        target = 'before' if plan['status'] in ('preview', 'rolled_back', 'recovered') else 'after'
        results = [dict(asset=c['asset'], property=c['property'],
                        matches=equivalent(self.backend.read(c['asset'], c['property']), c[target]))
                   for c in plan['changes']]
        return dict(id=plan_id, status=plan['status'], expected=target, matches=all(r['matches'] for r in results), results=results)

    def apply(self, plan_id):
        return self._execute(plan_id, rollback=False)

    def rollback(self, plan_id):
        return self._execute(plan_id, rollback=True)

    def _execute(self, plan_id, rollback):
        self.backend.ensure_editor_idle()
        plan = self._load(plan_id)
        expected_status = 'applied' if rollback else 'preview'
        final_status = 'rolled_back' if rollback else 'applied'
        if plan['status'] == final_status:
            report = self.verify(plan_id)
            if not report['matches']:
                raise AssetChangeError('Previously completed plan has conflicting current values')
            return report
        if plan['status'] != expected_status:
            raise AssetChangeError('Plan state does not permit this action: ' + plan['status'])
        source, target = ('after', 'before') if rollback else ('before', 'after')
        edits = [c for c in plan['changes'] if c['changed']]
        assets = sorted({c['asset'] for c in edits})
        # Validate the entire batch before any mutation.
        for c in plan['changes']:
            self.backend.ensure_clean(c['asset'])
            if not equivalent(self.backend.read(c['asset'], c['property']), c[source]):
                raise AssetChangeError('Stale preview/current-value conflict: ' + c['asset'] + ':' + c['property'])
            if not rollback and self.backend.fingerprint(c['asset']) != c['fingerprint']:
                raise AssetChangeError('Asset file changed after preview: ' + c['asset'])
        plan['status'] = 'rolling_back' if rollback else 'applying'
        self._write(plan)  # Recovery values are on disk before any property changes.
        attempted = []
        try:
            for c in edits:
                attempted.append(c)
                self.backend.write(c['asset'], c['property'], c[target])
                if not equivalent(self.backend.read(c['asset'], c['property']), c[target]):
                    raise AssetChangeError('Setter did not preserve requested value: ' + c['property'])
            for asset in assets:
                self.backend.save(asset)
            self.backend.reload(assets)
            for c in edits:
                if not equivalent(self.backend.read(c['asset'], c['property']), c[target]):
                    raise AssetChangeError('Reload verification failed: ' + c['property'])
        except Exception as error:
            recovery_errors = []
            for c in reversed(attempted):
                try:
                    self.backend.write(c['asset'], c['property'], c[source])
                except Exception as recovery:
                    recovery_errors.append(str(recovery))
            for asset in sorted({c['asset'] for c in attempted}):
                try:
                    self.backend.save(asset)
                except Exception as recovery:
                    recovery_errors.append(str(recovery))
            try:
                self.backend.reload(sorted({c['asset'] for c in attempted}))
                if any(not equivalent(self.backend.read(c['asset'], c['property']), c[source]) for c in attempted):
                    recovery_errors.append('Recovery verification mismatch')
            except Exception as recovery:
                recovery_errors.append(str(recovery))
            plan.update(status='recovery_failed' if recovery_errors else ('applied' if rollback else 'recovered'),
                        error=str(error), recovery_errors=recovery_errors)
            self._write(plan)
            raise AssetChangeError('Operation failed; journal status=' + plan['status'] + ': ' + str(error)) from error
        plan['status'] = final_status
        self._write(plan)
        return self.verify(plan_id)


class UnrealBackend:
    def __init__(self):
        import unreal
        self.u = unreal
        self.project = str(Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve())

    def ensure_editor_idle(self):
        subsystem = self.u.get_editor_subsystem(self.u.UnrealEditorSubsystem)
        if subsystem.get_game_world():
            raise AssetChangeError('Stop PIE/Simulate before editing assets')

    def asset(self, path):
        asset = self.u.load_asset(path)
        if not asset or not isinstance(asset, self.u.DataAsset):
            raise AssetChangeError('Expected an existing DataAsset: ' + path)
        return asset

    def ensure_clean(self, path):
        self.asset(path)
        dirty = {p.get_path_name() for p in self.u.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
        if path in dirty:
            raise AssetChangeError('Save or discard existing unsaved changes first: ' + path)

    def fingerprint(self, path):
        filename = Path(self.project) / 'Content' / (path.removeprefix('/Game/') + '.uasset')
        return hashlib.sha256(filename.read_bytes()).hexdigest()

    def encode(self, value):
        u = self.u
        if value is None or type(value) in (bool, int, float, str):
            if type(value) is float and not math.isfinite(value):
                raise AssetChangeError('Non-finite number')
            return value
        if isinstance(value, u.Name):
            return {'$type': 'Name', 'value': str(value)}
        if isinstance(value, u.Text):
            raise AssetChangeError('Localized Text edits are not supported; preserve localization metadata')
        if isinstance(value, u.EnumBase):
            return {'$type': 'Enum', 'class': type(value).__name__, 'value': value.name}
        if isinstance(value, u.Object):
            return {'$type': 'Object', 'path': value.get_path_name()}
        for typename, fields in [('Vector2D', ('x','y')), ('Vector', ('x','y','z')),
                                 ('LinearColor', ('r','g','b','a')), ('Rotator', ('pitch','yaw','roll'))]:
            if isinstance(value, getattr(u, typename)):
                return {'$type': typename, **{f: float(getattr(value,f)) for f in fields}}
        if isinstance(value, (list, u.Array)):
            return [self.encode(v) for v in value]
        raise AssetChangeError('Unsupported property type: ' + type(value).__name__)

    def decode(self, value):
        if isinstance(value, list):
            return [self.decode(v) for v in value]
        if not isinstance(value, dict):
            return value
        tag = value.get('$type')
        if tag == 'Name':
            return self.u.Name(value['value'])
        if tag == 'Object':
            path = value['path']
            if not isinstance(path,str) or not path.startswith(('/Game/', '/Engine/')) or '..' in path:
                raise AssetChangeError('Invalid object reference path')
            asset = self.u.load_asset(path)
            if not asset:
                raise AssetChangeError('Missing object reference: ' + path)
            return asset
        if tag == 'Enum':
            enum = getattr(self.u,value['class'],None)
            if not isinstance(enum,type) or not issubclass(enum,self.u.EnumBase):
                raise AssetChangeError('Invalid enum class')
            return getattr(enum,value['value'])
        fields = {'Vector2D': ('x','y'), 'Vector': ('x','y','z'),
                  'LinearColor': ('r','g','b','a'), 'Rotator': ('pitch','yaw','roll')}.get(tag)
        if not fields or set(value) != {'$type', *fields}:
            raise AssetChangeError('Unsupported value representation')
        return getattr(self.u,tag)(**{f: value[f] for f in fields})

    def validate_value(self, value, before):
        # JSON round-trip rejects NaN/infinity and arbitrary Python values.
        json.dumps(value,allow_nan=False)
        if isinstance(before,bool) and type(value) is not bool:
            raise AssetChangeError('Expected bool')
        if type(before) is int and type(value) is not int:
            raise AssetChangeError('Expected integer')
        if type(before) is float and (type(value) not in (int,float)):
            raise AssetChangeError('Expected number')
        if isinstance(before,str) and not isinstance(value,str):
            raise AssetChangeError('Expected string')
        if isinstance(before,list) and not isinstance(value,list):
            raise AssetChangeError('Expected array')
        if isinstance(before,dict) and (not isinstance(value,dict) or value.get('$type') != before.get('$type')):
            if not (before.get('$type') == 'Object' and value is None):
                raise AssetChangeError('Expected matching Unreal value type')
        return self.encode(self.decode(value))

    def read(self, path, prop):
        return self.encode(self.asset(path).get_editor_property(prop))

    def write(self, path, prop, value):
        self.asset(path).set_editor_property(prop,self.decode(value))

    def save(self, path):
        if not self.u.EditorAssetLibrary.save_loaded_asset(self.asset(path),False):
            raise AssetChangeError('Save failed: ' + path)

    def reload(self, paths):
        if not paths:
            return
        result = self.u.EditorLoadingAndSavingUtils.reload_packages(
            [self.asset(p).get_outermost() for p in paths], self.u.ReloadPackagesInteractionMode.ASSUME_NEGATIVE)
        if not result[0] or str(result[1]):
            raise AssetChangeError('Package reload failed: ' + str(result))


def editor_service():
    backend = UnrealBackend()
    config = json.loads((Path(backend.project) / 'Config/AssetAutomation.json').read_text(encoding='utf-8'))
    return AssetChangeService(backend, Path(backend.project) / 'Saved/AssetChanges', config['allowed_roots'])
