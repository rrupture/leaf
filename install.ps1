# Leaf Installer
# Usage: irm https://raw.githubusercontent.com/777rupture/leaf/main/install.ps1 | iex

$ErrorActionPreference = "Stop"

$repo    = "777rupture/leaf"
$exeName = "leaf.exe"
$installDir = "$env:USERPROFILE\.leaf"

Write-Host ""
Write-Host "  Downloading Leaf..." -ForegroundColor Green

# Get latest release download URL
$release = Invoke-RestMethod "https://api.github.com/repos/$repo/releases/latest"
$asset   = $release.assets | Where-Object { $_.name -eq $exeName }

if (-not $asset) {
    Write-Host "  Could not find leaf.exe in the latest release." -ForegroundColor Red
    Write-Host "  Make sure a GitHub Release exists with leaf.exe attached."
    exit 1
}

# Create install dir
if (-not (Test-Path $installDir)) {
    New-Item -ItemType Directory -Path $installDir | Out-Null
}

# Download
$dest = "$installDir\$exeName"
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $dest

Write-Host "  Installed to: $dest" -ForegroundColor Green

# Add to PATH if not already there
$currentPath = [System.Environment]::GetEnvironmentVariable("PATH", "User")
if ($currentPath -notlike "*$installDir*") {
    [System.Environment]::SetEnvironmentVariable("PATH", "$currentPath;$installDir", "User")
    Write-Host "  Added to PATH." -ForegroundColor Green
} else {
    Write-Host "  PATH already set." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "  Done! Open a new terminal and run:" -ForegroundColor Cyan
Write-Host "    leaf -help"
Write-Host ""
