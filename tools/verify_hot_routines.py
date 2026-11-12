"""Check selected CPU code placement and its actual shared SRAM cost before flash."""
import argparse
import json
import re
import subprocess
from pathlib import Path


def symbols(nm, elf):
    text = subprocess.run([str(nm), '-S', '-C', str(elf)], check=True, capture_output=True, text=True).stdout
    result = {}
    for line in text.splitlines():
        match = re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) [Tt] m6502_device::device_start\(\)::paged_memory_interface::(read|write|read_arg|read_sync)\(unsigned short(?:, unsigned char)?\)', line)
        if match:
            result['paged_memory_interface::' + match[3]] = {'address': int(match[1], 16), 'bytes': int(match[2], 16)}
        match = re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) [Tt] (m(?:6502|65c02)_device::\w+)\(\)', line)
        if match:
            entry = result.setdefault(match[3], {})
            entry.update(address=int(match[1], 16), bytes=int(match[2], 16))
        # This Xtensa toolchain leaves GCC's $part$ clones mangled even with -C.
        # They contain the body of partially inlined instruction wrappers.
        match = re.fullmatch(r'([0-9a-f]+) ([0-9a-f]+) [Tt] _ZN(12m6502_device|13m65c02_device)(\d+)(\w+)Ev\$part\$\d+', line)
        if match:
            cls = match[3].lstrip('0123456789')
            method = match[5]
            assert len(method) == int(match[4])
            name = f'{cls}::{method}'
            clone = {'address': int(match[1], 16), 'bytes': int(match[2], 16)}
            entry = result.setdefault(name, {'address': None, 'bytes': 0})
            entry.setdefault('parts', []).append(clone)
    for entry in result.values():
        entry['body_bytes_with_parts'] = entry['bytes'] + sum(p['bytes'] for p in entry.get('parts', []))
    return result


def shared_iram_reserved(path):
    text = path.read_text(errors='replace')
    match = re.search(r'^\.dram0.dummy\s+0x[0-9a-f]+\s+0x([0-9a-f]+)', text, re.M)
    assert match, 'Expected S3 linker overlap reservation'
    return int(match[1], 16)


def static_dram_end(path):
    text = path.read_text(errors='replace')
    match = re.search(r'^\s+0x([0-9a-f]+)\s+_bss_end = ABSOLUTE', text, re.M)
    assert match, 'Expected S3 static DRAM end symbol'
    return int(match[1], 16)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--nm', type=Path, required=True)
    parser.add_argument('--baseline-map', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--hot-direct', action='store_true', help='Require the bounded inline-memory experiment marker')
    parser.add_argument('--hot-memory', action='store_true', help='Require four shared-memory methods in IRAM')
    parser.add_argument('--hot-metadata', action='store_true', help='Require the opt-in low-page metadata profile')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    plan = json.loads((root / 'tools/65xx_hot_routines.json').read_text())
    syms = symbols(args.nm, args.build / 'firmware.elf')
    selected = {}
    for name in plan['routines']:
        assert name in syms, f'Missing selected function: {name}'
        info = syms[name]
        for piece in [info] + info.get('parts', []):
            assert piece['address'] is not None and 0x40374000 <= piece['address'] < 0x403cb700, f'Not in internal instruction RAM: {name}'
        selected[name] = info
    memory = {}
    for method in ('read', 'write', 'read_arg', 'read_sync'):
        name = 'paged_memory_interface::' + method
        assert name in syms, 'Missing shared memory entry: ' + name
        info = syms[name]
        in_iram = 0x40374000 <= info['address'] < 0x403cb700
        assert in_iram == args.hot_memory, 'Wrong shared-memory placement: ' + name
        memory[name] = info
    iram_extra = shared_iram_reserved(args.build / 'firmware.map') - shared_iram_reserved(args.baseline_map)
    extra = static_dram_end(args.build / 'firmware.map') - static_dram_end(args.baseline_map)
    assert 0 < extra <= plan['extra_iram_budget_bytes'], f'Actual shared SRAM cost {extra} exceeds bounded budget'
    binary = (args.build / 'firmware.bin').read_bytes()
    assert b'[MAME CORE BEGIN]' not in binary, 'Coarse core diagnostic is not a speed candidate'
    assert b'[MAME OPCODE COUNTS]' not in binary and b'[MAME DIRECT MEMORY]' not in binary, 'Diagnostic/other experimental path still present'
    assert (b'[MAME HOT DIRECT]' in binary) == args.hot_direct, 'Wrong inline-memory profile in application'
    assert (b'[MAME HOT METADATA]' in binary) == args.hot_metadata, 'Wrong metadata placement profile in application'
    result = {'extra_shared_sram_bytes': extra, 'extra_iram_reserved_bytes': iram_extra,
              'budget_bytes': plan['extra_iram_budget_bytes'], 'selected': selected, 'shared_memory': memory,
              'hot_metadata': args.hot_metadata,
              'note': 'Function text plus literal/alignment cost is bounded by actual linker SRAM reservation; runtime heap still requires device validation.'}
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
