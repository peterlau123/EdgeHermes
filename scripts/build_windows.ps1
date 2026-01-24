Param(
  [ValidateSet('Release','Debug')]
  [string]$Configuration = 'Release',
  [ValidateSet('ON','OFF')]
  [string]$EnableLogging = 'ON',
  [switch]$WithTests,
  [string]$InstallPrefix
)

# build_windows.ps1 â€?Build EdgeHermes on Windows using Conan + CMake (MSVC)
# Mirrors .github/workflows/windows.yml

function Need($cmd) {
  if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) {
    Write-Error "'$cmd' is required"; exit 1
  }
}

Need python
Need cmake
Need conan

conan profile detect --force

$buildSuffix = $Configuration.ToLower()
$BUILD_DIR = "build-$buildSuffix"
$INSTALL_DIR = if ($InstallPrefix) { $InstallPrefix } else { "install-$buildSuffix" }

New-Item -ItemType Directory -Force -Path $BUILD_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $INSTALL_DIR | Out-Null

Push-Location $BUILD_DIR

$ConanBuildTests = if ($WithTests) { 'True' } else { 'False' }
conan install .. --output-folder=. --build=missing -s build_type=$Configuration -o build_tests=$ConanBuildTests

$toolchain = Get-ChildItem -Recurse -Filter conan_toolchain.cmake | Select-Object -First 1
if (-not $toolchain) { Write-Error 'conan_toolchain.cmake not found'; exit 1 }
Write-Host "Using toolchain: $($toolchain.FullName)"

$generator = 'Visual Studio 17 2022'
cmake -S .. -B . -G "$generator" -A x64 `
  -DCMAKE_BUILD_TYPE=$Configuration `
  -Dedgehermes_ENABLE_LOGGING=$EnableLogging `
  -DCMAKE_INSTALL_PREFIX="$(Resolve-Path ..\$INSTALL_DIR)" `
  -DCMAKE_TOOLCHAIN_FILE="$($toolchain.FullName)"

cmake --build . --config $Configuration
cmake --install . --config $Configuration

Pop-Location

Write-Host "Build complete (type=$Configuration, logging=$EnableLogging). Artifacts installed to '$INSTALL_DIR'"

if ($WithTests) {
  Write-Host 'Building tests...'
  $TEST_BUILD_DIR = "build-test-$buildSuffix"
  New-Item -ItemType Directory -Force -Path $TEST_BUILD_DIR | Out-Null
  Push-Location $TEST_BUILD_DIR
  conan install ../test --output-folder=conan --build=missing -s build_type=$Configuration
  $toolchain = Get-ChildItem -Recurse -Filter conan_toolchain.cmake | Select-Object -First 1
  if (-not $toolchain) { Write-Error 'conan_toolchain.cmake (tests) not found'; exit 1 }
  cmake -S ../test -B . -G "$generator" -A x64 -DCMAKE_BUILD_TYPE=$Configuration -DCMAKE_TOOLCHAIN_FILE="$($toolchain.FullName)"
  cmake --build . --config $Configuration
  # Note: ctest optional
  Pop-Location
}




