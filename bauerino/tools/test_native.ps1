$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$taskBuild=Join-Path $taskRoot 'build/native'
New-Item -ItemType Directory -Path $taskBuild -Force | Out-Null
$taskVc='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$taskLibs=@('types','bitboard_compact','position','movegen','weights','eval','botdata','tt','search','adaptive','uci')
$taskFlags='/nologo /std:c++17 /DESP_PLATFORM=1 /I"C:\Users\DirkSommerfeld\Documents\PlatformIO\Projects\ChessL1nkwireless\mame-s3\bauerino\tests\host_caps" /DVIRGIN_EMBEDDED=1 /DVIRGIN_CORE2=1 /DVIRGIN_EMBEDDED_HASH_KIB=512 /DVIRGIN_EMBEDDED_MAX_PLY=20 /EHsc /utf-8 /O2 /I"' + (Join-Path $taskRoot 'lib/chess/src') + '" /I"' + (Join-Path $taskRoot 'lib/control/src') + '" '
Push-Location $taskBuild
try {
 $taskSources=$taskLibs | ForEach-Object {'"'+(Join-Path $taskRoot ('lib/chess/src/'+$_+'.cpp'))+'"'}
 $taskCommand='call "'+$taskVc+'" >nul && cl '+$taskFlags+'"'+(Join-Path $taskRoot 'tests/native.cpp')+'" '+($taskSources -join ' ')+' /Fe:tests.exe'
 & cmd.exe /d /s /c $taskCommand
 if($LASTEXITCODE -ne 0){throw 'Native compilation failed'}
 & (Join-Path $taskBuild 'tests.exe')
 if($LASTEXITCODE -ne 0){throw 'Rule checks failed'}
 $taskObjects=$taskLibs | ForEach-Object {$_+'.obj'}
 $taskCommand='call "'+$taskVc+'" >nul && cl '+$taskFlags+'"'+(Join-Path $taskRoot 'tests/virgin_uci.cpp')+'" '+($taskObjects -join ' ')+' /Fe:virgin_uci.exe'
 & cmd.exe /d /s /c $taskCommand
 if($LASTEXITCODE -ne 0){throw 'Virgin UCI compilation failed'}
 & (Join-Path $taskBuild 'virgin_uci.exe')
 if($LASTEXITCODE -ne 0){throw 'Virgin UCI checks failed'}
 $taskCommand='call "'+$taskVc+'" >nul && cl /nologo /std:c++17 /EHsc /I"'+(Join-Path $taskRoot 'include')+'" "'+(Join-Path $taskRoot 'tests/bauerino_layout.cpp')+'" /Fe:bauerino_layout.exe'
 & cmd.exe /d /s /c $taskCommand
 if($LASTEXITCODE -ne 0){throw 'Bauerino layout compilation failed'}
 & (Join-Path $taskBuild 'bauerino_layout.exe')
 if($LASTEXITCODE -ne 0){throw 'Bauerino layout checks failed'}
 $taskCommand='call "'+$taskVc+'" >nul && cl /nologo /std:c++17 /EHsc /I"'+(Join-Path $taskRoot 'include')+'" "'+(Join-Path $taskRoot 'tests/bauerino_bots.cpp')+'" /Fe:bauerino_bots.exe'
 & cmd.exe /d /s /c $taskCommand
 if($LASTEXITCODE -ne 0){throw 'Bot catalogue compilation failed'}
 & (Join-Path $taskBuild 'bauerino_bots.exe') (Join-Path $taskRoot 'data/bots/catalog.pack')
 if($LASTEXITCODE -ne 0){throw 'Bot catalogue checks failed'}
}finally{Pop-Location}
