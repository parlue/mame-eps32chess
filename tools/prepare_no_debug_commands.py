"""Disable desktop debugger command registration in a separate source overlay.

Retain declarations and validation helpers used elsewhere. Without registration
the linker can discard command implementations and their help/cheat dependencies.
An unexpected debugger activation fails explicitly instead of offering partial
commands. Upstream and generated compact source view remain untouched.
"""
import argparse
from pathlib import Path
import re


def prepare(source: Path, destination: Path):
    assert source.resolve() != destination.resolve()
    text = source.read_text(encoding='utf-8')
    anchor = 'debugger_commands::debugger_commands(running_machine& machine, debugger_cpu& cpu, debugger_console& console)'
    assert text.count(anchor) == 1, 'Unexpected debugger constructor'
    start = text.index('{', text.index(anchor))
    # Mask comments and literals while preserving positions for brace matching.
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: ' ' * len(m.group()), text)
    depth = 1
    end = start + 1
    while depth:
        assert end < len(masked), 'Unbalanced debugger constructor'
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    original = text[start:end]
    assert 'register_command("help"' in original and 'm_global_array' in original
    text = text[:start] + '''{
    throw emu_fatalerror("S3 chess profile: desktop debugger commands are disabled");
}''' + text[end:]
    # Source-local debugger headers now resolve from the shared emu include root.
    text = re.sub(r'#include "([^"/]+)"',
                  lambda m: '#include "debug/' + m[1] + '"' if (source.parent / m[1]).is_file() else m[0], text)
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('S3 debugger-command overlay prepared; unexpected activation is rejected')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
