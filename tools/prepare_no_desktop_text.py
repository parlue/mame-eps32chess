"""Remove desktop text posting and fuzzy software-name suggestions via overlays.

No Unicode codecs, character tables, exact lookups or input-port setters are
modified. Original class layouts/initializers and copyright headers remain.
Posting/configuring a desktop text keyboard fails explicitly; queries see an
idle object. A missing software name yields no approximate suggestions.
"""
import argparse
from pathlib import Path
import re


def replace_body(text, signature, body):
    assert text.count(signature) == 1, 'Unexpected function: ' + signature
    start = text.index('{', text.index(signature))
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: ' ' * len(m.group()), text)
    depth, end = 1, start + 1
    while depth:
        assert end < len(masked), 'Unbalanced function: ' + signature
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return text[:start] + '{\n' + body + '\n}' + text[end:]


def prepare(source, destination, kind):
    assert source.resolve() != destination.resolve()
    text = source.read_text(encoding='utf-8')
    if kind == 'keyboard':
        assert 'build_codes(machine.ioport());' in text and 'KEY_BUFFER_SIZE = 4096' in text
        error = '    throw emu_fatalerror("S3 chess profile: desktop text keyboard is disabled");'
        text = replace_body(text, 'natural_keyboard::natural_keyboard(running_machine &machine)',
                            '    if (machine.options().natural_keyboard())\n' + error)
        signatures = [
            'void natural_keyboard::configure(ioport_queue_chars_delegate queue_chars, ioport_accept_char_delegate accept_char, ioport_charqueue_empty_delegate charqueue_empty)',
            'void natural_keyboard::post_char(char32_t ch, bool normalize_crlf)',
            'void natural_keyboard::post(const char32_t *text, size_t length, const attotime &rate)',
            'void natural_keyboard::post_utf8(const char *text, size_t length, const attotime &rate)',
            'void natural_keyboard::post_utf8(const std::string &text, const attotime &rate)',
            'void natural_keyboard::post_coded(const char *text, size_t length, const attotime &rate)',
            'void natural_keyboard::post_coded(const std::string &text, const attotime &rate)',
            'void natural_keyboard::paste()',
        ]
        for signature in signatures:
            text = replace_body(text, signature, error)
        text = replace_body(text, 'void natural_keyboard::set_in_use(bool usage)',
                            '    if (usage)\n' + error + '\n    m_in_use = false;')
        text = replace_body(text, 'void natural_keyboard::dump(std::ostream &str) const',
                            '    str << "Desktop text keyboard disabled\\n";')
        text = replace_body(text, 'std::string natural_keyboard::dump() const',
                            '    return "Desktop text keyboard disabled\\n";')
    else:
        assert 'normalize_unicode(swinfo.longname()' in text
        text = replace_body(text,
            'void software_list_device::find_approx_matches(const std::string &name, int matches, const software_info **list, const char *interface)',
            '''    // Preserve ordinary exact lookup/error reporting, omit only suggestions.
    if (list)
        for (int i = 0; i < matches; ++i)
            list[i] = nullptr;
    osd_printf_verbose("S3 chess profile: approximate software-name suggestions disabled\\n");''')
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('Desktop text overlay prepared:', kind)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kind', choices=['keyboard', 'software'])
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination, args.kind)
