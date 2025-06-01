param(
    [ValidateSet("build", "clean", "tests")]
    [string]$Action = "build",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",

    [string]$TestName = ""
)

# Create build directory
$buildDir = "build"
if ($Action -eq "clean") {
    if (Test-Path $buildDir) {
        Remove-Item -Path $buildDir -Recurse -Force
        Write-Host "Build directory cleaned." -ForegroundColor Green
    }
    if (Test-Path "debug_env_startup_commands.txt") {
        Remove-Item "debug_env_startup_commands.txt"
    }
    exit 0
}

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Configure with CMake
Write-Host "Configuring with CMake..." -ForegroundColor Cyan
cmake -S . -B $buildDir -G "Visual Studio 17 2022" -A x64

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build
if ($Action -eq "build" -or $Action -eq "tests") {
    Write-Host "Building project..." -ForegroundColor Cyan
    cmake --build $buildDir --config $Config

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
}

# Run tests
if ($Action -eq "tests") {
    Write-Host "Running tests..." -ForegroundColor Cyan
    Push-Location $buildDir
    if ($TestName) {
        ctest -C $Config -R $TestName -V
    } else {
        ctest -C $Config -V
    }
    Pop-Location

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Tests failed!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Done!" -ForegroundColor Green
