# ---------------------------------------------------------------------------
# Dauerwaechter fuer den entkoppelten Build (Lauf #22).
#
# Grund: Parallel laeuft ein zweiter PlatformIO-Build (output/espmame-prep),
# beide mit --jobs 12. Der eigene Lauf wird dadurch stark ausgebremst; die
# Konfigurationsphase kann viele Minuten dauern. Dieses Skript schreibt
# deshalb im Abstand von 30 s eine kompakte Fortschrittszeile nach
# docs/_watchloop.txt und endet, sobald raw_out.txt die Zeile "exit=" enthaelt.
#
# Aufruf: powershell -NoProfile -ExecutionPolicy Bypass -File tools\watch_loop.ps1
# ---------------------------------------------------------------------------
param([int]$Minutes = 45)

$root = Split-Path -Parent $PSScriptRoot
$out  = Join-Path $root 'docs\raw_out.txt'
$err  = Join-Path $root 'docs\raw_err.txt'
$log  = Join-Path $root 'docs\_watchloop.txt'
$lay  = Join-Path $root 'esp32s3\components\mame0220\generated\layout'

Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue

for ($i = 0; $i -lt ($Minutes * 2); $i++) {
    $lines = 0; $tail = ''
    if (Test-Path -LiteralPath $out) {
        $lines = @(Get-Content -LiteralPath $out).Count
        $tail = ((Get-Content -LiteralPath $out -Tail 1 -ErrorAction SilentlyContinue) -join ' ')
    }
    $errSz  = if (Test-Path -LiteralPath $err) { (Get-Item -LiteralPath $err).Length } else { -1 }
    $pio    = @(Get-CimInstance Win32_Process -Filter "Name='platformio.exe'").Count
    $gpp    = @(Get-CimInstance Win32_Process -Filter "Name='xtensa-esp32s3-elf-g++.exe'").Count
    $lh     = @(Get-ChildItem -LiteralPath $lay -Filter '*.lh' -ErrorAction SilentlyContinue).Count
    $failed = 0
    if (Test-Path -LiteralPath $err) {
        $failed = @(Select-String -LiteralPath $err -Pattern 'FAILED:' -ErrorAction SilentlyContinue).Count
    }
    $errs = 0
    if (Test-Path -LiteralPath $err) {
        $errs = @(Select-String -LiteralPath $err -Pattern ': error:' -ErrorAction SilentlyContinue).Count
    }

    Add-Content -LiteralPath $log -Value ("[{0}] zeilen={1} pio={2} gpp={3} lh={4} errKB={5} fehler={6} FAILED={7} :: {8}" -f `
        (Get-Date).ToString('HH:mm:ss'), $lines, $pio, $gpp, $lh, [math]::Round($errSz/1024,1), $errs, $failed, $tail)

    if ($tail -match 'exit=') { break }
    Start-Sleep -Seconds 30
}
