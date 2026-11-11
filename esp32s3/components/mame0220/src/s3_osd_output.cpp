// Keep MAME's original output routing, while the ESP platform supplies its
// monotonic timer, FreeRTOS sleep and command-line entry points. Rename only
// the original host implementations in this translation unit. Upstream stays
// unchanged and unused host functions are discarded by the linker.
#define osd_ticks mame_host_osd_ticks
#define osd_ticks_per_second mame_host_osd_ticks_per_second
#define osd_sleep mame_host_osd_sleep
#define osd_get_command_line mame_host_osd_get_command_line
#include "../../../../ref/mame0220-src/src/osd/osdcore.cpp"
