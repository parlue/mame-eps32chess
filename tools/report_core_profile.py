"""Validate exclusive nested MAME-core diagnostic intervals in a search window."""
import argparse
import json
import re
from pathlib import Path
from report_opcode_profile import logical_records

NAMES = ('scheduler', 'timers', 'cpu', 'video', 'ports', 'sound', 'osd')
RECORD = re.compile(r'\[MAME CORE\] seq=(\d+) wall_us=(\d+) exclusive_us=([0-9,]+) other_us=(\d+) calls=([0-9,]+) depth=(\d+)$')


def summarize(folder, allow_missing=False):
    evidence = json.loads((folder / 'evidence.json').read_text())
    assert evidence.get('measurement_kind') == 'instrumented-core-profile'
    assert evidence.get('clean_exit'), 'Require a clean ROM exit'
    events = json.loads((folder / 'events.json').read_text())
    start = next(t for t, s in events if s.startswith('[BENCH] search_begin'))
    end = next(t for t, s in events if s == '[BENCH] search_end')
    intervals = {}
    for timestamp, line in logical_records(events):
        if not start + 10 <= timestamp <= end or '[MAME CORE]' not in line:
            continue
        match = RECORD.search(line)
        assert match, 'Truncated/corrupt core profile record'
        seq, wall, values, other, calls, depth = match.groups()
        seq, wall, other, depth = map(int, (seq, wall, other, depth))
        values, calls = ([int(x) for x in group.split(',')] for group in (values, calls))
        assert len(values) == len(calls) == len(NAMES), 'Wrong core bucket count'
        assert 4000000 <= wall <= 15000000 and 1 <= depth <= 32
        assert sum(values) + other == wall, 'Exclusive times do not partition the interval'
        assert calls[0] and calls[1] and calls[2], 'Missing scheduler/timer/CPU scopes'
        assert seq not in intervals, 'Duplicate interval'
        intervals[seq] = {'wall': wall, 'values': values, 'other': other, 'calls': calls, 'depth': depth}
    assert len(intervals) >= 8, 'Insufficient complete core intervals'
    first, last = min(intervals), max(intervals)
    assert last - first < 1000
    missing = sorted(set(range(first, last + 1)) - intervals.keys())
    assert allow_missing or not missing, 'Missing interior core interval'
    assert len(intervals) / (last - first + 1) >= .8, 'More than 20% of intervals lost'
    wall = sum(x['wall'] for x in intervals.values())
    rows = []
    for i, name in enumerate(NAMES):
        value = sum(x['values'][i] for x in intervals.values())
        rows.append({'scope': name, 'exclusive_us': value, 'wall_percent': 100 * value / wall,
                     'calls': sum(x['calls'][i] for x in intervals.values())})
    other = sum(x['other'] for x in intervals.values())
    return {'driver': evidence['driver'], 'case': evidence['case'], 'intervals': sorted(intervals),
            'missing_intervals': missing, 'measured_wall_seconds': wall / 1e6,
            'scopes': sorted(rows, key=lambda x: -x['exclusive_us']),
            'unscoped_wall_percent': 100 * other / wall,
            'maximum_depth': max(x['depth'] for x in intervals.values()),
            'limits': 'Exclusive instrumented wall time includes task preemption and scope overhead. Timer scope includes unmeasured timer callbacks. Unscoped includes MAME work outside selected functions. Instrumented speed is not a candidate speed result.'}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('folder', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--allow-missing-intervals', action='store_true')
    a = p.parse_args()
    result = summarize(a.folder, a.allow_missing_intervals)
    a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
