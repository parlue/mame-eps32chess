# Schnelltest: prueft zwei xtensa-Typfallen ausserhalb des PlatformIO-Builds.
#
#   powershell -File mame-s3\tools\type_probe.ps1
#
# Drei Varianten in einem Lauf (Ausgabe nach mame-s3\docs\_probe.txt):
#   1) unveraendert                     -> erwartet beide Fehler
#   2) -include mame_s3_xtensa_types.h  -> save_item(int) ok, std::min(size_t,u32) Fehler
#   3) -U__INT32_TYPE__ -D__INT32_TYPE__=int -> erwartet beide ok
#
# Variante 3 ist die interessante: sie stellt den Typzustand her, den MAME
# voraussetzt (int32_t == int). Sie wirkt nur auf die MAME-Komponente, nicht auf
# das uebrige ESP-IDF.

$gcc  = "$env:USERPROFILE\.platformio\packages\toolchain-xtensa-esp-elf\bin\xtensa-esp-elf-gcc.exe"
$root = (Resolve-Path "$PSScriptRoot\..").Path
$src  = "$root\ref\mame0220-src"
$hdr  = "$root\esp32s3\components\mame0220\include\mame_s3_xtensa_types.h"
$out  = "$env:TEMP\xtensa_type_probe.o"
$log  = "$root\docs\_probe.txt"

'=== xtensa-Typprobe (drei Varianten) ===' | Set-Content $log

$base = @(
    '-c', '-std=gnu++17', '-fexceptions', '-frtti', '-fsigned-char', '-w',
    '-DMAME_NOASM=1', '-DPTR64=0', '-DLSB_FIRST=1',
    "-I$src\src\emu", "-I$src\src\lib", "-I$src\src\lib\util", "-I$src\src\osd",
    "-I$src\3rdparty", "-I$src\3rdparty\utf8proc", "-I$src\3rdparty\zlib",
    '-o', $out, "$PSScriptRoot\xtensa_type_probe.cpp"
)

$varianten = @(
    @{ Name = '1) unveraendert';                             Extra = @() },
    @{ Name = '2) -include mame_s3_xtensa_types.h';          Extra = @('-include', $hdr) },
    @{ Name = '3) -U__INT32_TYPE__ -D__INT32_TYPE__=int';    Extra = @('-U__INT32_TYPE__', '-D__INT32_TYPE__=int') }
)

foreach ($v in $varianten) {
    "--- $($v.Name) ---" | Add-Content $log
    (& $gcc @($v.Extra + $base) 2>&1 |
        Select-String -Pattern 'error|is_atom|static assertion|deduced conflicting' |
        Select-Object -First 10 | ForEach-Object { $_.Line }) | Add-Content $log
    "exit=$LASTEXITCODE" | Add-Content $log
    Write-Output "$($v.Name): exit=$LASTEXITCODE"
}
