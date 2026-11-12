"""Validate default/profile interactions without importing SCons or an SDK."""
import ast
from pathlib import Path
from types import SimpleNamespace
import unittest

ROOT = Path(__file__).resolve().parents[1]
NAMES = {'direct_memory', 'hot_direct', 'hot_memory', 'hot_memory_default',
         'memory_probe', 'opcode_profile', 'core_profile', 'quiet_dac',
         'cpu_unity', 'hot_default', 'hot_routines', 'hot_metadata', 'flat_pages', 'flat_default'}
nodes = []
for node in ast.parse((ROOT / 'bauerino/mame_build.py').read_text()).body:
    if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
        function = node.value.func
        if isinstance(function, ast.Attribute) and isinstance(function.value, ast.Name) and function.value.id == 'subprocess':
            break
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id in NAMES for t in node.targets):
        nodes.append(node)
    elif isinstance(node, ast.Assert):
        nodes.append(node)
CONFIG = compile(ast.Module(body=nodes, type_ignores=[]), 'actual-build-profile-configuration', 'exec')


def config(flags=None, unity='yes'):
    context = {'os': SimpleNamespace(environ=flags or {}),
               'env': SimpleNamespace(GetProjectOption=lambda name, default: unity)}
    exec(CONFIG, context)
    return context


class HotMemoryConfigTests(unittest.TestCase):
    def test_flat_pages_default_explicit_off_and_measured_alone(self):
        self.assertTrue(config()['flat_pages'])
        self.assertFalse(config({'BAUERINO_MAME_FLAT_PAGES':'0'})['flat_pages'])
        self.assertTrue(config({'BAUERINO_MAME_FLAT_PAGES': '1'})['flat_pages'])
        for other in ('HOT_METADATA', 'DIRECT_MEMORY', 'HOT_DIRECT', 'MEMORY_PROBE', 'OPCODE_PROFILE'):
            with self.subTest(other=other), self.assertRaises(AssertionError):
                config({'BAUERINO_MAME_FLAT_PAGES': '1', 'BAUERINO_MAME_' + other: '1'})
        with self.assertRaises(AssertionError):
            config({'BAUERINO_MAME_FLAT_PAGES':'1'},unity='no')

    def test_metadata_is_opt_in_and_excludes_other_experiments(self):
        self.assertFalse(config()['hot_metadata'])
        self.assertTrue(config({'BAUERINO_MAME_HOT_METADATA': '1'})['hot_metadata'])
        self.assertFalse(config({'BAUERINO_MAME_HOT_METADATA': '1'})['flat_pages'])
        for other in ('DIRECT_MEMORY', 'HOT_DIRECT', 'MEMORY_PROBE', 'OPCODE_PROFILE', 'CORE_PROFILE'):
            with self.subTest(other=other), self.assertRaises(AssertionError):
                config({'BAUERINO_MAME_HOT_METADATA': '1', 'BAUERINO_MAME_' + other: '1'})

    def test_default_and_explicit_memory_off(self):
        self.assertTrue(config()['hot_memory'])
        disabled = config({'BAUERINO_MAME_HOT_MEMORY': '0'})
        self.assertTrue(disabled['hot_routines'])
        self.assertFalse(disabled['hot_memory'])
        self.assertFalse(disabled['flat_pages'])

    def test_all_iram_off_and_non_unity(self):
        for result in (config({'BAUERINO_MAME_HOT_ROUTINES': '0'}), config(unity='no')):
            self.assertFalse(result['hot_routines'])
            self.assertFalse(result['hot_memory'])
            self.assertFalse(result['flat_pages'])

    def test_legacy_probe_and_direct_profiles_remain_independent(self):
        for flag in ('MEMORY_PROBE', 'OPCODE_PROFILE', 'DIRECT_MEMORY'):
            with self.subTest(flag=flag):
                result = config({'BAUERINO_MAME_' + flag: '1'})
                self.assertFalse(result['hot_routines'])
                self.assertFalse(result['hot_memory'])
                self.assertFalse(result['flat_pages'])

    def test_inline_experiment_keeps_its_original_iram_selection(self):
        result = config({'BAUERINO_MAME_HOT_DIRECT': '1'})
        self.assertTrue(result['hot_routines'])
        self.assertFalse(result['hot_memory'])
        self.assertFalse(result['flat_pages'])

    def test_core_diagnostic_inherits_current_default(self):
        self.assertTrue(config({'BAUERINO_MAME_CORE_PROFILE': '1'})['hot_memory'])
        self.assertTrue(config({'BAUERINO_MAME_CORE_PROFILE': '1'})['flat_pages'])

    def test_explicit_memory_requires_iram(self):
        with self.assertRaises(AssertionError):
            config({'BAUERINO_MAME_HOT_MEMORY': '1', 'BAUERINO_MAME_HOT_ROUTINES': '0'})
        with self.assertRaises(AssertionError):
            config({'BAUERINO_MAME_HOT_MEMORY': '1'}, unity='no')

    def test_conflicting_core_diagnostic_rejected(self):
        with self.assertRaises(AssertionError):
            config({'BAUERINO_MAME_CORE_PROFILE': '1', 'BAUERINO_MAME_OPCODE_PROFILE': '1'})


if __name__ == '__main__':
    unittest.main()
