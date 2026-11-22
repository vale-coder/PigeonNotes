$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$slintDir = 'C:\Program Files\Slint-cpp 1.19.0\lib\cmake\Slint'
if (-not (Test-Path (Join-Path $slintDir 'SlintConfig.cmake'))) { throw 'Slint 1.19 SDK not found.' }
if (-not (Test-Path 'fonts\Manrope.ttf')) {
    New-Item -ItemType Directory -Force fonts | Out-Null
    Write-Host 'Downloading Manrope (OFL)...'
    Invoke-WebRequest 'https://raw.githubusercontent.com/google/fonts/main/ofl/manrope/Manrope%5Bwght%5D.ttf' -OutFile 'fonts\Manrope.ttf' -UseBasicParsing
    Invoke-WebRequest 'https://raw.githubusercontent.com/google/fonts/main/ofl/manrope/OFL.txt' -OutFile 'fonts\OFL.txt' -UseBasicParsing
}
cmake -S . -B build -G 'Visual Studio 17 2022' -A x64 "-DSlint_DIR=$slintDir"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
cmake --build build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed. Do not launch the previous exe.' }
Write-Host 'PIGEON NOTES BUILD SUCCESSFUL' -ForegroundColor Green
& '.\build\Release\PigeonNotes.exe'
