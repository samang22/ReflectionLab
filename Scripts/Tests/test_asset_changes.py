import copy
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[2] / 'Content/Python'))
from rl_asset_changes import AssetChangeService, AssetChangeError, equivalent


class FakeBackend:
    project = 'test-project'

    def __init__(self):
        self.values = {'/Game/ReflectionLab/A': {'speed': 1.0}, '/Game/ReflectionLab/B': {'speed': 2.0}}
        self.disk = copy.deepcopy(self.values)
        self.dirty = set()
        self.saves = []
        self.fail_save_once = None
        self.playing = False

    def ensure_editor_idle(self):
        if self.playing:
            raise AssetChangeError('PIE running')

    def ensure_clean(self,path):
        if path in self.dirty:
            raise AssetChangeError('Unsaved edits')

    def read(self,path,prop):
        return copy.deepcopy(self.values[path][prop])

    def validate_value(self,value,before):
        return copy.deepcopy(value)

    def fingerprint(self,path):
        return str(self.disk[path])

    def write(self,path,prop,value):
        self.values[path][prop] = value

    def save(self,path):
        self.saves.append(path)
        if self.fail_save_once == path:
            self.fail_save_once = None
            raise AssetChangeError('Injected save failure')
        self.disk[path] = copy.deepcopy(self.values[path])

    def reload(self,paths):
        for path in paths:
            self.values[path] = copy.deepcopy(self.disk[path])


class AssetChangeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.backend = FakeBackend()
        self.service = AssetChangeService(self.backend,self.temp.name,['/Game/ReflectionLab/'])

    def preview(self,value=3.0):
        return self.service.preview({'changes':[{'asset':'/Game/ReflectionLab/A','property':'speed','value':value}]})

    def test_preview_is_read_only(self):
        plan = self.preview()
        self.assertEqual(self.backend.read('/Game/ReflectionLab/A','speed'),1.0)
        self.assertEqual(self.backend.saves,[])
        self.assertTrue(self.service.verify(plan['id'])['matches'])

    def test_apply_verify_and_rollback(self):
        plan = self.preview()
        self.assertTrue(self.service.apply(plan['id'])['matches'])
        self.assertEqual(self.backend.disk['/Game/ReflectionLab/A']['speed'],3.0)
        self.assertTrue(self.service.rollback(plan['id'])['matches'])
        self.assertEqual(self.backend.disk['/Game/ReflectionLab/A']['speed'],1.0)

    def test_repeated_apply_does_not_save_again(self):
        plan = self.preview()
        self.service.apply(plan['id'])
        count = len(self.backend.saves)
        self.service.apply(plan['id'])
        self.assertEqual(len(self.backend.saves),count)

    def test_noop_does_not_save(self):
        self.service.apply(self.preview(1.0)['id'])
        self.assertEqual(self.backend.saves,[])

    def test_dirty_target_rejected(self):
        plan = self.preview()
        self.backend.dirty.add('/Game/ReflectionLab/A')
        with self.assertRaises(AssetChangeError):
            self.service.apply(plan['id'])
        self.assertEqual(self.backend.saves,[])

    def test_stale_memory_rejected(self):
        plan = self.preview()
        self.backend.values['/Game/ReflectionLab/A']['speed'] = 4.0
        with self.assertRaises(AssetChangeError):
            self.service.apply(plan['id'])

    def test_stale_file_rejected(self):
        plan = self.preview()
        self.backend.disk['/Game/ReflectionLab/A']['speed'] = 5.0
        with self.assertRaises(AssetChangeError):
            self.service.apply(plan['id'])

    def test_conflicting_undo_rejected(self):
        plan = self.preview()
        self.service.apply(plan['id'])
        self.backend.values['/Game/ReflectionLab/A']['speed'] = 4.0
        with self.assertRaises(AssetChangeError):
            self.service.rollback(plan['id'])

    def test_batch_save_failure_recovers_previous_values(self):
        plan = self.service.preview({'changes':[
            {'asset':'/Game/ReflectionLab/A','property':'speed','value':3.0},
            {'asset':'/Game/ReflectionLab/B','property':'speed','value':4.0}]})
        self.backend.fail_save_once = '/Game/ReflectionLab/B'
        with self.assertRaises(AssetChangeError):
            self.service.apply(plan['id'])
        self.assertEqual(self.backend.disk['/Game/ReflectionLab/A']['speed'],1.0)
        self.assertEqual(self.backend.disk['/Game/ReflectionLab/B']['speed'],2.0)
        self.assertEqual(self.service.verify(plan['id'])['status'],'recovered')

    def test_scope_and_plan_path_rejected(self):
        with self.assertRaises(AssetChangeError):
            self.service.preview({'changes':[{'asset':'/Game/Other/A','property':'speed','value':1}]})
        with self.assertRaises(AssetChangeError):
            self.service.apply('../outside')

    def test_duplicate_property_rejected(self):
        item = {'asset':'/Game/ReflectionLab/A','property':'speed','value':1}
        with self.assertRaises(AssetChangeError):
            self.service.preview({'changes':[item,item]})

    def test_pie_rejected(self):
        self.backend.playing = True
        with self.assertRaises(AssetChangeError):
            self.preview()

    def test_tolerance_does_not_confuse_bool_and_integer(self):
        self.assertTrue(equivalent(1.0,1.0000001))
        self.assertFalse(equivalent(True,1))


if __name__ == '__main__':
    unittest.main()
