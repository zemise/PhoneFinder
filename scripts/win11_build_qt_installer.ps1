param(
  [string]$QtCMakePrefix = "",
  [string]$Generator = "Visual Studio 17 2022",
  [string]$Arch = "x64"
)

$ErrorActionPreference = "Stop"

function Require-Command($name) {
  if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
    throw "Missing required command: $name"
  }
}

function Resolve-ProjectRoot {
  $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  return (Resolve-Path (Join-Path $scriptDir "..")).Path
}

Write-Host "==> Checking required tools..."
Require-Command cmake
Require-Command makensis
Require-Command windeployqt

$root = Resolve-ProjectRoot
$buildDir = Join-Path $root "build-win-qt"
$stageDir = Join-Path $buildDir "stage"
$distDir = Join-Path $root "dist"
$nsiFile = Join-Path $root "packaging\qt_installer.nsi"
$outFile = Join-Path $distDir "PhoneFinder-installer.exe"
$exePath = Join-Path $buildDir "Release\PhoneFinder.exe"
$sampleCsv = Join-Path $root "samples\医院科室电话数据模板_demo.csv"
$repoRoot = (Resolve-Path (Join-Path $root "..")).Path
$appIco = Join-Path $repoRoot "build\windows\icon.ico"
$trayPng = Join-Path $repoRoot "build\trayicon.png"

if (-not (Test-Path $distDir)) {
  New-Item -ItemType Directory -Path $distDir | Out-Null
}

if (Test-Path $buildDir) {
  Write-Host "==> Reusing build dir: $buildDir"
} else {
  New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Write-Host "==> Configuring CMake..."
$cmakeArgs = @(
  "-S", $root,
  "-B", $buildDir,
  "-G", $Generator,
  "-A", $Arch
)
if ($QtCMakePrefix -ne "") {
  $cmakeArgs += @("-DCMAKE_PREFIX_PATH=$QtCMakePrefix")
}
& cmake @cmakeArgs

Write-Host "==> Building PhoneFinder..."
& cmake --build $buildDir --config Release --target phonefinder_qt

if (-not (Test-Path $exePath)) {
  throw "Build completed but executable not found: $exePath"
}

Write-Host "==> Preparing stage directory..."
if (Test-Path $stageDir) {
  Remove-Item -Recurse -Force $stageDir
}
New-Item -ItemType Directory -Path $stageDir | Out-Null
Copy-Item $exePath (Join-Path $stageDir "PhoneFinder.exe") -Force
if (Test-Path $sampleCsv) {
  Copy-Item $sampleCsv (Join-Path $stageDir "医院科室电话数据模板_demo.csv") -Force
}
if (Test-Path $appIco) {
  Copy-Item $appIco (Join-Path $stageDir "icon.ico") -Force
}
if (Test-Path $trayPng) {
  Copy-Item $trayPng (Join-Path $stageDir "trayicon.png") -Force
}

Write-Host "==> Running windeployqt..."
& windeployqt --release (Join-Path $stageDir "PhoneFinder.exe")

Write-Host "==> Building NSIS installer..."
& makensis "/DAPP_STAGE_DIR=$stageDir" "/DAPP_OUTFILE=$outFile" $nsiFile

Write-Host ""
Write-Host "Installer built successfully:"
Write-Host "  $outFile"
