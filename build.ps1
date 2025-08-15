param(
    [ValidateSet("build", "clean", "tests")]
    [string]$Action = "build",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",

    [string]$TestName = ""
)

# Set up Visual Studio Developer Environment
function Initialize-VsDevEnvironment {
    # Common Visual Studio installation paths
    $vsPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\Launch-VsDevShell.ps1",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\Launch-VsDevShell.ps1",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\Common7\Tools\Launch-VsDevShell.ps1",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\Common7\Tools\Launch-VsDevShell.ps1",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\Launch-VsDevShell.ps1"
    )

    # Check if we're already in a VS Developer environment
    if ($env:VSINSTALLDIR) {
        Write-Host "Already in Visual Studio Developer environment." -ForegroundColor Green
        return
    }

    # Find and execute the VS Developer shell script
    foreach ($path in $vsPaths) {
        if (Test-Path $path) {
            Write-Host "Setting up Visual Studio Developer environment..." -ForegroundColor Cyan
            & $path -Arch amd64 -HostArch amd64
            if ($LASTEXITCODE -eq 0) {
                Write-Host "Visual Studio Developer environment initialized." -ForegroundColor Green
                return
            }
        }
    }

    Write-Host "Warning: Could not find Visual Studio Developer environment." -ForegroundColor Yellow
    Write-Host "Please ensure Visual Studio 2019/2022 with C++ tools is installed." -ForegroundColor Yellow
}

# Initialize VS Developer environment
Initialize-VsDevEnvironment

# Ensure we're in the correct directory (where this script is located)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($PWD.Path -ne $scriptDir) {
    Write-Host "Changing to script directory: $scriptDir" -ForegroundColor Cyan
    Set-Location $scriptDir
}

# Verify CMakeLists.txt exists
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "Error: CMakeLists.txt not found in current directory." -ForegroundColor Red
    Write-Host "Please ensure you're running this script from the WinDbg extensions project root." -ForegroundColor Red
    exit 1
}

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
