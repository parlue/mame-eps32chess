"""Include the unchanged 6502/65C02 sources in one compiler translation unit."""
import argparse
from pathlib import Path


def prepare(base, cmos, destination):
    assert destination.resolve() not in (base.resolve(), cmos.resolve())
    assert 'm6502_device::execute_run()' in base.read_text(encoding='utf-8')
    assert 'DEFINE_DEVICE_TYPE(M65C02,' in cmos.read_text(encoding='utf-8')
    text = ('// Generated CPU-only unity compilation; source bodies are unchanged.\n'
            f'#include "{base.resolve().as_posix()}"\n'
            f'#include "{cmos.resolve().as_posix()}"\n')
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('CPU unity source prepared: 6502 + 65C02')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base', type=Path)
    parser.add_argument('cmos', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.base, args.cmos, args.destination)
