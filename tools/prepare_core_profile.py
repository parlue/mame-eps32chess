"""Add diagnostic coarse scopes, preserving source bodies and header layouts."""
from pathlib import Path

SIGNATURES = {
    'cpu': [('void m6502_device::execute_run()', 'cpu')],
    'schedule': [('void device_scheduler::timeslice()', 'scheduler'),
                 ('inline void device_scheduler::execute_timers()', 'timers')],
    'video': [('void video_manager::frame_update(bool from_debugger)', 'video')],
    'ioport': [('void ioport_manager::frame_update()', 'ports')],
    'sound': [('void sound_manager::update(void *ptr, int param)', 'sound')],
    'osd': [('void s3_osd::update(bool skip_redraw)', 'osd')],
}


def prepare(source, destination, kind):
    original = source.read_text(encoding='utf-8')
    text = original
    added = []
    for signature, bucket in SIGNATURES[kind]:
        marker = signature + '\n{'
        assert text.count(marker) == 1, (source, signature)
        extra = f'\n    s3_core_profile::scope s3_core_scope(s3_core_profile::{bucket});'
        text = text.replace(marker, marker + extra)
        added.append(extra)
    if kind == 'osd':
        marker = 'void s3_osd::init(running_machine &machine)\n{'
        assert text.count(marker) == 1
        extra = '\n    s3_core_profile::reset();'
        text = text.replace(marker, marker + extra)
        added.append(extra)
    restored = text
    for extra in added:
        restored = restored.replace(extra, '')
    assert restored == original, 'Only scope/reset statements may be added'
    # Relocating the OSD source must retain its relative include resolution.
    import re
    def include(match):
        path = source.parent / match[1]
        return '#include "' + path.resolve().as_posix() + '"' if path.is_file() else match[0]
    text = re.sub(r'#include "([^"\n]+)"', include, text)
    header = Path(__file__).resolve().parents[1] / 'esp32s3/components/mame0220/src/s3_core_profile.h'
    prefix = '#define S3_CORE_PROFILE_IMPLEMENTATION\n' if kind == 'schedule' else ''
    text = prefix + '#include "' + header.as_posix() + '"\n' + text
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('destination', type=Path)
    p.add_argument('kind', choices=SIGNATURES)
    a = p.parse_args()
    prepare(a.source, a.destination, a.kind)
