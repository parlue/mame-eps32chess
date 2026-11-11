"""Skip unused final stereo conversion; retain stream generation/accounting."""
from pathlib import Path


def prepare(source, destination):
    text = source.read_text(encoding='utf-8')
    start_marker = '\t// now downmix the final result\n'
    end_marker = '\t// see if we ticked over to the next second\n'
    assert text.count(start_marker) == text.count(end_marker) == 1
    start, end = text.index(start_marker), text.index(end_marker)
    original = text[start:end]
    replacement = '''    // Only the platform's discarded final output may be omitted. Speaker
    // streams were generated above; all original accounting remains below.
    const bool discard_final = m_nosound_mode && m_wavfile == nullptr
        && !machine().video().is_recording();
    if (discard_final) {
        const u32 step = machine().video().speed_factor();
        const u32 limit = u32(m_samples_this_update) * 1000;
        const u32 first = u32(m_finalmix_leftover);
        assert(step && m_finalmix_leftover >= 0);
        m_finalmix_leftover = s3_discard_mix_remainder(first, limit, step);
    } else {
''' + original + '    }\n\n'
    result = text[:start] + replacement + text[end:]
    assert result.replace(replacement, original, 1) == text
    marker = '\t// get filename for WAV file or AVI file if specified\n'
    assert result.count(marker) == 1
    result = result.replace(marker, '    std::printf("[MAME DISCARD MIX] conditional nosound/no-record; streams preserved\\n");\n' + marker)
    header = Path(__file__).resolve().parents[1] / 'esp32s3/components/mame0220/src/s3_discard_mix.h'
    result = '#include <cstdio>\n#include "' + header.as_posix() + '"\n' + result
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != result:
        destination.write_text(result, encoding='utf-8', newline='\n')


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('destination', type=Path)
    a = p.parse_args()
    prepare(a.source, a.destination)
