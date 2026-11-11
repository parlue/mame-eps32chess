"""Disable AVI/MNG recording without changing rendering, snapshots or timing."""
import argparse
from pathlib import Path
from prepare_no_desktop_text import replace_body


def prepare(source, destination):
    assert source.resolve() != destination.resolve()
    text = source.read_text(encoding='utf-8')
    for signature in [
        'void video_manager::begin_recording(const char *name, movie_format format)',
        'void video_manager::begin_recording_mng(const char *name, uint32_t index, screen_device *screen)',
        'void video_manager::begin_recording_avi(const char *name, uint32_t index, screen_device *screen)',
        'void video_manager::toggle_record_movie(movie_format format)',
    ]:
        text = replace_body(text, signature,
            '    throw emu_fatalerror("S3 chess profile: AVI/MNG movie recording is disabled");')
    # No recording can be started, so update/stop/audio hooks have no work.
    # Retain the original class layout, empty containers and their destruction.
    for signature in [
        'void video_manager::end_recording(movie_format format)',
        'void video_manager::end_recording_mng(uint32_t index)',
        'void video_manager::end_recording_avi(uint32_t index)',
        'void video_manager::add_sound_to_recording(const s16 *sound, int numsamples)',
        'void video_manager::add_sound_to_avi_recording(const s16 *sound, int numsamples, uint32_t index)',
        'void video_manager::record_frame()',
    ]:
        text = replace_body(text, signature, '    // Movie recording is disabled in this profile.')
    text = replace_body(text, 'bool video_manager::is_recording() const', '    return false;')
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('Movie recording overlay prepared')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
