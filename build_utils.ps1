function Setup-VisualStudioEnvironment {
    # Check if Visual Studio environment is already set up
    $isAlreadySetup = $false

    try {
        $clCommand = Get-Command cl.exe -ErrorAction Stop

        # Check if we have the right architecture and required environment variables
        $hasVsInstallDir = $env:VSINSTALLDIR -ne $null
        $hasVcToolsDir = $env:VCToolsInstallDir -ne $null
        $isX64 = $clCommand.Source -like "*x64*" -or $clCommand.Source -like "*amd64*"

        if ($hasVsInstallDir -and $hasVcToolsDir -and $isX64) {
            $isAlreadySetup = $true
            Write-Host "Visual Studio environment (x64) is already set up" -ForegroundColor Green
            Write-Host "  VS Install Dir: $env:VSINSTALLDIR" -ForegroundColor Gray
            Write-Host "  VC Tools Dir: $env:VCToolsInstallDir" -ForegroundColor Gray
            Write-Host "  Compiler: $($clCommand.Source)" -ForegroundColor Gray
        } else {
            Write-Host "Visual Studio environment partially set up, but missing x64 configuration..." -ForegroundColor Yellow
        }
    } catch {
        Write-Host "Visual Studio environment not detected..." -ForegroundColor Cyan
    }

    if ($isAlreadySetup) {
        return
    }

    Write-Host "Setting up Visual Studio environment..." -ForegroundColor Cyan

    # Common Visual Studio paths to try
    $vsPaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
    )

    $vsDevCmdPath = $null
    foreach ($path in $vsPaths) {
        if (Test-Path $path) {
            $vsDevCmdPath = $path
            Write-Host "Found Visual Studio at: $path" -ForegroundColor Green
            break
        }
    }

    if (-not $vsDevCmdPath) {
        Write-Host "Error: Could not find Visual Studio installation" -ForegroundColor Red
        Write-Host "Searched paths:" -ForegroundColor Yellow
        $vsPaths | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
        throw "Visual Studio not found"
    }

    # Import Visual Studio environment variables
    $tempFile = [System.IO.Path]::GetTempFileName()
    cmd.exe /c "`"$vsDevCmdPath`" -arch=amd64 -host_arch=amd64 && set" > $tempFile

    Get-Content $tempFile | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            $varName = $matches[1]
            $varValue = $matches[2]
            Set-Item -Path "env:$varName" -Value $varValue
        }
    }
    Remove-Item $tempFile

    # Verify cl.exe is available after setup
    try {
        $clCommand = Get-Command cl.exe -ErrorAction Stop
        Write-Host "Visual Studio environment loaded successfully" -ForegroundColor Green
        Write-Host "Using compiler at: $($clCommand.Source)" -ForegroundColor Gray
    } catch {
        Write-Host "Error: cl.exe not found after environment setup" -ForegroundColor Red
        throw "Compiler not available after VS environment setup"
    }
}
