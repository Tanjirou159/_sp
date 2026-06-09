<#
.SYNOPSIS
    Simple cross-platform process monitor - Windows edition
.DESCRIPTION
    Displays real-time system resource usage and top running processes.
    Features: auto-refresh, sort by CPU/memory/PID/name, filter by name, kill by PID.
.NOTES
    Version: 1.0.0
    Platform: Windows PowerShell 5.1+ / PowerShell Core 6+
#>

param(
    [int]$TopCount = 20,
    [ValidateSet("cpu","mem","pid","name")]
    [string]$SortBy = "cpu",
    [string]$FilterName = "",
    [double]$Interval = 2,
    [switch]$NoColor = $false,
    [int]$KillPid = 0,
    [switch]$Help = $false
)

# --- Help ---------------------------------------------------------
if ($Help) {
    Write-Host @"
BOLDprocess-monitor.ps1 VERSION 1.0.0 — Windows real-time process monitor

USAGE:
  .\process-monitor.ps1 [options]

OPTIONS:
  -TopCount <N>       Show top N processes (default: 20)
  -SortBy <column>    Sort by: cpu, mem, pid, name (default: cpu)
  -FilterName <name>  Filter processes by name (case-insensitive)
  -KillPid <pid>      Kill the process with given PID and exit
  -Interval <seconds> Refresh interval in seconds (default: 2)
  -NoColor            Disable colored output
  -Help               Show this help message

EXAMPLES:
  .\process-monitor.ps1                            # Default
  .\process-monitor.ps1 -TopCount 10 -SortBy mem   # Top 10 by memory
  .\process-monitor.ps1 -FilterName chrome          # Chrome only
  .\process-monitor.ps1 -KillPid 1234               # Kill process 1234

INTERACTIVE KEYS (during monitoring):
  q  Quit    c  Sort by CPU    m  Sort by memory
  p  Sort by PID    n  Sort by name
"@
    exit 0
}

# --- Kill mode ----------------------------------------------------
if ($KillPid -gt 0) {
    $proc = Get-Process -Id $KillPid -ErrorAction SilentlyContinue
    if (-not $proc) {
        Write-Host "Error: No process with PID $KillPid" -ForegroundColor Red
        exit 1
    }
    Write-Host "Killing process $KillPid ($($proc.ProcessName)) ..."
    Stop-Process -Id $KillPid -Force
    Write-Host "Done."
    exit 0
}

# --- Helper: color-coded percentage -------------------------------
function Color-Pct {
    param([double]$Value, [string]$Label = "")
    if ($NoColor) {
        return "{0}{1,6:F1}%" -f $Label, $Value
    }
    if ($Value -gt 80) { return "$(Write-Host "$Label$($Value.ToString('F1'))%" -ForegroundColor Red -NoNewLine)" }
    elseif ($Value -gt 50) { return "$(Write-Host "$Label$($Value.ToString('F1'))%" -ForegroundColor Yellow -NoNewLine)" }
    else { return "$(Write-Host "$Label$($Value.ToString('F1'))%" -ForegroundColor Green -NoNewLine)" }
}

# --- System info --------------------------------------------------
function Get-CpuUsage {
    $cpu = Get-Counter '\Processor(_Total)\% Processor Time' -ErrorAction SilentlyContinue
    if ($cpu -and $cpu.CounterSamples) {
        return [math]::Round($cpu.CounterSamples[0].CookedValue, 1)
    }
    return 0.0
}

function Get-MemInfo {
    $os = Get-CimInstance Win32_OperatingSystem
    $totalMB = [math]::Round($os.TotalVisibleMemorySize / 1024)
    $freeMB  = [math]::Round($os.FreePhysicalMemory / 1024)
    $usedMB  = $totalMB - $freeMB
    return @{
        TotalMB = $totalMB
        UsedMB  = $usedMB
        FreeMB  = $freeMB
    }
}

function Get-UpTime {
    $os = Get-CimInstance Win32_OperatingSystem
    $uptime = $os.LastBootUpTime
    if ($uptime) {
        $span = (Get-Date) - $uptime
        return "{0}d {1}h {2}m" -f $span.Days, $span.Hours, $span.Minutes
    }
    return "N/A"
}

function Get-SystemLoad {
    $cpu_samples = (Get-Counter '\System\Processor Queue Length' -ErrorAction SilentlyContinue).CounterSamples
    if ($cpu_samples) {
        return [math]::Round($cpu_samples[0].CookedValue, 2)
    }
    return 0
}

# --- Process list -------------------------------------------------
function Get-ProcessList {
    param([string]$SortColumn = "cpu")

    $procs = Get-Process | ForEach-Object {
        $p = $_
        $cpuSec = [math]::Round($p.TotalProcessorTime.TotalSeconds, 2)
        $memMB  = [math]::Round($p.WorkingSet64 / 1MB, 1)
        $user   = try { (Get-CimInstance Win32_Process -Filter "ProcessId=$($p.Id)").GetOwner().User } catch { "N/A" }

        [PSCustomObject]@{
            PID      = $p.Id
            User     = if ($user) { $user } else { "N/A" }
            CPUTime  = $cpuSec
            MemMB    = $memMB
            Name     = $p.ProcessName
            Command  = if ($p.MainWindowTitle) { $p.MainWindowTitle } else { $p.ProcessName }
        }
    }

    if ($FilterName) {
        $procs = $procs | Where-Object { $_.Name -like "*$FilterName*" }
    }

    switch ($SortColumn) {
        "cpu"  { $procs | Sort-Object CPUTime -Descending }
        "mem"  { $procs | Sort-Object MemMB -Descending }
        "pid"  { $procs | Sort-Object PID -Descending }
        "name" { $procs | Sort-Object Name }
        default { $procs | Sort-Object CPUTime -Descending }
    }
}

# --- Display ------------------------------------------------------
function Show-Display {
    param($CpuPct, $MemInfo, $Uptime, $Processes)

    Clear-Host

    Write-Host "==============================================================" -ForegroundColor Cyan
    Write-Host "  Process Monitor — $env:COMPUTERNAME — $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor White
    Write-Host "==============================================================" -ForegroundColor Cyan
    Write-Host -NoNewline "  CPU: "
    Color-Pct $CpuPct
    Write-Host "  "
    Write-Host -NoNewline "  Mem: ${usedMB} / ${totalMB} MB ("
    Color-Pct $memPct
    Write-Host ")" -NoNewline
    Write-Host "  Uptime: $Uptime"
    Write-Host "==============================================================" -ForegroundColor Cyan
    Write-Host ("{0,-8} {1,-12} {2,-12} {3,-8} {4,-16} {5}" -f "PID","USER","CPU TIME","MEM(MB)","NAME","COMMAND") -ForegroundColor White
    Write-Host "==============================================================" -ForegroundColor Cyan

    if (-not $Processes) {
        Write-Host "  No processes found." -ForegroundColor DarkGray
    } else {
        $Processes | Select-Object -First $TopCount | ForEach-Object {
            $cpuFmt = "{0}h{1:D2}m{2:D2}s" -f [math]::Floor($_.CPUTime/3600), [math]::Floor(($_.CPUTime%3600)/60), [math]::Floor($_.CPUTime%60)
            Write-Host ("{0,-8} {1,-12} {2,-12} {3,-8} {4,-16} {5}" -f $_.PID, $_.User, $cpuFmt, $_.MemMB, $_.Name, $_.Command) -ForegroundColor Gray
        }
    }

    Write-Host "==============================================================" -ForegroundColor Cyan
    Write-Host "[q]uit [c]pu-sort [m]em-sort [p]id-sort [n]ame-sort   interval=${Interval}s" -ForegroundColor DarkGray
}

# --- Interactive key reader ---------------------------------------
if ($Host.Name -notmatch 'ConsoleHost' -and -not $env:WT_SESSION) {
    # Running in ISE or similar — no interactive keys
    $interactive = $false
} else {
    $interactive = $true
}

function Read-Key {
    param([double]$TimeoutSec)
    if (-not $interactive) { return $null }

    $startTime = Get-Date
    while ((Get-Date) - $startTime).TotalSeconds -lt $TimeoutSec {
        if ([Console]::KeyAvailable) {
            return [Console]::ReadKey($true).KeyChar
        }
        Start-Sleep -Milliseconds 100
    }
    return $null
}

# --- Main loop ----------------------------------------------------
$ErrorActionPreference = "SilentlyContinue"

Write-Host "Starting process monitor... (press q to quit)" -ForegroundColor Green

do {
    $cpuPct = Get-CpuUsage
    $memInfo = Get-MemInfo
    $memPct = if ($memInfo.TotalMB -gt 0) { [math]::Round(100 * $memInfo.UsedMB / $memInfo.TotalMB, 1) } else { 0 }
    $uptime = Get-UpTime
    $processes = Get-ProcessList -SortColumn $SortBy

    Show-Display -CpuPct $cpuPct -MemInfo $memInfo -Uptime $uptime -Processes $processes

    $key = Read-Key -TimeoutSec $Interval
    switch ($key) {
        'q' { $quit = $true; Write-Host "`nExiting." -ForegroundColor Green }
        'Q' { $quit = $true; Write-Host "`nExiting." -ForegroundColor Green }
        'c' { $SortBy = "cpu" }
        'C' { $SortBy = "cpu" }
        'm' { $SortBy = "mem" }
        'M' { $SortBy = "mem" }
        'p' { $SortBy = "pid" }
        'P' { $SortBy = "pid" }
        'n' { $SortBy = "name" }
        'N' { $SortBy = "name" }
    }
} until ($quit)
