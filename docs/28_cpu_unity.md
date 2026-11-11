# CPU-only unity compiler experiment

Reference: tested Build 24, commit `ebfcd1875055ea88391c297ee4ebfc4a39a95bf5`.
Its qualified RAM/ROM pages reached MM IV 21.40-21.48% and MM V 23.27-23.47%
original speed. The practical 50% target remains unmet.

`prepare_cpu_unity.py` generates a wrapper including the unchanged owning
6502/profile source followed by unchanged upstream `m65c02.cpp`. This gives
the compiler both implementations together, allowing cross-source inlining
without whole-program LTO. CPU source/header bodies, page adapter, clocks,
interrupts, UCI options and scheduler are not edited for this experiment.

The combined builder asserts that both original manifest entries have
identical include/define profiles before selecting the wrapper and excluding
the old CMOS object. The native project remains on its separate CPU files;
unity selection is a combined-builder experiment only. Combined option
`custom_mame_cpu_unity` defaults to `yes`; `no` restores the prior compilation
grouping. Both choices keep CPU O2/jump tables and `-fno-merge-constants`,
signed char, exceptions, RTTI and the common Arduino/IDF SDK.

Build 25 succeeded in 200.96 seconds, with 4,998,859 application bytes and
154,612 static RAM bytes. The compilation count falls from 298 to 297 sources
with the same seven option profiles. Archive inspection verifies exactly one
unity CPU object and no stale separate 6502/65C02 objects. Its producer record
confirms the intended flags; both CPU classes are present. CPU and page-source
hashes match the Build-24 inputs. Assets and the 28 KiB stack must also be
validated by the pre-flash checkpoint.

Two complete device regressions passed after guarded Flash 17. MM IV reached
22.93% and 22.90% original speed; MM V reached 24.57% and 24.55%. This is
approximately 5-7% faster than Build 24, with unchanged emulated clocks.
Both runs validated page reads/writes/invalidation, real ROM moves, clean
result-0 exits, heap integrity, launcher gestures, Virginia and all 215 Bots.
Minimum reported worker stack reserve was 7,356 bytes. These short opening
tests are not a complete search benchmark or a fresh physical touch/web test.

The exact private rollback is `combined-build25-cpu-unity`, including the
firmware and its source inputs. Build 24 remains the previous published
rollback. Neither ROMs nor binary backups are distributed in the repository.
The practical 50% original-speed target remains unmet.
