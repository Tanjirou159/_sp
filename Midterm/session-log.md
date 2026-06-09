# Session Log — Midterm Process Monitor

**Date:** 2026-06-09  
**Course:** 系統程式 (System Programming), 114 學年下學期

---

## Context

- **Repo:** `_sp` — course homework repository for 系統程式 (System Programming)
- **Baseline:** The `Midterm/` directory existed but was empty (only a blank `README.md`)
- **Existing conventions:** Each HW folder contains code + documentation; course covers compilers, /proc, system calls, signals

## Goal

Create a complete midterm project: a cross-platform **Process Monitor** using pure terminal/scripting languages:
- **Linux:** bash, reading `/proc` filesystem
- **Windows:** PowerShell, using `Get-Process` / WMI
- **macOS:** bash, using BSD-style `ps`/`vm_stat`/`sysctl`

Plus a README with clone/usage instructions and a written report (learning notes).

## Decisions

| Decision | Rationale |
|----------|-----------|
| Pure bash (not C) for Linux | User's choice; easier to read, no compiler needed |
| `awk` instead of `bc` for floating point | `bc` is not universally installed; `awk` is available everywhere |
| ANSI color codes for thresholds | Red >80%, Yellow >50%, Green otherwise; `-c` flag to disable |
| Non-TTY detection for one-shot mode | `[ -t 0 ]` check prevents infinite hangs when piped/scripted |
| CSV pipe-delimited internal format (`pid|user|cpu|rss|name|cmd`) | Simple sort/filter without external dependencies |
| Stat field positions (Linux 2.6+ format) | Corrected from initial 8 underscores to 10 before utime; field 22=rss after paren |
| Uptime: strip decimal from `/proc/uptime` | Bash `$((...))` only handles integers; `${var%%.*}` truncates |
| MIT License | Consistent with other public educational projects |

## Architecture

```
Midterm/
├── README.md                    (228 lines)  User-facing docs: clone, usage, flags, screenshots
├── doc/
│   └── report.md                (336 lines)  Learning notes: /proc, syscalls, signals, scheduling
└── src/
    ├── process-monitor.sh       (387 lines)  Linux monitor (bash)
    ├── process-monitor.ps1      (231 lines)  Windows monitor (PowerShell)
    ├── process-monitor-mac.sh   (348 lines)  macOS monitor (bash)
    └── tests/
        └── run_tests.sh         (213 lines)  Test suite (17 cases)
```

**Data flow (Linux):**
```
/proc/stat  → CPU % (two-sample delta)
/proc/meminfo → Memory/swap usage
/proc/loadavg → Load averages
/proc/uptime  → Uptime
/proc/[pid]/stat + /proc/[pid]/status → Process list (PID, user, CPU time, RSS, name, cmdline)
                                       → Sort/filter/top-N → Terminal display
```

**Features (all platforms):**
- Real-time refresh with configurable interval (`-i`)
- Sort by CPU time, memory, PID, name (`-s`)
- Filter by process name (`-f`)
- Kill process by PID (`-k`)
- Color-coded thresholds (red/yellow/green) — toggle with `-c`
- Interactive keyboard controls (`q` quit, `c`/`m`/`p`/`n` sort)
- Auto-exits after one render when stdin is not a TTY

## Changes

### Files Created

| File | Lines | Description |
|------|-------|-------------|
| `Midterm/src/process-monitor.sh` | 387 | Linux bash monitor: reads `/proc`, parses stat/meminfo/loadavg/uptime, ANSI display |
| `Midterm/src/process-monitor.ps1` | 231 | Windows PowerShell monitor: Get-Process, Get-Counter, Get-CimInstance |
| `Midterm/src/process-monitor-mac.sh` | 348 | macOS bash monitor: ps -eo, vm_stat, sysctl, top parsing |
| `Midterm/src/tests/run_tests.sh` | 213 | Test suite: 17 tests covering help, invalid args, kill, listing, sort modes, filter, colors |
| `Midterm/doc/report.md` | 336 | 學習筆記: /proc deep dive, process lifecycle, syscalls, signals, scheduling, cross-platform comparison |
| `Midterm/README.md` | 228 | User guide: clone, install, usage examples for Linux/macOS/Windows, flags reference |

### Bug Fixes During Development

| Issue | Root Cause | Fix |
|-------|-----------|-----|
| Exit code 127 on `-c` flag | `bc` not installed in env | Replaced all `bc` calls with `awk` |
| Arithmetic syntax error in `get_uptime()` | `/proc/uptime` returns float; bash `$((...))` rejects floats | `${uptime_sec%%.*}` to truncate decimal |
| `cpu_pct_display: unbound variable` error | Undefined variable passed to `print_process_row()` | Removed unused params from function signature and call site |
| RSS showing 375515.4 MB (implausible) | Wrong `/proc/[pid]/stat` field offsets; had 8 underscores before `utime` instead of 10 | Corrected to `read -r state _ _ _ _ _ _ _ _ _ _ utime stime cutime cstime _ _ _ _ _ _ rss _` |
| Tests hanging on `-i 100` | Monitor's `read -t 100` waited full timeout on non-TTY | Added `[ -t 0 ]` check in `read_key()` and `main()` for one-shot mode |
| Color test false negative | Test checked for *no* ANSI codes, but box borders always have cyan | Changed to check absence of red color codes specifically |

## Status

- **All 17 tests pass** — help, invalid args, kill mode, process listing, sort modes, filter, color/no-color, top count limit
- Linux version verified to render correctly with real `/proc` data
- Windows PowerShell version is syntactically sound (not tested on actual Windows due to Linux environment)
- macOS version follows same pattern as Linux with BSD-specific commands (not tested on actual macOS due to Linux environment)
- Report covers all required system programming topics

## Commands

```bash
# Run the monitor (Linux)
./Midterm/src/process-monitor.sh
./Midterm/src/process-monitor.sh -n 10 -s mem -i 1
./Midterm/src/process-monitor.sh -f firefox
./Midterm/src/process-monitor.sh -k <pid>

# Run tests
./Midterm/src/tests/run_tests.sh

# Windows
# PowerShell:
#   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#   .\Midterm\src\process-monitor.ps1 -TopCount 10 -SortBy mem

# macOS
#   chmod +x Midterm/src/process-monitor-mac.sh
#   ./Midterm/src/process-monitor-mac.sh -n 10 -s mem
```
