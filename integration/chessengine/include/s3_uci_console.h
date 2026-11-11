#pragma once
class running_machine;
// Call from the emulation task after machine start, then once per frame.
// USB stdin is non-blocking; no second task accesses MAME or the Lua VM.
bool mame_s3_uci_open(running_machine &machine);
void mame_s3_uci_poll();
// Close before running_machine is destroyed, including error/quit paths.
void mame_s3_uci_close();
