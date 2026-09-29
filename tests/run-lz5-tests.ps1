# Run from a Visual Studio developer prompt; exercise the non-LTCG /O1 path
# used by ARM64 as well as an unoptimized control build on every Windows arch.
$ErrorActionPreference = 'Stop'
$dir = Join-Path $env:TEMP ('7zip-lz5-tests-' + [guid]::NewGuid().ToString('N'))
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sources = @('tests/lz5-frame.c', 'C/lz5/lz5.c', 'C/lz5/lz5hc.c',
  'C/lz5/lz5frame.c', 'C/hashes/xxhash.c') | ForEach-Object { Join-Path $root $_ }
[void](New-Item -ItemType Directory -Path $dir)
try {
  foreach ($optimization in @('/O1', '/Od')) {
    & cl /nologo $optimization /Gy /Gw /GS- /MT "/I$root/C/lz5" "/Fo$dir\" "/Fe$dir/lz5-frame.exe" $sources
    if ($LASTEXITCODE -ne 0) { throw "LZ5 compilation failed: $optimization" }
    & "$dir/lz5-frame.exe"
    if ($LASTEXITCODE -ne 0) { throw "LZ5 frame tests failed: $optimization" }
  }
} finally {
  foreach ($source in $sources) {
    $obj = [IO.Path]::GetFileNameWithoutExtension($source) + '.obj'
    Remove-Item -LiteralPath (Join-Path $dir $obj) -ErrorAction SilentlyContinue
  }
  Remove-Item -LiteralPath "$dir/lz5-frame.exe" -ErrorAction SilentlyContinue
  [IO.Directory]::Delete($dir)
}
