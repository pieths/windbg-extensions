# This script copies the dbgeng.h file from the Windows SDK to the
# prompts directory so that it can be used for extra context when
# using Copilot. This is only needed at the moment since it appears
# that the prompt.md files are not able to reference files outside
# of the current disk (i.e. absolute paths to other disk drives).

$dbgengPath = "C:\Program Files (x86)\Windows Kits\10\Debuggers\inc\dbgeng.h"
$instructionsDir = "$PSScriptRoot\.github\instructions"

# Check if Windows SDK is installed and dbgeng.h exists
if (-not (Test-Path $dbgengPath)) {
    Write-Error "Windows SDK debugging tools not found. Please install the Windows SDK with Debugging Tools."
    Write-Error "Expected file not found: $dbgengPath"
    Write-Error "Download from: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/"
    Write-Error "During installation, ensure 'Debugging Tools for Windows' is selected."
    Write-Error "Re-run this script after installation."
    exit 1
}

# Copy the file
cp $dbgengPath $instructionsDir\.
Write-Host "Successfully copied dbgeng.h to $instructionsDir" -ForegroundColor Green
