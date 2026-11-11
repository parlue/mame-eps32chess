# ---------------------------------------------------------------------------
# Baubericht verdichten: Fehler, gescheiterte Ziele und die vier Dateien,
# die in den Laeufen #17-#19 gescheitert waren.
#
# Hintergrund: raw_err.txt ist extrem lang (viele Warnungen, u. a. die
# erwartete Meldung "\"__INT32_TYPE__\" redefined" aus den Typ-Schaltern der
# Komponente). Die Ausgabe der Entwicklungsumgebung ist bei langen Laeufen
# unzuverlaessig, daher schreibt dieses Skript einen kurzen Bericht in
# docs/_check.txt.
#
# Aufruf: powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_build.ps1
# ---------------------------------------------------------------------------

$root = Split-Path -Parent $PSScriptRoot
$out  = Join-Path $root 'docs\raw_out.txt'
$err  = Join-Path $root 'docs\raw_err.txt'
$log  = Join-Path $root 'docs\_check.txt'
$bld  = Join-Path $root 'esp32s3\.pio\build\esp32s3-skeleton'

$report = New-Object System.Collections.Generic.List[string]
$report.Add("=== Bericht $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') ===")

# 1) harte Fehler in beiden Stromen
foreach ($pair in @(@('stdout', $out), @('stderr', $err))) {
    $name = $pair[0]; $file = $pair[1]
    if (-not (Test-Path -LiteralPath $file)) { $report.Add("$name: fehlt"); continue }
    $errors = @(Select-String -LiteralPath $file -Pattern ': error:' -ErrorAction SilentlyContinue)
    $failed = @(Select-String -LiteralPath $file -Pattern 'FAILED:' -ErrorAction SilentlyContinue)
    $report.Add("$name: ': error:'=$($errors.Count)  'FAILED:'=$($failed.Count)")
    foreach ($m in ($errors | Select-Object -First 8)) { $report.Add("    E> " + $m.Line.Trim()) }
    foreach ($m in ($failed | Select-Object -First 8)) { $report.Add("    F> " + $m.Line.Trim()) }
}

# 2) die vier kritischen Dateien aus den Laeufen #17-#19
$crit = @('divtlb', 'dipalette', 'inputdev', 'natkeyboard')
$report.Add("--- kritische Dateien (Uebersetzung gestartet?) ---")
foreach ($c in $crit) {
    $hit = @(Select-String -LiteralPath $out -Pattern ("Compiling.*" + $c + "\.cpp") -ErrorAction SilentlyContinue)
    $obj = @(Get-ChildItem -LiteralPath $bld -Recurse -Filter ($c + '.cpp.obj') -ErrorAction SilentlyContinue)
    $report.Add("$c : Compiling-Zeilen=$($hit.Count)  objektdateien=$($obj.Count)")
}

# 3) Gesamtfortschritt MAME-Komponente
$mameObj = @(Get-ChildItem -LiteralPath (Join-Path $bld 'esp-idf\mame0220') -Recurse -Filter '*.obj' -ErrorAction SilentlyContinue)
$mameLib = @(Get-ChildItem -LiteralPath (Join-Path $bld 'esp-idf\mame0220') -Recurse -Filter '*.a' -ErrorAction SilentlyContinue)
$report.Add("mame0220: objektdateien=$($mameObj.Count)")
foreach ($l in $mameLib) { $report.Add("    lib: $($l.Name) $($l.Length) Bytes") }
$report.Add("Archivierungszeilen in stdout: " + @(Select-String -LiteralPath $out -Pattern 'mame0220' -ErrorAction SilentlyContinue).Count)
$report.Add("letzte 3 Zeilen:")
foreach ($l in (Get-Content -LiteralPath $out -Tail 3)) { $report.Add("    | $l") }

$report.Add("")
Add-Content -LiteralPath $log -Value ($report -join "`r`n")
