# Sustained middlegame measurement

The next performance reference uses two reproducible 32-ply histories, ending
before White's move 17: a closed Spanish centre and a Sicilian Dragon with
opposite castling. `tools/retro_benchmark_positions.json` records the moves.
They are test lines, not claims of best play or standard published positions.

`retro_position_check.cpp` uses the existing product move generator to verify
every move, produce the resulting FEN and list legal replies. The FEN is
evidence only: the ROM receives `position startpos moves ...`, preserving the
history through the existing MEM/edit-game and sensorboard/button interface.
The limited ESP FEN setup remains disabled.

## Host verification

Both complete histories passed on both actual ROMs using official PC MAME
0.220, the ported cooperative Lua bridge and level `le 1`. All replay-complete
and readiness markers were observed; every ROM reply was legal in the expected
position:

| Position | MM IV reply | MM V reply |
|---|---|---|
| Spanish centre | d4e5 | e3d2 |
| Dragon attack | d2e2 | d2e2 |

This is protocol/replay evidence, not ESP speed or direct inspection of the
ROM's internal board RAM. The source of the move validator and the histories
are included; ROMs and generated host binaries remain private.

## ESP transport and measurement

The combined frontend adds a bounded USB-only `retro-uci <command>` diagnostic.
It takes over a completely handshaken idle session and feeds the same existing
eight-entry UCI mailbox consumed by the MAME worker. VM/device operations stay
on that worker. Position/option/new-game commands are rejected while the
frontend is thinking; quit clears queued work. Input overflow discards the
whole serial line rather than accepting its tail. Normal touch operation is
unchanged until a diagnostic session is explicitly requested.
The test waits for the startup animation and sends 32-byte USB portions with
50 ms spacing, respecting the Arduino CDC receiver's default 256-byte ring.

`benchmark_retro_middlegame.py` resets the identified Waveshare, loads a ROM,
sets Speed 0 (unthrottled) and the chosen original device level, then replays
the history. It waits for the `isready` queued after that history to receive
`readyok`. The cooperative command coroutine cannot answer it before replay
and the final ENT wait complete. ESP Runtime deliberately suppresses Log /
popmessage; the PC host's visible edit-complete message is not an ESP marker.
Multi-move replay already starts the ROM using ENT; the following `go` is
idempotent and marks the frontend as thinking. Only rate intervals ending at
least ten wall seconds after `go` are included, excluding replay overlap.
Speed is the sum of emulated interval seconds divided by the sum of
wall interval seconds, multiplied by 100. Bestmove ends the measurement early
and is checked against the legal reply list; fewer than eight usable intervals
fails the sustained measurement. Quit must finish with result 0 and intact
heaps. Error cleanup uses the independent Home/stop path.

Original ROM clocks, timers and instruction logic are unchanged. The chosen
device level uses emulated time; `--seconds 180` means 180 wall seconds.
`go movetime` is deliberately not used, because this plugin ignores it.
CPU/profile windows report wall time including preemption, not sampled CPU
cycles. Search phase is inferred from replay/start/no-bestmove events; no ROM
depth, PV or internal search-state readback is claimed.

```powershell
powershell -File tools/build_retro_position_check.ps1
python tools/test_retro_middlegame_host.py --output <absolute-new-directory>
python tools/benchmark_retro_middlegame.py --driver mm4 --case spanish-centre --seconds 180 --output <new-directory>
python tools/benchmark_retro_middlegame.py --driver mm5 --case dragon-attack --seconds 180 --output <new-directory>
```

## Next measured candidate

The model-specific interface translates the ROM's emulated electronics into
selection events; the shared bridge translates those events into UCI moves.
This separation must remain. Current MM IV/MM V `is_selected(x,y)` constructs
two digit-code tables and reads four display outputs for every square.
`search_selected_piece()` invokes it for all 64 squares: 128 table creations
and 256 repeated digit reads per full scan, in addition to LED queries.

A candidate is to reuse the immutable digit tables and snapshot the four
display values once per scan, with any preparation hook confined to the model
interface. These functions do not yield or advance the emulated CPU within
the scan. Any cached readings must refresh on every new scan, preserving LED
transitions and the existing device-signal recognition. This is a proposed
optimization, not an implemented or measured gain in Build 28.

## Build 28 results

Build 28 succeeded in 337.05 seconds: 5,003,879 application bytes and 154,628
static internal RAM bytes. Guarded Flash 20 verified every segment. Source
audit confirms that CPU, page adapter, unity wrapper and clock/profile source
are byte-identical to Build 27. This build adds measurement transport, not a
new CPU optimization.

The frontend regression passed Virginia, all 215 Bot decodes, launcher/swipes,
UCI options, Board/Notation, actual opening replies from both ROMs, memory and
SYNC probes, two result-0 exits with intact heaps and return to Virginia.
Host UI tests also pass diagnostic takeover/rejection and mailbox handling.

| ROM / position | Search wall time | Included rate windows | Included wall time | Original speed |
|---|---:|---:|---:|---:|
| MM IV / Spanish centre | 180.047 s | 34 | 171.606 s | 19.79% |
| MM V / Dragon attack | 180.375 s | 34 | 171.444 s | 20.67% |

Both use original device level `le 9`, with no bestmove completed within the
window, and both quit with result 0 and intact heaps. The completed, legal
middlegame replies listed earlier are PC ROM verification; no completed
middlegame move on the ESP is claimed by these fixed-duration runs. Each
ROM/position has one complete sustained run, not a repeated search suite.
Replay time is excluded, as are the leading rate intervals. The final MAME
average at exit includes replay and is deliberately not the search result.
The two ROMs use different positions, so this is not a controlled comparison
of their relative search performance.

Median CPU execution wall time was 56.7% / 55.1%; the UCI/Lua polling section
was 25.3% / 26.6%. CPU time is contained within outside-OSD time, not added to
it. These remain wall-time profiles including preemption. They point to the
redundant interface polling described above as the next candidate; no cache
optimization is yet enabled. The 50% target is approximately 2.4-2.5x away for
these workloads, rather than the smaller factor suggested by opening tests.

Minimum measured internal free heap was 26,556 / 26,544 bytes, stack reserve
7,372 / 7,456 bytes, and free PSRAM remained within 2,618,780-2,677,328 /
2,616,472-2,681,556 bytes during included windows. No unbounded reload or
physical finger/phone acceptance is implied.

Two initial harness attempts failed the USB stress-line check during startup
without loading a ROM. A third waited for the ESP-suppressed desktop Log
marker; its tester was stopped and the ROM then exited cleanly through UCI
quit. These are retained as unsuccessful harness attempts, not passed
benchmarks. The corrected runs above validate paced transmission, whole-line
overflow rejection and readiness-based phase separation.

The exact private checkpoint is `combined-build28-middlegame`, with firmware,
matching sources, host/device evidence and unsuccessful harness logs. Tested
Build 27 remains available as rollback. ROMs and binary backups remain private.
