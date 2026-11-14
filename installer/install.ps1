# isaac-highfps installer for Windows. Run install.bat (or uninstall.bat) instead of this file.
param([switch]$Uninstall)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$gameDirName = 'The Binding of Isaac Rebirth'

function Find-IsaacFolder {
    $steamRoots = @()
    foreach ($key in 'HKCU:\Software\Valve\Steam', 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam', 'HKLM:\SOFTWARE\Valve\Steam') {
        $p = Get-ItemProperty -Path $key -ErrorAction SilentlyContinue
        if ($p.SteamPath)   { $steamRoots += $p.SteamPath }
        if ($p.InstallPath) { $steamRoots += $p.InstallPath }
    }
    $steamRoots += "${env:ProgramFiles(x86)}\Steam", "$env:ProgramFiles\Steam"
    $libraries = @()
    foreach ($root in ($steamRoots | Select-Object -Unique)) {
        $root = $root -replace '/', '\'
        $libraries += $root
        $vdf = Join-Path $root 'steamapps\libraryfolders.vdf'
        if (Test-Path $vdf) {
            foreach ($m in (Select-String -Path $vdf -Pattern '"path"\s+"([^"]+)"')) {
                $libraries += ($m.Matches[0].Groups[1].Value -replace '\\\\', '\')
            }
        }
    }
    foreach ($lib in ($libraries | Select-Object -Unique)) {
        $dir = Join-Path $lib "steamapps\common\$gameDirName"
        if (Test-Path (Join-Path $dir 'isaac-ng.exe')) { return $dir }
    }
    return $null
}

function Ask-IsaacFolder {
    Add-Type -AssemblyName System.Windows.Forms
    $dlg = New-Object System.Windows.Forms.FolderBrowserDialog
    $dlg.Description = "Select the folder that contains isaac-ng.exe`n(Steam > right-click the game > Manage > Browse local files)"
    if ($dlg.ShowDialog() -eq 'OK' -and (Test-Path (Join-Path $dlg.SelectedPath 'isaac-ng.exe'))) { return $dlg.SelectedPath }
    return $null
}

Write-Host ''
Write-Host '  isaac-highfps' -ForegroundColor Cyan
Write-Host ''

$game = Find-IsaacFolder
if (-not $game) {
    Write-Host '  Could not find the game automatically, please select its folder...'
    $game = Ask-IsaacFolder
}
if (-not $game) { Write-Host '  Game folder not found. Nothing was changed.' -ForegroundColor Red; exit 1 }
Write-Host "  Game folder: $game"

if (Get-Process -Name 'isaac-ng' -ErrorAction SilentlyContinue) {
    Write-Host '  The game is running. Close it and run this again.' -ForegroundColor Yellow; exit 1
}

if ($Uninstall) {
    foreach ($f in 'winmm.dll', 'highfps.ini', 'highfps.log') {
        $p = Join-Path $game $f
        if (Test-Path $p) { Remove-Item $p -Force }
    }
    Write-Host '  Uninstalled. The game is back to normal.' -ForegroundColor Green
    exit 0
}

Copy-Item (Join-Path $here 'winmm.dll') (Join-Path $game 'winmm.dll') -Force
$ini = Join-Path $game 'highfps.ini'
if (-not (Test-Path $ini)) { Copy-Item (Join-Path $here 'highfps.ini') $ini }
Write-Host '  Installed! Launch the game from Steam as usual.' -ForegroundColor Green
Write-Host '  Tip: turn VSync on in the game options so the frame rate follows your monitor.'
