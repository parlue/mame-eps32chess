"""Regressions for exclusive-time accounting and incomplete USB diagnostics."""
import json
from pathlib import Path
import tempfile
import unittest
from report_core_profile import summarize


class CoreProfileTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        (self.folder / 'evidence.json').write_text(json.dumps({
            'measurement_kind': 'instrumented-core-profile', 'driver': 'mm4',
            'case': 'spanish-centre', 'clean_exit': True}))
        self.events = [[0, '[BENCH] search_begin wall_seconds=90']]
        for seq in range(1, 11):
            self.events.append([10 + seq * 5, f'[MAME CORE] seq={seq} wall_us=5000000 exclusive_us=500000,500000,3000000,200000,100000,100000,400000 other_us=200000 calls=100,100,100,10,10,1,10 depth=5'])
        self.events.append([90, '[BENCH] search_end'])

    def result(self, allow_missing=False):
        (self.folder / 'events.json').write_text(json.dumps(self.events))
        return summarize(self.folder, allow_missing)

    def test_partition_and_weighted_time(self):
        result = self.result()
        self.assertEqual(result['scopes'][0]['scope'], 'cpu')
        self.assertEqual(result['scopes'][0]['wall_percent'], 60)
        self.assertEqual(result['measured_wall_seconds'], 50)
        self.assertEqual(result['unscoped_wall_percent'], 4)

    def test_known_live_insertion_inside_number(self):
        timestamp, line = self.events[4]
        before, after = line.split('3000000', 1)
        self.events[4:5] = [[timestamp, before + '300[LIVE] t=123 clients=0 heap=20000 engine=0 error='], [timestamp, '0000' + after]]
        self.assertEqual(self.result()['scopes'][0]['wall_percent'], 60)

    def test_sum_mismatch_fails(self):
        self.events[4][1] = self.events[4][1].replace('other_us=200000', 'other_us=200001')
        with self.assertRaisesRegex(AssertionError, 'partition'):
            self.result()

    def test_lost_interval_requires_explicit_exclusion(self):
        del self.events[4]
        with self.assertRaisesRegex(AssertionError, 'Missing interior'):
            self.result()
        self.assertEqual(self.result(True)['missing_intervals'], [4])

    def test_unknown_corruption_fails(self):
        self.events[4][1] = self.events[4][1].replace('3000000', '300X0000')
        with self.assertRaisesRegex(AssertionError, 'corrupt'):
            self.result()

    def test_diagnostic_label_required(self):
        (self.folder / 'evidence.json').write_text(json.dumps({'measurement_kind': 'sustained-speed'}))
        with self.assertRaises(AssertionError):
            self.result()


if __name__ == '__main__':
    unittest.main()
