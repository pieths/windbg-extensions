# Native extensions
$extensions = @(
    @{
        Name = "break_commands"
        SourceFiles = @("src\break_commands.cpp", "src\utils.cpp")
    },
    @{
        Name = "breakpoints_history"
        SourceFiles = @("src\breakpoints_history.cpp", "src\breakpoint_list.cpp", "src\utils.cpp")
    },
    @{
        Name = "command_lists"
        SourceFiles = @("src\command_lists.cpp", "src\command_list.cpp", "src\utils.cpp")
    },
    @{
        Name = "command_logger"
        SourceFiles = @("src\command_logger.cpp", "src\utils.cpp")
    },
    @{
        Name = "js_command_wrappers"
        SourceFiles = @("src\js_command_wrappers.cpp", "src\utils.cpp")
    },
    @{
        Name = "mcp_server"
        SourceFiles = @("src\mcp_server.cpp", "src\utils.cpp")
    }
)

# Javascript extensions
$scripts = @(
    "callback_location",
    "continuation_commands",
    "type_signatures"
)

# Commands to execute after all extensions are loaded
$postLoadCommands = @(
    # Show nearby command lists every time the target is suspended for any reason.
    "!AddBreakCommand !ShowNearbyCommandLists",
    "!ListBreakpointsHistory"
)

# Standalone executables
$standaloneExecutables = @(
    @{
        Name = "mcp_stdio_bridge"
        SourceFiles = @("src\mcp_server_stdio_bridge.cpp")
        Libraries = @("ws2_32.lib")
        CompilerFlags = @("/std:c++20", "/EHsc")
    }
)

# Test executables
$tests = @(
    @{
        Name = "break_commands"
        SourceFiles = @("tests\test_break_commands.cpp", "src\utils.cpp", "src\break_commands.cpp")
        Libraries = @("dbgeng.lib")
        CompilerFlags = @("/std:c++20", "/EHsc", "/Zi", "/Od", "/MDd")
    }
)

################################################################################
# Internal functionality beyond this point
################################################################################

# If "clean" is passed as an argument, clean the build output directory
if ($args.Count -gt 0 -and $args[0] -eq "clean") {
    Write-Host "Cleaning build output directory..."
    if (Test-Path -Path ".\build_output") {
        Remove-Item -Path ".\build_output" -Recurse -Force
        Write-Host "Build output directory cleaned." -ForegroundColor Green
    } else {
        Write-Host "No build output directory found to clean." -ForegroundColor Yellow
    }
    exit 0
}

# Setup the Visual Studio environment
. "$PSScriptRoot\build_utils.ps1"
Setup-VisualStudioEnvironment -ErrorAction Stop

# Create build output directory
if (-not (Test-Path -Path ".\build_output" -PathType Container)) {
    New-Item -Path ".\build_output" -ItemType Directory -Force | Out-Null
}

# If "test" is passed as an argument, build and run tests
if ($args.Count -gt 0 -and $args[0] -eq "tests") {
    # Get the specific test name if provided
    $specificTest = if ($args.Count -gt 1) { $args[1] } else { $null }

    # Filter tests if specific test name is provided
    $testsToRun = $tests | Where-Object { !$specificTest -or $_.Name -eq $specificTest }

    if ($specificTest -and $testsToRun.Count -eq 0) {
        Write-Host "Error: Test '$specificTest' not found." -ForegroundColor Red
        exit 1
    }

    # Build the test executables
    foreach ($test in $testsToRun) {
        Write-Host "`nCompiling test: $($test.Name)..."

        $sourceFilesStr = $test.SourceFiles -join " "
        $librariesStr = $test.Libraries -join " "
        $compilerFlagsStr = $test.CompilerFlags -join " "

        $command = "cl.exe $compilerFlagsStr $sourceFilesStr $librariesStr /Fo`".\build_output\\`" /Fe`".\build_output\test_$($test.Name).exe`" /link /DEBUG"
        Invoke-Expression $command

        if ($LASTEXITCODE -ne 0) {
            Write-Host "Error building test: $($test.Name)." -ForegroundColor Red
            exit 1
        } else {
            Write-Host "Successfully built test_$($test.Name).exe" -ForegroundColor Green
        }
    }

    # Run the test executables
    foreach ($test in $testsToRun) {
        Write-Host "`nRunning test: $($test.Name)..." -ForegroundColor Cyan
        $testExePath = ".\build_output\test_$($test.Name).exe"
        & $testExePath
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Test run failure" -ForegroundColor Red
            exit 1
        }
    }

    Write-Host "`nAll tests completed successfully!" -ForegroundColor Green
    exit 0
}

#
# Build standalone executables
#

foreach ($executable in $standaloneExecutables) {
    Write-Host "`nCompiling $($executable.Name)..."

    $sourceFilesStr = $executable.SourceFiles -join " "
    $librariesStr = $executable.Libraries -join " "
    $compilerFlagsStr = $executable.CompilerFlags -join " "

    $command = "cl.exe $compilerFlagsStr $sourceFilesStr $librariesStr /Fo`".\build_output\\`" /Fe`".\build_output\$($executable.Name).exe`""
    Invoke-Expression $command

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error building $($executable.Name)." -ForegroundColor Red
        exit 1
    } else {
        Write-Host "Successfully built $($executable.Name).exe" -ForegroundColor Green
    }
}

#
# Build the native extensions
#

foreach ($extension in $extensions) {
    Write-Host "`nCompiling $($extension.Name)..."

    $sourceFilesStr = $extension.SourceFiles -join " "
    $command = "cl.exe /std:c++20 /LD /EHsc $sourceFilesStr dbgeng.lib /Fo`".\build_output\\`" /link /out:`".\build_output\$($extension.Name).dll`" /IMPLIB:`".\build_output\$($extension.Name).lib`" /PDB:`".\build_output\$($extension.Name).pdb`" /MACHINE:X64"
    Invoke-Expression $command

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error building $($extension.Name)." -ForegroundColor Red
        exit 1
    } else {
        Write-Host "Successfully built $($extension.Name).dll" -ForegroundColor Green
    }
}

#
# Create the debug_env_startup_commands.txt file
#

$currentDir = (Get-Location).Path
$buildOutputDir = Join-Path -Path $currentDir -ChildPath "build_output"
$buildOutputDirWinDbg = $buildOutputDir -replace '\\', '\\'
$scriptsDir = Join-Path -Path $currentDir -ChildPath "scripts"
$scriptsDirWinDbg = $scriptsDir -replace '\\', '\\'
$outputFile = Join-Path -Path $currentDir -ChildPath "debug_env_startup_commands.txt"
$contentLines = @()

foreach ($extension in $extensions) {
    # Only include extensions where the DLL exists
    $dllPath = Join-Path -Path $buildOutputDir -ChildPath "$($extension.Name).dll"
    if (Test-Path $dllPath) {
        $contentLines += ".load `"$buildOutputDirWinDbg\\$($extension.Name).dll`""
    } else {
        Write-Host "Warning: $($extension.Name).dll not found, skipping in startup commands" -ForegroundColor Yellow
    }
}

foreach ($script in $scripts) {
    $contentLines += ".scriptload `"$scriptsDirWinDbg\\$script.js`""
}

foreach ($command in $postLoadCommands) {
    $contentLines += $command
}

# Join the lines into a single string
$content = $contentLines -join "`r`n"

# Use UTF8NoBOM encoding to avoid BOM in the output file
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($outputFile, $content, $utf8NoBom)
Write-Host "`nCreated debug_env_startup_commands.txt."
