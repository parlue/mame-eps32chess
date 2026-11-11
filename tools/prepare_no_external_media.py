"""Omit ROM-disk loading, software catalogs and optional HSI metadata.

Raw ROM-region loading, CRC/SHA validation, archive handling, class layout and
cleanup remain unchanged. Unsupported CHD/catalog operations fail explicitly;
optional extrainfo lookup returns the original 'not found' result.
"""
import argparse
from pathlib import Path
from prepare_no_desktop_text import replace_body


def prepare(source, destination, kind):
    assert source.resolve() != destination.resolve()
    text = source.read_text(encoding='utf-8')
    if kind == 'rom':
        assert 'void rom_load_manager::verify_length_and_hash(' in text
        assert 'm_chd_list.push_back' in text
        signatures = [
            'int rom_load_manager::set_disk_handle(const char *region, const char *fullpath)',
            'int open_disk_image(emu_options &options, const game_driver *gamedrv, const rom_entry *romp, chd_file &image_chd, const char *locationtag)',
            'chd_error rom_load_manager::open_disk_diff(emu_options &options, const rom_entry *romp, chd_file &source, chd_file &diff_chd)',
            'void rom_load_manager::process_disk_entries(const char *regiontag, const rom_entry *parent_region, const rom_entry *romp, const char *locationtag)',
        ]
        for signature in signatures:
            text = replace_body(text, signature,
                '    throw emu_fatalerror("S3 chess profile: CHD disk images are disabled");')
    elif kind == 'catalog':
        assert 'softlist_parser parser(' in text
        text = replace_body(text, 'void software_list_device::parse()',
            '    throw emu_fatalerror("S3 chess profile: external software catalogs are disabled");')
    else:
        assert 'pugi::xml_document doc;' in text
        for signature in [
            'bool hashfile_extrainfo(const char *hash_path, const game_driver &driver, const util::hash_collection &hashes, std::string &result)',
            'bool hashfile_extrainfo(device_image_interface &image, std::string &result)',
        ]:
            text = replace_body(text, signature,
                '    // Optional .hsi extrainfo is absent; ROM checksum validation is separate.\n    return false;')
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('External media overlay prepared:', kind)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kind', choices=['rom', 'catalog', 'metadata'])
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination, args.kind)
