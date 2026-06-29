param(
    [string]$SourceFile = 'psFlip.cpp',
    [string]$OutputFile = 'psFlip.exe'
)

if (-not (Test-Path $SourceFile)) {
    Write-Error "Source file '$SourceFile' does not exist."
    exit 1
}

if ($SourceFile -ne 'psFlip.cpp') {
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourceFile)
    $OutputFile = "$baseName.exe"
}

$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Error 'g++ was not found on PATH. Install MinGW or another GCC toolchain, or update PATH.'
    exit 1
}

$arguments = @('-O3', '-Wall', '-Wextra', $SourceFile, '-std=c++17', '-o', $OutputFile)
Write-Host "Compiling $SourceFile to $OutputFile..."

$process = Start-Process -FilePath $gpp.Path -ArgumentList $arguments -NoNewWindow -Wait -PassThru
if ($process.ExitCode -ne 0) {
    Write-Error "Compilation failed with exit code $($process.ExitCode)."
    exit $process.ExitCode
}

Write-Host "Compilation succeeded: $OutputFile"

# Register the exe path in Windows system PATH
$exeFullPath = (Resolve-Path $OutputFile).Path
$exeDirectory = Split-Path -Parent $exeFullPath

# Ask for user permission
$permission = Read-Host "Do you want to register the exe in Windows system PATH? This requires administrator privileges (Y/N)"

if ($permission -eq 'Y' -or $permission -eq 'y') {
    # Check if running as administrator
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    
    if (-not $isAdmin) {
        Write-Error "Administrator privileges required. Please run this script as administrator (right-click PowerShell and select 'Run as administrator')."
        exit 1
    }
    
    try {
        $currentPath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
        
        if ($currentPath -notlike "*$exeDirectory*") {
            Write-Host "Registering exe path in Windows system PATH: $exeDirectory"
            
            $newPath = "$currentPath;$exeDirectory"
            [Environment]::SetEnvironmentVariable('Path', $newPath, 'Machine')
            
            Write-Host "Successfully added to system PATH. You may need to restart your shell for changes to take effect."
        } else {
            Write-Host "Exe directory is already in system PATH."
        }
    } catch {
        Write-Error "Failed to register path in system PATH. Error: $_"
        exit 1
    }
} else {
    Write-Host "Skipped registering exe in system PATH."
}