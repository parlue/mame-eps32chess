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
library = env.StaticLibrary('$BUILD_DIR/mame0220', objects)
env.Append(LIBS=[library], LINKFLAGS=['-Wl,--wrap=__cxa_throw', '-frtti'])
print('Common-SDK MAME build:', len(objects), 'sources;', len(groups), 'compile profiles')
