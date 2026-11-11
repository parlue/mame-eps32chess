"""Build the selected MAME sources with the frontend's own Arduino/IDF SDK."""
Import('env')
import hashlib
import os
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
speed_base = root / 'esp32s3/components/mame0220/generated/m6502_s3.cpp'
speed_overlay = speed_base.with_name('m6502_speed_s3.cpp')
direct_memory = os.environ.get('BAUERINO_MAME_DIRECT_MEMORY') == '1'
subprocess.run([sys.executable, str(root / 'tools/prepare_65xx_speed.py'),
    str(speed_base), str(speed_overlay)]
    + (['--memory-probe'] if os.environ.get('BAUERINO_MAME_MEMORY_PROBE') == '1' else [])
    + (['--direct-memory'] if direct_memory else []), check=True)
speed_selected = 0
cpu_unity = env.GetProjectOption('custom_mame_cpu_unity', 'yes').lower() in ('yes', 'true', '1')
unity_overlay = speed_base.with_name('m6502_unity_s3.cpp')
cmos_selected = 0
assert not direct_memory or cpu_unity, 'Direct-memory experiment requires the CPU unity translation unit'
cmos_source = speed_base.with_name('m65c02_direct_s3.cpp') if direct_memory else root / 'ref/mame0220-src/src/devices/cpu/m6502/m65c02.cpp'
if cpu_unity:
    cpu_entries = [e for e in entries if Path(e['file']).name in ('m6502_s3.cpp', 'm6502_speed_s3.cpp', 'm65c02.cpp')]
    assert len(cpu_entries) == 2, 'Expected two CPU manifest entries before unity selection'
    profiles = [[t for t in shlex.split(e['command']) if t.startswith(('-I', '-D'))] for e in cpu_entries]
    assert profiles[0] == profiles[1], 'CPU unity requires identical include/define profiles'
    subprocess.run([sys.executable, str(root / 'tools/prepare_cpu_unity.py'), str(speed_overlay),
        str(cmos_source), str(unity_overlay)], check=True)
driver_overlay = root / 'esp32s3/components/mame0220/generated/mephisto_internal_s3.cpp'
subprocess.run([sys.executable, str(root / 'tools/prepare_mephisto_internal_cpu.py'),
    str(root / 'ref/mame0220-src/src/mame/drivers/mephisto_mm2.cpp'), str(driver_overlay)], check=True)
driver_selected = 0
no_movie_recording = env.GetProjectOption('custom_mame_movie_recording', 'no').lower() in ('no', 'false', '0')
movie_overlay = root / 'esp32s3/components/mame0220/generated/video_nomovie_s3.cpp'
if no_movie_recording:
    subprocess.run([sys.executable, str(root / 'tools/prepare_no_movie_recording.py'),
        str(root / 'ref/mame0220-src/src/emu/video.cpp'), str(movie_overlay)], check=True)
movie_selected = 0
no_external_media = env.GetProjectOption('custom_mame_external_media', 'no').lower() in ('no', 'false', '0')
media_sources = {
    'romload.cpp': ('rom', 'romload_nomedia_s3.cpp'),
    'hashfile.cpp': ('metadata', 'hashfile_nomedia_s3.cpp'),
    'softlist_dev.cpp': ('catalog', 'softlist_dev_nomedia_s3.cpp'),
}
media_selected = 0
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
if no_external_media:
    for original, (kind, overlay) in media_sources.items():
        media_input = (root / 'esp32s3/components/mame0220/generated/softlist_dev_text_s3.cpp') if original == 'softlist_dev.cpp' and no_desktop_text else (root / 'ref/mame0220-src/src/emu' / original)
        subprocess.run([sys.executable, str(root / 'tools/prepare_no_external_media.py'), kind,
            str(media_input), str(root / 'esp32s3/components/mame0220/generated' / overlay)], check=True)
for entry in entries:
    source = Path(entry['file']).resolve()
    normalized = source.as_posix()
    if not source.is_relative_to(root) or '/.pio/' in normalized:
        continue
    if not any(part in normalized for part in ('/components/mame0220/', '/ref/mame0220-src/', '/integration/chessengine/src/')):
        continue
    if source.name == 'm65c02.cpp':
        cmos_selected += 1
        if cpu_unity:
            continue
    tokens = shlex.split(entry['command'])
    if source.name in ('mephisto_mm2.cpp', 'mephisto_internal_s3.cpp'):
        source = driver_overlay
        normalized = source.as_posix()
        driver_selected += 1
    if source.name in ('m6502_s3.cpp', 'm6502_speed_s3.cpp'):
        source = unity_overlay if cpu_unity else speed_overlay
        normalized = source.as_posix()
        speed_selected += 1
    if source.name in ('video.cpp', 'video_nomovie_s3.cpp'):
        source = movie_overlay if no_movie_recording else root / 'ref/mame0220-src/src/emu/video.cpp'
        normalized = source.as_posix()
        movie_selected += 1
    for original, (_, overlay) in text_sources.items():
        if source.name in (original, overlay) or (original == 'softlist_dev.cpp' and source.name == 'softlist_dev_nomedia_s3.cpp'):
            source = (root / 'esp32s3/components/mame0220/generated' / overlay) if no_desktop_text else (root / 'ref/mame0220-src/src/emu' / original)
            normalized = source.as_posix()
            text_selected += 1
            break
    for original, (_, overlay) in media_sources.items():
        if source.name in (original, overlay) or (original == 'softlist_dev.cpp' and source.name == 'softlist_dev_text_s3.cpp'):
            if no_external_media:
                source = root / 'esp32s3/components/mame0220/generated' / overlay
            elif original != 'softlist_dev.cpp':
                source = root / 'ref/mame0220-src/src/emu' / original
            normalized = source.as_posix()
            media_selected += 1
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
    # Speed experiment limited to the two CPU interpreters. Preserve the
    # delegate identity fix even when selecting a different optimization level.
    if source.name in ('m6502_speed_s3.cpp', 'm6502_unity_s3.cpp', 'm65c02.cpp'):
        core = core.Clone()
        core.ProcessUnFlags('-Os -O1 -O2 -O3 -fno-jump-tables -fno-tree-switch-conversion')
        core.Append(CCFLAGS=['-O2', '-fjump-tables', '-ftree-switch-conversion', '-fno-merge-constants'])
        if os.environ.get('BAUERINO_MAME_CPU_ASM') == '1':
            core.Append(CCFLAGS=['-save-temps=obj'])
    key = hashlib.sha256(normalized.encode()).hexdigest()[:12]
    objects.extend(core.Object(str(Path(env.subst('$BUILD_DIR')) / 'mame' / (source.stem + '-' + key)), str(source)))

assert len(objects) > 100, 'Missing MAME compilation manifest'
assert memory_sources == 1, 'Expected exactly one owning MAME memory source'
assert text_selected == 2, 'Expected exactly two desktop-text profile sources'
assert media_selected == 3, 'Expected exactly three external-media profile sources'
assert movie_selected == 1, 'Expected exactly one movie-recording profile source'
assert speed_selected == 1, 'Expected exactly one owning 6502 speed source'
assert cmos_selected == 1, 'Expected exactly one 65C02 source before unity selection'
assert driver_selected == 1, 'Expected exactly one Mephisto allocation source'
library = env.StaticLibrary('$BUILD_DIR/mame0220', objects)
env.Append(LIBS=[library], LINKFLAGS=['-Wl,--wrap=__cxa_throw', '-frtti'])
print('Common-SDK MAME build:', len(objects), 'sources;', len(groups), 'compile profiles')
