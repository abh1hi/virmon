# ==============================================================================
# VirMon Virtual Display Driver - Enhanced Installation Script
# ==============================================================================

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "  VirMon Virtual Display Installer  " -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Self-elevation to Administrator
if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "Requesting Administrator privileges..." -ForegroundColor Yellow
    Start-Process powershell.exe "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

# ==============================================================================
# STEP 1: Check for Driver Binary
# ==============================================================================
Write-Host "STEP 1: Checking for VirMonDriver.sys..." -ForegroundColor Cyan

$possibleDriverPaths = @(
    "C:\Users\abhin\Desktop\virmon\WindowsSender\x64\Release\VirMonDriver.dll",
    "C:\Users\abhin\Desktop\virmon\WindowsSender\x64\Debug\VirMonDriver.dll",
    "C:\Users\abhin\Desktop\virmon\WindowsSender\x64\Release\VirMonDriver\VirMonDriver.dll",
    "C:\Users\abhin\Desktop\virmon\WindowsSender\VirMonDriver\x64\Release\VirMonDriver.dll"
)

$driverPath = $null
foreach ($path in $possibleDriverPaths) {
    if (Test-Path $path) {
        $driverPath = $path
        Write-Host "  [OK] Found driver at: $path" -ForegroundColor Green
        break
    }
}

if (-not $driverPath) {
    Write-Host ""
    Write-Host "  [ERROR] VirMonDriver.sys not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "  The driver binary has not been built yet." -ForegroundColor Yellow
    Write-Host "  Please build the driver first using one of these methods:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Method 1: Visual Studio" -ForegroundColor White
    Write-Host "    1. Open VirMonDriver.vcxproj in Visual Studio" -ForegroundColor Gray
    Write-Host "    2. Set configuration to 'Release' and platform to 'x64'" -ForegroundColor Gray
    Write-Host "    3. Build > Build Solution (Ctrl+Shift+B)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  Method 2: MSBuild (Command Line)" -ForegroundColor White
    Write-Host "    Run from Developer Command Prompt for VS:" -ForegroundColor Gray
    Write-Host "    msbuild VirMonDriver.vcxproj /p:Configuration=Release /p:Platform=x64" -ForegroundColor Gray
    Write-Host ""
    Read-Host "Press Enter to exit"
    exit 1
}

# Get driver directory and INF path
$driverDir = Split-Path -Parent $driverPath
$infPath = Join-Path $driverDir "VirMonDriver.inf"

if (-not (Test-Path $infPath)) {
    Write-Host "  [ERROR] VirMonDriver.inf not found at $infPath" -ForegroundColor Red
    exit 1
}

Write-Host "  [OK] INF file found: $infPath" -ForegroundColor Green
Write-Host ""

# ==============================================================================
# STEP 2: Check Test Signing Status
# ==============================================================================
Write-Host "STEP 2: Checking test signing status..." -ForegroundColor Cyan

$testsigningEnabled = $false
try {
    $bcdeditOutput = & bcdedit | Out-String
    if ($bcdeditOutput -match "testsigning\s+Yes") {
        $testsigningEnabled = $true
        Write-Host "  [OK] Test signing is already enabled" -ForegroundColor Green
    } else {
        Write-Host "  [WARN] Test signing is NOT enabled" -ForegroundColor Yellow
    }
} catch {
    Write-Host "  [WARN] Could not check test signing status" -ForegroundColor Yellow
}

if (-not $testsigningEnabled) {
    Write-Host ""
    Write-Host "  Enabling test signing..." -ForegroundColor Yellow
    try {
        & bcdedit /set testsigning on | Out-Null
        Write-Host "  [OK] Test signing enabled successfully" -ForegroundColor Green
        Write-Host "  [IMPORTANT] You MUST restart your computer for this to take effect!" -ForegroundColor Yellow
        $needsReboot = $true
    } catch {
        Write-Host "  [ERROR] Failed to enable test signing" -ForegroundColor Red
        Write-Host "  You may need to disable Secure Boot in BIOS/UEFI settings" -ForegroundColor Yellow
        Read-Host "Press Enter to continue anyway (driver may not load)"
    }
}
Write-Host ""

# ==============================================================================
# STEP 3: Install Test Certificate (if available)
# ==============================================================================
Write-Host "STEP 3: Checking for test certificate..." -ForegroundColor Cyan

$certPath = Join-Path $driverDir "VirMonDriver.cer"
if (Test-Path $certPath) {
    Write-Host "  [OK] Certificate found, installing..." -ForegroundColor Yellow
    try {
        Import-Certificate -FilePath $certPath -CertStoreLocation Cert:\LocalMachine\Root -ErrorAction SilentlyContinue | Out-Null
        Import-Certificate -FilePath $certPath -CertStoreLocation Cert:\LocalMachine\TrustedPublisher -ErrorAction SilentlyContinue | Out-Null
        Write-Host "  [OK] Certificate installed" -ForegroundColor Green
    } catch {
        Write-Host "  [WARN] Certificate installation failed (may not be needed)" -ForegroundColor Yellow
    }
} else {
    Write-Host "  [WARN] No certificate found (driver may be unsigned)" -ForegroundColor Yellow
    Write-Host "  This is OK if test signing is enabled" -ForegroundColor Gray
}
Write-Host ""

# ==============================================================================
# STEP 4: Check/Build DeviceInstaller
# ==============================================================================
Write-Host "STEP 4: Checking for DeviceInstaller.exe..." -ForegroundColor Cyan

$installerPaths = @(
    "C:\Users\abhin\Desktop\virmon\WindowsSender\build\Release\DeviceInstaller.exe",
    "C:\Users\abhin\Desktop\virmon\WindowsSender\x64\Release\DeviceInstaller.exe"
)

$installerExe = $null
foreach ($path in $installerPaths) {
    if (Test-Path $path) {
        $installerExe = $path
        Write-Host "  [OK] Found installer at: $path" -ForegroundColor Green
        break
    }
}

if (-not $installerExe) {
    Write-Host "  [WARN] DeviceInstaller.exe not found, will use pnputil instead" -ForegroundColor Yellow
    $useModernInstaller = $false
} else {
    $useModernInstaller = $true
}
Write-Host ""

# ==============================================================================
# STEP 5: Install Driver
# ==============================================================================
Write-Host "STEP 5: Installing driver..." -ForegroundColor Cyan

if ($useModernInstaller) {
    Write-Host "  Using modern PnP installer..." -ForegroundColor Yellow
    try {
        # Pass the INF path as an argument to avoid Working Directory issues
        $proc = Start-Process -FilePath $installerExe -ArgumentList "`"$infPath`"" -Verb RunAs -Wait -PassThru
        
        if ($proc.ExitCode -eq 0) {
             Write-Host "  [OK] Driver installation completed" -ForegroundColor Green
        } else {
             throw "Installer exited with code $($proc.ExitCode)"
        }
    } catch {
        Write-Host "  [ERROR] Modern installer failed: $_" -ForegroundColor Red
        Write-Host "  Falling back to pnputil..." -ForegroundColor Yellow
        $useModernInstaller = $false
    }
}

if (-not $useModernInstaller) {
    Write-Host "  Using pnputil..." -ForegroundColor Yellow
    try {
        & pnputil /add-driver $infPath /install
        Write-Host "  [OK] Driver installed via pnputil" -ForegroundColor Green
    } catch {
        Write-Host "  [ERROR] Driver installation failed: $_" -ForegroundColor Red
        exit 1
    }
}
Write-Host ""

# ==============================================================================
# STEP 6: Verify Installation
# ==============================================================================
Write-Host "STEP 6: Verifying installation..." -ForegroundColor Cyan

$device = Get-PnpDevice | Where-Object { $_.FriendlyName -like "*VirMon*" } | Select-Object -First 1

if ($device) {
    Write-Host "  Driver device found: $($device.FriendlyName)" -ForegroundColor Green
    Write-Host "  Status: $($device.Status)" -ForegroundColor $(if ($device.Status -eq "OK") { "Green" } else { "Yellow" })
    
    if ($device.Status -ne "OK") {
        Write-Host ""
        Write-Host "  [WARNING] Device status is not 'OK'" -ForegroundColor Yellow
        Write-Host "  This usually means:" -ForegroundColor Yellow
        Write-Host "    - Test signing is not enabled (needs restart)" -ForegroundColor Gray
        Write-Host "    - Driver is not properly signed" -ForegroundColor Gray
        Write-Host "    - Driver binary has errors" -ForegroundColor Gray
        Write-Host ""
        
        if ($device.Problem) {
            Write-Host "  Problem Code: $($device.Problem)" -ForegroundColor Red
        }
    }
} else {
    Write-Host "  [WARN] No VirMon device found (may need restart)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "  Installation Summary" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan

if ($needsReboot) {
    Write-Host ""
    Write-Host "  *** RESTART REQUIRED ***" -ForegroundColor Red
    Write-Host "  Test signing was just enabled." -ForegroundColor Yellow
    Write-Host "  You MUST restart Windows before the driver will work." -ForegroundColor Yellow
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "  Driver installation completed!" -ForegroundColor Green
    Write-Host ""
    Write-Host "  Next steps:" -ForegroundColor Cyan
    Write-Host "    1. Check Display Settings (Win+P or Settings > Display)" -ForegroundColor White
    Write-Host "    2. Look for 'VirMon Virtual Display' as a second monitor" -ForegroundColor White
    Write-Host "    3. Extend or duplicate your display to the virtual monitor" -ForegroundColor White
    Write-Host "    4. Run VirMonSender.exe to start streaming" -ForegroundColor White
    Write-Host ""
}

Read-Host "Press Enter to exit"
