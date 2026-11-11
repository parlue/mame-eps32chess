# Size experiment: omit desktop debugger commands

## Baseline

Combined Build 10: 6,154,599 application bytes and 160,788 static internal RAM
bytes. The full actual-ROM regression passed and the binary/source delta is
preserved separately. Combined Build 09 remains the physically accepted
animation, touch and phone-web baseline.

The allocated linker map shows substantial native 8-bit dispatch, Unicode,
debugger and rendering contributions. ELF debug information and discarded
sections must be excluded from size rankings: they are not flashed. The
`tools/report_mame_flash.py` tool reports MAME archive input sections in the
S3's mapped flash windows, with duplicate address/size entries removed. Those
section totals differ from total application image bytes and cannot be treated
as guaranteed removable space.

## Selected change

The product does not offer MAME's desktop debugger command console. However,
the debugger manager's constructor references command registration and thereby
retains help text, cheat/search commands, trace and dump utilities. The first
isolated experiment replaces only that registration constructor with an
explicit fatal error if an unexpected activation reaches it. Public validation
helpers and original class declarations remain available to other MAME code.
The linker can discard command implementations whose registration is gone.

This is not a claim that the entire debugger subsystem has been removed.
Device inspection and other core debug classes may still be retained. It does
not change the 6502/65C02 implementation, ROMs, Lua/UCI bridge, memory maps,
interrupts, timing or platform UI. Existing serial diagnostics, heap checks,
exception backtraces, rate and wall-time profile reports remain available.

Upstream is untouched. `tools/prepare_no_debug_commands.py` generates a
copyright-preserving source overlay, verifies the expected constructor,
matches its braces while masking comments/literals, and fixes source-local
include paths. It retains all remaining original function bodies.

- Native CMake switch: `MAME_S3_NO_DEBUG_COMMANDS` (default ON).
- Combined PlatformIO switch: `custom_mame_no_debug_commands = yes` in the
  `bauerino480` environment; `no` selects the original constructor.
- Both paths use the same overlay generator. Combined selection also handles
  a compile database generated with either native switch setting.

Changing PlatformIO project configuration triggered a full rebuild of the
combined project and its libraries. The final image, rather than source/object
size estimates, determines the actual saving.

## Validation

Build 11 passed in 1234.77 seconds (full rebuild): 6,019,439 application bytes
and 159,764 static internal RAM bytes. Against Build 10 this saves 135,160
application bytes (132 KiB / 2.20%) and 1,024 bytes of static RAM.
`combined-build11-flash.json` shows 10,012 mapped bytes in the replacement
command object (including retained validators) and only 40 bytes in debughlp,
compared with the previous 66,113 and 51,748 bytes. The compiled image retains
the explicit disabled-command error and no longer contains the desktop help
text. Other debugger classes remain.

Flash 07 verified all written segments. The actual-device regression
`bauerino-device-09-no-debug-commands --no-captures` passed both real MM IV
e7e5 and MM V d2d4 moves, clean result-0 quits, intact heaps and return to
the original frontend/Virginia. MM V reported 15.91% average speed, consistent
with the prior approximately 16%; this is a size improvement, not a demonstrated
speed improvement. Exact images, source delta, map and evidence are preserved
under `backups/takeover-20261110/combined-build11-no-debug-commands`.
The filesystem/ROM bytes remain those of the accepted
baseline; no assets or languages have been removed. Full desktop debugger
commands require rebuilding with this profile disabled.
