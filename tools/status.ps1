# ---------------------------------------------------------------------------
# Kurzstatus des entkoppelten Builds in docs/_status.txt schreiben.
#
# Zweck: Die Terminal-Ausgabeerfassung der Entwicklungsumgebung ist bei
# laengeren Laeufen unzuverlaessig (siehe docs/09). Dieses Skript schreibt
# den Status deshalb in eine Datei, die anschliessend mit einem Dateileser
# geprueft wird.
#
# Aufruf: powershell -NoProfile -ExecutionPolicy Bypass -File tools\status.ps1
# ---------------------------------------------------------------------------

$root = Split-Path -Parent $PSScriptRoot          # ...\mame-s3
$out  = Join-Path $root 'docs\raw_out.txt'
$err  = Join-Path $root 'docs\raw_err.txt'
$log  = Join-Path $root 'docs\_status.txt'

$lines = 0
$ageMin = -1
$tail = @()
if (Test-Path -LiteralPath $out) {
    $f = Get-Item -LiteralPath $out
    $lines = @(Get-Content -LiteralPath $out).Count
    $ageMin = [math]::Round(((Get-Date) - $f.CreationTime).TotalMinutes, 1)
    $tail = Get-Content -LiteralPath $out -Tail 4
}

$pio    = @(Get-CimInstance Win32_Process -Filter "Name='platformio.exe'").Count
$ccGpp  = @(Get-CimInstance Win32_Process -Filter "Name='xtensa-esp32s3-elf-g++.exe'").Count
$ccGcc  = @(Get-CimInstance Win32_Process -Filter "Name='xtensa-esp32s3-elf-gcc.exe'").Count
$ninja  = @(Get-CimInstance Win32_Process -Filter "Name='ninja.exe'").Count
$errSz  = if (Test-Path -LiteralPath $err) { (Get-Item -LiteralPath $err).Length } else { -1 }
$failed = if (Test-Path -LiteralPath $err) { @(Select-String -LiteralPath $err -Pattern 'FAILED:' -ErrorAction SilentlyContinue).Count } else { 0 }
$done   = @($tail | Where-Object { $_ -match 'exit=' }).Count

$line = "[{0}] laufzeitMin={1} zeilen={2} pio={3} ninja={4} gcc={5} gpp={6} errBytes={7} FAILED={8} fertig={9}" -f `
    (Get-Date).ToString('HH:mm:ss'), $ageMin, $lines, $pio, $ninja, $ccGcc, $ccGpp, $errSz, $failed, $done

# Belastbare Zeit- und Fortschrittsdaten: der Zeitstempel von CMakeCache.txt
# entsteht bei jeder Neukonfiguration, die Zahl der .obj-Dateien zeigt, wie weit
# die MAME-Komponente ist, und firmware.bin markiert das Ende des Gesamtbaus.
$bld = Join-Path $root 'esp32s3\.pio\build\esp32s3-skeleton'
$facts = @()
$cache = Join-Path $bld 'CMakeCache.txt'
if (Test-Path -LiteralPath $cache) {
    $c = Get-Item -LiteralPath $cache
    $facts += "neukonfiguration=$($c.LastWriteTime.ToString('HH:mm:ss')) vorMin=$([math]::Round(((Get-Date)-$c.LastWriteTime).TotalMinutes,1))"
}
$mameDir = Join-Path $bld 'esp-idf\mame0220'
if (Test-Path -LiteralPath $mameDir) {
    $objs = @(Get-ChildItem -LiteralPath $mameDir -Recurse -Filter '*.obj' -ErrorAction SilentlyContinue)
    $libs = @(Get-ChildItem -LiteralPath $mameDir -Recurse -Filter '*.a' -ErrorAction SilentlyContinue)
    $facts += "mameObj=$($objs.Count) mameLib=$($libs.Count)"
}
$bin = Join-Path $bld 'firmware.bin'
if (Test-Path -LiteralPath $bin) {
    $b = Get-Item -LiteralPath $bin
    $facts += "firmware.bin=$($b.Length) B um $($b.LastWriteTime.ToString('HH:mm:ss'))"
}
$allFw = Get-ChildItem -LiteralPath $bld -Filter 'firmware.elf' -ErrorAction SilentlyContinue
if ($allFw) { $facts += "firmware.elf=$($allFw.Length) B um $($allFw.LastWriteTime.ToString('HH:mm:ss'))" }

$block = @($line) + @(($facts -join '  ')) + ($tail | ForEach-Object { "    | $_" })
Add-Content -LiteralPath $log -Value ($block -join "`r`n")
