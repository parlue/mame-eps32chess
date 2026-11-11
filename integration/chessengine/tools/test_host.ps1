$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$taskLua=Join-Path $taskRoot '../../ref/mame0220-src/3rdparty/lua/src'
$taskBuild=Join-Path $taskRoot 'build-host'
New-Item -ItemType Directory -Path $taskBuild -Force | Out-Null
$taskVc='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$taskSources=Get-ChildItem -LiteralPath $taskLua -Filter '*.c' | Where-Object BaseName -NotIn @('lua','luac','linit','liolib','loslib','loadlib','ldblib')
$taskChanged=@($taskSources | Where-Object { $taskObj=Join-Path $taskBuild ($_.BaseName+'.obj'); -not (Test-Path -LiteralPath $taskObj) -or (Get-Item -LiteralPath $taskObj -ErrorAction SilentlyContinue).LastWriteTimeUtc -lt $_.LastWriteTimeUtc })
$taskC=if($taskChanged.Count){'cl /nologo /O2 /TC /c /I"'+$taskLua+'" '+(($taskSources | ForEach-Object {'"'+$_.FullName+'"'}) -join ' ')}else{'rem Lua C objects unchanged'}
$taskLuaObjects=($taskSources | ForEach-Object {'"'+$_.BaseName+'.obj"'}) -join ' '
$taskCpp='cl /nologo /std:c++17 /EHsc /utf-8 /O2 /I"'+(Join-Path $taskRoot 'include')+'" /I"'+(Join-Path $taskRoot 'generated')+'" /I"'+$taskLua+'" "'+(Join-Path $taskRoot 'src/retro_runtime.cpp')+'" "'+(Join-Path $taskRoot 'tests/runtime_test.cpp')+'" '+$taskLuaObjects+' /Fe:runtime_test.exe'
$taskScript='@echo off'+[Environment]::NewLine+'call "'+$taskVc+'" >nul'+[Environment]::NewLine+'cd /d "'+$taskBuild+'"'+[Environment]::NewLine+$taskC+[Environment]::NewLine+'if errorlevel 1 exit /b 1'+[Environment]::NewLine+$taskCpp+[Environment]::NewLine+'if errorlevel 1 exit /b 1'+[Environment]::NewLine+'"'+(Join-Path $taskBuild 'runtime_test.exe')+'"'+[Environment]::NewLine
# Rebuild the C++ test objects when repeating a test build.
foreach($taskOld in @('retro_runtime.obj','runtime_test.obj')){ $taskPath=Join-Path $taskBuild $taskOld; if(Test-Path -LiteralPath $taskPath){Remove-Item -LiteralPath $taskPath} }
$taskCmd=Join-Path $taskBuild 'compile.cmd';Set-Content -LiteralPath $taskCmd -Value $taskScript -Encoding ASCII
& cmd.exe /d /s /c "`"$taskCmd`""
if($LASTEXITCODE -ne 0){throw 'Retro host verification failed'}
