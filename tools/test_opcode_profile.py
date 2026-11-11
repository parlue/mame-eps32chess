"""Host regressions for frequency attribution and observed USB record loss."""
import json
import tempfile
import unittest
from pathlib import Path
from report_opcode_profile import routine_map, summarize


class OpcodeProfileTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        self.mapping = routine_map(Path(__file__).resolve().parents[1])
        (self.folder / 'evidence.json').write_text(json.dumps({
            'measurement_kind': 'instrumented-opcode-profile', 'driver': 'mm4', 'case': 'spanish-centre'}))
        self.events = [[0, '[BENCH] search_begin wall_seconds=90.0']]
        for seq in range(1, 11):
            self.events.append([10 + seq, f'[MAME OPCODE TOTAL] seq={seq} full=100 partial=1 special=0'])
            for group in range(8):
                values = [0] * 32
                if group == 5:
                    values[9], values[25] = 80, 20  # LDA immediate / absolute-Y
                self.events.append([10 + seq, f'[MAME OPCODE COUNTS] seq={seq} group={group} counts=' + ','.join(map(str, values))])
        self.events.append([90, '[BENCH] search_end'])

    def result(self, allow_missing=False):
        (self.folder / 'events.json').write_text(json.dumps(self.events))
        return summarize(self.folder, self.mapping, allow_missing)

    def test_known_mix_and_inherited_routines(self):
        result = self.result()
        self.assertEqual(result['full_instruction_starts'], 1000)
        self.assertEqual(result['partial_resume_calls'], 10)
        self.assertEqual(result['routines'][0], {'routine': 'm6502_device::lda_imm_full', 'starts': 800, 'percent': 80.0})
        self.assertEqual(self.mapping[0x69], 'm65c02_device::adc_c_imm_full')

    def test_live_message_inside_decimal_count(self):
        index = next(i for i, (_, line) in enumerate(self.events) if 'seq=3 group=5 counts=' in line)
        timestamp, line = self.events[index]
        left, right = line.split('80', 1)
        self.events[index:index + 1] = [[timestamp, left + '8[LIVE] t=1 clients=0 heap=1 engine=0 error='], [timestamp, '0' + right]]
        self.assertEqual(self.result()['full_instruction_starts'], 1000)

    def test_missing_group_requires_explicit_exclusion(self):
        self.events = [row for row in self.events if 'seq=3 group=4 ' not in row[1]]
        with self.assertRaisesRegex(AssertionError, 'Incomplete interior'):
            self.result()
        result = self.result(True)
        self.assertEqual(result['full_instruction_starts'], 900)
        self.assertIn(3, result['excluded_intervals'])

    def test_entire_missing_interval_is_detected(self):
        self.events = [row for row in self.events if 'seq=3 ' not in row[1]]
        result = self.result(True)
        self.assertEqual(result['accepted_interval_fraction'], .9)
        self.assertIn(3, result['excluded_intervals'])

    def test_wrong_sum_fails_even_in_loss_mode(self):
        self.events = [[t, s.replace('seq=3 full=100 ', 'seq=3 full=101 ')] for t, s in self.events]
        with self.assertRaisesRegex(AssertionError, 'sum mismatch'):
            self.result(True)

    def test_loss_limit_is_enforced(self):
        prototype = [row for row in self.events if 'seq=10 ' in row[1]]
        for seq in (11, 12):
            self.events[-1:-1] = [[10 + seq, s.replace('seq=10 ', f'seq={seq} ')] for _, s in prototype]
        self.events = [row for row in self.events if not any(f'seq={seq} group=4 ' in row[1] for seq in (3, 4, 5))]
        with self.assertRaisesRegex(AssertionError, '20%'):
            self.result(True)


if __name__ == '__main__':
    unittest.main()
