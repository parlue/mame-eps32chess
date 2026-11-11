$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$taskBuild=Join-Path $taskRoot 'integration/chessengine/build-host/position-check'
$taskChess=Join-Path $taskRoot 'bauerino/lib/chess/src'
$taskVc='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
New-Item -ItemType Directory -Path $taskBuild -Force | Out-Null
$taskSources=@((Join-Path $PSScriptRoot 'retro_position_check.cpp'))+@('position.cpp','movegen.cpp','bitboard_compact.cpp','types.cpp','weights.cpp','eval.cpp' | ForEach-Object {Join-Path $taskChess $_})
$taskCompile='cl /nologo /std:c++17 /EHsc /O2 /DVIRGIN_EMBEDDED /constexpr:steps10000000 /I"'+$taskChess+'" '+(($taskSources | ForEach-Object {'"'+$_+'"'}) -join ' ')+' /Fe:retro_position_check.exe'
$taskScript='@echo off'+[Environment]::NewLine+'call "'+$taskVc+'" >nul'+[Environment]::NewLine+'cd /d "'+$taskBuild+'"'+[Environment]::NewLine+$taskCompile+[Environment]::NewLine+'exit /b %errorlevel%'+[Environment]::NewLine
$taskCmd=Join-Path $taskBuild 'compile.cmd'
Set-Content -LiteralPath $taskCmd -Value $taskScript -Encoding ASCII
& cmd.exe /d /s /c "`"$taskCmd`""
if($LASTEXITCODE -ne 0){throw 'Position checker build failed'}
