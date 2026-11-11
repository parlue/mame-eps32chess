"""Validate diagnostic opcode intervals and map counts to generated CPU routines."""
import argparse
import json
import re
from collections import Counter
from pathlib import Path


def logical_records(events):
    # Serial.printf from the frontend can interrupt stdout midway through a
    # decimal count. Rejoin only this known, complete LIVE record insertion;
    # unknown corruption is left intact and fails the group/sum checks below.
    pending = ''
    pending_time = None
    for timestamp, line in events:
        match = re.search(r'\[LIVE\] t=\d+ clients=\d+ heap=\d+ engine=\d+ error=.*$', line)
        if match:
            prefix = line[:match.start()]
            if prefix:
                if pending_time is None:
                    pending_time = timestamp
                pending += prefix
            continue
        yield pending_time if pending_time is not None else timestamp, pending + line
        pending = ''
        pending_time = None
    assert not pending.startswith('[MAME OPCODE'), 'Unfinished opcode record'


def routine_map(root):
    headers = {name: (root / f'esp32s3/components/mame0220/include/cpu/m6502/{name}.hxx').read_text()
               for name in ('m6502', 'm65c02')}
    definitions = {name: set(re.findall(r'void \w+_device::(\w+_full)\(\)', text))
                   for name, text in headers.items()}
    text = headers['m65c02'].split('void m65c02_device::do_exec_full()', 1)[1].split('void m65c02_device::do_exec_partial()', 1)[0]
    mapping = {}
    for opcode, method in re.findall(r'case 0x([0-9a-f]{2}): (\w+_full)\(\); break;', text):
        owner = 'm65c02' if method in definitions['m65c02'] else 'm6502'
        assert method in definitions[owner], method
        mapping[int(opcode, 16)] = f'{owner}_device::{method}'
    assert len(mapping) == 256, 'Expected complete current CMOS dispatch map'
    return mapping


def summarize(folder, mapping, allow_missing=False):
    evidence = json.loads((folder / 'evidence.json').read_text())
    assert evidence.get('measurement_kind') == 'instrumented-opcode-profile', 'Require an explicitly instrumented run'
    events = json.loads((folder / 'events.json').read_text())
    start = next(t for t, s in events if s.startswith('[BENCH] search_begin'))
    end = next(t for t, s in events if s == '[BENCH] search_end')
    intervals = {}
    for timestamp, line in logical_records(events):
        if not start + 10 <= timestamp <= end:
            continue
        match = re.search(r'\[MAME OPCODE TOTAL\] seq=(\d+) full=(\d+) partial=(\d+) special=(\d+)\b', line)
        if match:
            seq, full, partial, special = map(int, match.groups())
            record = intervals.setdefault(seq, {'groups': {}})
            assert 'full' not in record, 'Duplicate interval header'
            record.update(full=full, partial=partial, special=special)
        match = re.search(r'\[MAME OPCODE COUNTS\] seq=(\d+) group=([0-7]) counts=([0-9,]+)', line)
        if match:
            seq, group = int(match[1]), int(match[2])
            values = [int(x) for x in match[3].split(',')]
            assert len(values) == 32, 'Truncated/interleaved count record'
            record = intervals.setdefault(seq, {'groups': {}})
            assert group not in record['groups'], 'Duplicate count group'
            record['groups'][group] = values
    counts = Counter()
    partial = special = full = 0
    accepted = []
    excluded = {}
    assert intervals, 'No opcode intervals in search window'
    first, last = min(intervals), max(intervals)
    assert last - first < 1000, 'Unexpected opcode sequence span'
    for seq in range(first, last + 1):
        intervals.setdefault(seq, {'groups': {}})
    # Boundary fragments may straddle the search window. Interior corruption
    # fails rather than silently changing the apparent routine distribution.
    for seq, record in sorted(intervals.items()):
        complete = 'full' in record and len(record['groups']) == 8
        if not complete and seq in (min(intervals), max(intervals)):
            excluded[seq] = 'incomplete boundary interval'
            continue
        if not complete and allow_missing:
            excluded[seq] = 'missing header or entire count group; whole interval excluded'
            continue
        assert complete, f'Incomplete interior opcode interval {seq}'
        values = sum((record['groups'][g] for g in range(8)), [])
        assert sum(values) == record['full'], f'Count sum mismatch in interval {seq}'
        counts.update(dict(enumerate(values)))
        full += record['full']; partial += record['partial']; special += record['special']
        accepted.append(seq)
    assert len(accepted) >= 8 and full > 0, 'Insufficient complete instruction intervals'
    assert len(accepted) / len(intervals) >= .8, 'More than 20% of opcode intervals were lost'
    routines = Counter()
    for opcode, count in counts.items():
        routines[mapping[opcode]] += count
    return {'driver': evidence['driver'], 'case': evidence['case'], 'intervals': accepted,
            'excluded_intervals': excluded, 'accepted_interval_fraction': len(accepted) / len(intervals),
            'full_instruction_starts': full, 'partial_resume_calls': partial, 'special_dispatches': special,
            'routines': [{'routine': name, 'starts': count, 'percent': 100 * count / full}
                         for name, count in routines.most_common() if count],
            'opcodes': {f'{op:02x}': count for op, count in sorted(counts.items()) if count},
            'limits': 'Exact full-dispatch starts, including BRK/interrupt dispatch. Frequencies are not wall-time attribution. Instrumented speed is not comparable to normal firmware.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--allow-missing-intervals', action='store_true', help='Explicitly exclude intervals with entirely missing records; require at least 80% complete. Corrupt counts still fail.')
    args = parser.parse_args()
    result = summarize(args.folder, routine_map(Path(__file__).resolve().parents[1]), args.allow_missing_intervals)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ('driver', 'case', 'full_instruction_starts', 'partial_resume_calls')}, indent=2))
    for row in result['routines'][:16]:
        print(f"{row['percent']:6.2f}% {row['routine']}")
    print('Excluded intervals:', result['excluded_intervals'])
