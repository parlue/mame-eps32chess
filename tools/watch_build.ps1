# ---------------------------------------------------------------------------
# Waechter fuer den entkoppelten PlatformIO-Build (siehe run_pio_build.cmd).
#
# Warum entkoppelt: ein im Vordergrund gestarteter Build stirbt, wenn das
# aufrufende Terminal geschlossen wird (Lauf #20 endete mit Error 3221225786 =
# STATUS_CONTROL_C_EXIT). Deshalb wird der Build per WMI gestartet; dieser
# Waechter schreibt den Fortschritt in docs/_watch.txt, damit der Fortschritt
# ohne Terminalinteraktion geprueft werden kann.
#
# Aufruf:  powershell -NoProfile -ExecutionPolicy Bypass -File tools\watch_build.ps1
# Abbruch: sobald raw_out.txt die Abschlusszeile "exit=" enthaelt oder die
#          Laufzeit abgelaufen ist.
# ---------------------------------------------------------------------------
param([int]$Minutes = 15)

$root = Split-Path -Parent $PSScriptRoot          # ...\mame-s3
$out  = Join-Path $root 'docs\raw_out.txt'
$err  = Join-Path $root 'docs\raw_err.txt'
$log  = Join-Path $root 'docs\_watch.txt'

Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue

for ($i = 0; $i -lt ($Minutes * 3); $i++) {
    Start-Sleep -Seconds 20
    $tail = ''
    if (Test-Path -LiteralPath $out) {
        $tail = ((Get-Content -LiteralPath $out -Tail 3 -ErrorAction SilentlyContinue) -join ' | ')
    }
    $errBytes = if (Test-Path -LiteralPath $err) { (Get-Item -LiteralPath $err).Length } else { -1 }
    $pio = @(Get-CimInstance Win32_Process -Filter "Name='platformio.exe'").Count
    Add-Content -LiteralPath $log -Value ("t={0,4}s pio={1} errBytes={2} :: {3}" -f ($i * 20), $pio, $errBytes, $tail)
    if ($tail -match 'exit=') { break }
}
