# SignDriver.ps1
param(
    [string]$DriverPath = ".\x64\Release"
)

$CertName = "VirMonDevCert"
$CertStore = "Cert:\LocalMachine\My"

# 1. Check if Cert exists, else create
$Cert = Get-ChildItem $CertStore | Where-Object { $_.Subject -match "CN=$CertName" }
if (-not $Cert) {
    Write-Host "Creating Self-Signed Certificate..."
    $Cert = New-SelfSignedCertificate -Subject "CN=$CertName" -CertStoreLocation $CertStore -KeyUsage DigitalSignature -Type CodeSigningCert
    
    # Export to Trusted Root (Requires Admin)
    Write-Host "Installing Cert to Trusted Root..."
    $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store "Root", "LocalMachine"
    $rootStore.Open("ReadWrite")
    $rootStore.Add($Cert)
    $rootStore.Close()
}

# 2. Sign Files
$SignTool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe" # Adjust path as needed
if (-not (Test-Path $SignTool)) {
    Write-Error "SignTool not found. Please install WDK."
    exit
}

$FilesToSign = @("$DriverPath\VirMonDriver.cat", "$DriverPath\VirMonDriver.sys")

foreach ($File in $FilesToSign) {
    if (Test-Path $File) {
        Write-Host "Signing $File..."
        & $SignTool sign /v /s My /n "$CertName" /t http://timestamp.digicert.com /fd sha256 "$File"
    } else {
        Write-Warning "File not found: $File"
    }
}

# 3. Enable Test Signing
Write-Host "You must enable Test Signing to load this driver: bcdedit /set testsigning on"
