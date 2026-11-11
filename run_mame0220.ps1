# Starter fuer die Referenzfassung MAME 0.220 (Windows).
#
# Prueft zuerst die Voraussetzungen und sagt klar, was fehlt. Es werden keine
# Optionen geraten: verwendet werden nur dokumentierte MAME-Schalter
# (-rompath, -skip_gameinfo, -video, -plugin, -listroms, -version).
#
#   .\run_mame0220.ps1 -Action check      # Zustand melden (Standard)
#   .\run_mame0220.ps1 -Action version    # 0.220-Version ausgeben
#   .\run_mame0220.ps1 -Action listroms -Set mm4
#   .\run_mame0220.ps1 -Action run -Set mm4 -Video auto
#
[CmdletBinding()]
param(
    [ValidateSet('check', 'version', 'listroms', 'run')] [string]$Action = 'check',
    [string]$Set = 'mm4',
    [string]$Plugin = 'chessengine',
    [string]$Video = 'auto'
)

$ErrorActionPreference = 'Stop'
$bin = Join-Path $PSScriptRoot 'ref\mame0220-win\bin'
$exe = Join-Path $bin 'mame64.exe'
$roms = Join-Path $bin 'roms'
$plug = Join-Path $bin "plugins\$Plugin"

function Fail($msg) { Write-Host "FEHLT: $msg" -ForegroundColor Red }

if (-not (Test-Path $exe)) {
    Fail "mame64.exe nicht vorhanden: $exe"
    Write-Host "  -> python mame-s3\tools\get_0220.py   (laedt und prueft das 0.220-Release-Asset)"
    exit 2
}

switch ($Action) {
    'version'  { & $exe -version; exit $LASTEXITCODE }
    'listroms' { & $exe -listroms $Set; exit $LASTEXITCODE }
    'check' {
        Write-Host "MAME 0.220 Referenzfassung" -ForegroundColor Cyan
        Write-Host ("  exe    : {0}" -f $exe)
        Write-Host ("  version: {0}" -f (& $exe -version))
        if (Test-Path $plug) {
            $ifaces = @(Get-ChildItem (Join-Path $plug 'interfaces') -Filter '*.lua' -ErrorAction SilentlyContinue)
            Write-Host ("  plugin : OK ({0} Interface-Dateien)" -f $ifaces.Count) -ForegroundColor Green
        } else {
            Fail  "Plugin '$Plugin'  ->  $plug"
            Write-Host "  (aus dem CB-Emu-Paket hierher kopieren; Rechte beachten)"
        }
        $rompath = Join-Path $roms $Set
        $zip = Join-Path $roms "$Set.zip"
        if (Test-Path $rompath) {
            Write-Host ("  ROMs   : OK ({0})" -f $rompath) -ForegroundColor Green
        } elseif (Test-Path $zip) {
            Write-Host ("  ROMs   : OK ({0})" -f $zip) -ForegroundColor Green
        } else {
            Fail "ROM-Satz '$Set'  ->  $rompath  oder  $zip"
        }
        Write-Host ""
        Write-Host "Pruefung der ROM-Pruefsummen: python mame-s3\tools\inventory.py"
        exit 0
    }
    'run' {
        if (-not (Test-Path $plug)) {
            Fail "Plugin '$Plugin' fehlt - Start abgebrochen (nichts wird vorgetaeuscht)."
            exit 3
        }
        $have = (Test-Path (Join-Path $roms $Set)) -or (Test-Path (Join-Path $roms "$Set.zip"))
        if (-not $have) {
            Fail "ROM-Satz '$Set' fehlt - Start abgebrochen."
            exit 3
        }
        $args = @('-rompath', $roms, '-skip_gameinfo', '-video', $Video,
                  '-plugin', $Plugin, $Set)
        Write-Host ("mame64.exe " + ($args -join ' ')) -ForegroundColor Cyan
        & $exe @args
        exit $LASTEXITCODE
    }
}
