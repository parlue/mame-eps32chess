"""Invalidate direct page pointers before any upstream bank-base mutation."""
import argparse
from pathlib import Path

HOOK = 's3_flat_bank_changed();'
SIGNATURES = (
    'void memory_bank::set_base(void *base)',
    'void memory_bank::set_entry(int entrynum)',
    'void memory_bank::configure_entry(int entrynum, void *base)',
    'void memory_bank::configure_entries(int startentry, int numentries, void *base, offs_t stride)',
)


def transform(text):
    original = text
    include = '#include "emu.h"'
    declaration = '\nextern "C" void s3_flat_bank_changed();'
    assert text.count(include) == 1 and HOOK not in text
    text = text.replace(include, include + declaration)
    anchors = ('\t// set the base\n', '\tm_curentry = entrynum;\n',
               "\t// if we haven't allocated this many entries yet, expand our array\n",
               '\tif (startentry + numentries >= int(m_entries.size()))\n')
    # Find only the named bodies, not similarly named code elsewhere.
    for signature, anchor in zip(SIGNATURES, anchors):
        assert text.count(signature) == 1
        start = text.index(signature)
        stop = text.find('\n}\n', start) + 3
        assert stop > start
        body = text[start:stop]
        assert body.count(anchor) == 1
        text = text[:start] + body.replace(anchor, '\t' + HOOK + '\n' + anchor) + text[stop:]
    assert text.count('\t' + HOOK) == 4
    init = 'void memory_manager::initialize()\n{\n'
    postload = ('\tmachine().save().register_postload(save_prepost_delegate([] { s3_flat_bank_changed(); }, "s3 flat pages bank restore"));\n'
                '\tstd::printf("[MAME FLAT RESTORE] postload_registered=1\\n");\n')
    assert text.count(init) == 1
    text = text.replace(init, init + postload)
    assert text.replace(postload, '').replace(declaration, '').replace('\t' + HOOK + '\n', '') == original
    return text


def prepare(source, destination):
    assert source.resolve() != destination.resolve()
    text = transform(source.read_text(encoding='utf-8'))
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('Flat-page bank overlay: four pre-mutation hooks plus postload invalidation; upstream layouts preserved')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('destination', type=Path)
    a = p.parse_args()
    prepare(a.source, a.destination)
