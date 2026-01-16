# fix_driver_build.ps1
$ErrorActionPreference = "Stop"

# Check for Administrator privileges
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must be run as Administrator to copy files to Program Files."
    exit 1
}

Write-Host "Locating WDK VSIX..."
$WdkVsixPath = "C:\Program Files (x86)\Windows Kits\10\Vsix\VS2022\10.0.22621.0\WDK.vsix"
if (-not (Test-Path $WdkVsixPath)) {
    Write-Error "WDK VSIX not found at expected path: $WdkVsixPath"
    exit 1
}

$TempDir = Join-Path $env:TEMP "WDK_Fix_$(Get-Random)"
$TempZip = Join-Path $TempDir "WDK.zip"
$ExtractDir = Join-Path $TempDir "Extracted"

Write-Host "Preparing temporary files in $TempDir..."
New-Item -ItemType Directory -Path $TempDir -Force | Out-Null
Copy-Item -Path $WdkVsixPath -Destination $TempZip
Expand-Archive -Path $TempZip -DestinationPath $ExtractDir -Force

# Define source and destination
# The VSIX usually contains a folder named `$MSBuild` (literal) or sometimes just `MSBuild`
$SourceMSBuild = Join-Path $ExtractDir "`$MSBuild" 
if (-not (Test-Path $SourceMSBuild)) {
    $SourceMSBuild = Join-Path $ExtractDir "MSBuild"
}

if (-not (Test-Path $SourceMSBuild)) {
    Write-Error "Could not find MSBuild folder in extracted VSIX. Contents:"
    Get-ChildItem $ExtractDir | Select-Object Name
    exit 1
}

$DestMSBuildBase = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild"

Write-Host "Copying WDK MSBuild files to Visual Studio Build Tools..."
Write-Host "Source: $SourceMSBuild"
Write-Host "Dest:   $DestMSBuildBase"

# Use Robocopy for robust merging (Exit code 1 means success/files copied)
$proc = Start-Process robocopy.exe -ArgumentList "`"$SourceMSBuild`" `"$DestMSBuildBase`" /E /XC /XN /XO" -Wait -PassThru -NoNewWindow
if ($proc.ExitCode -gt 7) {
    Write-Error "Robocopy failed with exit code $($proc.ExitCode)"
    exit 1
}

Write-Host "Files copied successfully."
Write-Host "Cleaning up..."
Remove-Item -Path $TempDir -Recurse -Force

Write-Host "Done! Please try building the driver again."
