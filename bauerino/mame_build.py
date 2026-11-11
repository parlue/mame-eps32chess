"""Build the selected MAME sources with the frontend's own Arduino/IDF SDK."""
Import('env')
import hashlib
import json
import shlex
import subprocess
import sys
from pathlib import Path

project = Path(env.subst('$PROJECT_DIR')).resolve()
root = project.parent
env.Append(ASFLAGS=['-Wa,-I' + str(root / 'esp32s3/src')])
database = root / 'esp32s3/.pio/build/esp32s3-skeleton/compile_commands.json'
entries = json.loads(database.read_text(encoding='utf-8'))
objects = []
no_desktop_text = env.GetProjectOption('custom_mame_desktop_text', 'no').lower() in ('no', 'false', '0')
text_sources = {
    'natkeyboard.cpp': ('keyboard', 'natkeyboard_text_s3.cpp'),
    'softlist_dev.cpp': ('software', 'softlist_dev_text_s3.cpp'),
}
text_selected = 0
if no_desktop_text:
    for original, (kind, overlay) in text_sources.items():
        subprocess.run([sys.executable, str(root / 'tools/prepare_no_desktop_text.py'), kind,
            str(root / 'ref/mame0220-src/src/emu' / original),
            str(root / 'esp32s3/components/mame0220/generated' / overlay)], check=True)
restricted_roots = env.GetProjectOption('custom_mame_65xx_roots', 'yes').lower() in ('yes', 'true', '1')
memory_base = root / 'esp32s3/components/mame0220/generated/emumem_s3.cpp'
memory_roots = memory_base.with_name('emumem_roots_s3.cpp')
if restricted_roots:
    subprocess.run([sys.executable, str(root / 'tools/prepare_65xx_roots.py'),
        str(memory_base), str(memory_roots)], check=True)
memory_sources = 0
no_debug_commands = env.GetProjectOption('custom_mame_no_debug_commands', 'yes').lower() in ('yes', 'true', '1')
debug_overlay = root / 'esp32s3/components/mame0220/generated/debugcmd_s3.cpp'
if no_debug_commands:
    subprocess.run([sys.executable, str(root / 'tools/prepare_no_debug_commands.py'),
        str(root / 'ref/mame0220-src/src/emu/debug/debugcmd.cpp'), str(debug_overlay)], check=True)
groups = {}
for entry in entries:
    source = Path(entry['file']).resolve()
    normalized = source.as_posix()
    if not source.is_relative_to(root) or '/.pio/' in normalized:
        continue
    if not any(part in normalized for part in ('/components/mame0220/', '/ref/mame0220-src/', '/integration/chessengine/src/')):
        continue
    tokens = shlex.split(entry['command'])
    for original, (_, overlay) in text_sources.items():
        if source.name in (original, overlay):
            source = (root / 'esp32s3/components/mame0220/generated' / overlay) if no_desktop_text else (root / 'ref/mame0220-src/src/emu' / original)
            normalized = source.as_posix()
            text_selected += 1
            break
    if source.name in ('emumem_s3.cpp', 'emumem_roots_s3.cpp'):
        source = memory_roots if restricted_roots else memory_base
        normalized = source.as_posix()
        memory_sources += 1
    if source.name in ('debugcmd.cpp', 'debugcmd_s3.cpp'):
        source = debug_overlay if no_debug_commands else root / 'ref/mame0220-src/src/emu/debug/debugcmd.cpp'
        normalized = source.as_posix()
    includes = []
    definitions = []
    for token in tokens:
        if token.startswith('-I'):
            path = Path(token[2:])
            if not path.is_absolute():
                path = Path(entry['directory']) / path
            path = path.resolve()
            if path.is_relative_to(root) and '/.pio/' not in path.as_posix():
                includes.append(str(path))
        elif token.startswith('-D'):
            name, separator, value = token[2:].partition('=')
            if name in ('IDF_VER', '__INT32_TYPE__', '__UINT32_TYPE__', 'ESP_PLATFORM') or name.startswith('SOC_'):
                continue
            if separator:
                if value.startswith('"') and value.endswith('"'):
                    value = env.StringifyMacro(value[1:-1])
                definitions.append((name, value))
            else:
                definitions.append(name)
    signature = repr((includes, definitions))
    if signature not in groups:
        core = env.Clone()
        core.ProcessUnFlags('-fno-rtti -fno-exceptions')
        core.Append(CPPPATH=includes, CPPDEFINES=definitions + ['BAUERINO_MAME_FRONTEND'])
        core.Append(CCFLAGS=['-fsigned-char', '-mtext-section-literals', '-mlongcalls',
            '-fno-merge-constants', '-Wno-error', '-Wno-format',
            '-include', str(project / 'mame_types.h'),
            '-include', str(root / 'esp32s3/components/mame0220/src/xtensa_posix_shim.h')])
        core.Append(CXXFLAGS=['-std=gnu++17', '-frtti', '-fexceptions'])
        groups[signature] = core
    core = groups[signature]
    key = hashlib.sha256(normalized.encode()).hexdigest()[:12]
    objects.extend(core.Object(str(Path(env.subst('$BUILD_DIR')) / 'mame' / (source.stem + '-' + key)), str(source)))

assert len(objects) > 100, 'Missing MAME compilation manifest'
assert memory_sources == 1, 'Expected exactly one owning MAME memory source'
assert text_selected == 2, 'Expected exactly two desktop-text profile sources'
library = env.StaticLibrary('$BUILD_DIR/mame0220', objects)
env.Append(LIBS=[library], LINKFLAGS=['-Wl,--wrap=__cxa_throw', '-frtti'])
print('Common-SDK MAME build:', len(objects), 'sources;', len(groups), 'compile profiles')
