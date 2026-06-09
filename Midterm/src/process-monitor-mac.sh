#!/usr/bin/env bash
set -euo pipefail

# ================================================================
# process-monitor-mac.sh — macOS Process Monitor
# ================================================================
# Uses BSD-style commands (ps, vm_stat, sysctl, top) for macOS.
# Usage: ./process-monitor-mac.sh [-n N] [-s col] [-f name] [-k pid] [-i interval]
# ================================================================

VERSION="1.0.0"
SCRIPT_NAME="$(basename "$0")"
REFRESH_INTERVAL=2
TOP_COUNT=20
SORT_BY="cpu"
FILTER_NAME=""
KILL_PID=""
NO_COLOR=false

# --- ANSI colors -------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

die() {
    echo -e "${RED}Error:${NC} $*" >&2
    exit 1
}

color_pct() {
    local val="$1"
    local label="${2:-}"
    if [ "$NO_COLOR" = true ]; then
        printf '%s%6.1f%%' "$label" "$val"
        return
    fi
    if (( $(awk -v v="$val" 'BEGIN { print (v > 80) ? 1 : 0 }') )); then
        printf '%b%s%6.1f%%%b' "$RED$BOLD" "$label" "$val" "$NC"
    elif (( $(awk -v v="$val" 'BEGIN { print (v > 50) ? 1 : 0 }') )); then
        printf '%b%s%6.1f%%%b' "$YELLOW" "$label" "$val" "$NC"
    else
        printf '%b%s%6.1f%%%b' "$GREEN" "$label" "$val" "$NC"
    fi
}

usage() {
    cat <<EOF
${BOLD}process-monitor-mac.sh${NC} v${VERSION} — macOS real-time process monitor

${BOLD}Usage:${NC}
  $SCRIPT_NAME [options]

${BOLD}Options:${NC}
  -n <N>         Show top N processes (default: 20)
  -s <column>    Sort by: cpu, mem, pid, name (default: cpu)
  -f <name>      Filter processes by name (case-insensitive)
  -k <pid>       Kill the process with given PID and exit
  -i <seconds>   Refresh interval in seconds (default: 2)
  -c             Disable colored output
  -h             Show this help message
  -v             Print version

${BOLD}Interactive keys (during monitoring):${NC}
  q              Quit
  c              Sort by CPU
  m              Sort by memory
  p              Sort by PID
  n              Sort by name

${BOLD}Examples:${NC}
  $SCRIPT_NAME                        # Default: top 20 by CPU, refresh every 2s
  $SCRIPT_NAME -n 10 -s mem           # Top 10 by memory
  $SCRIPT_NAME -f Terminal            # Show only Terminal processes
  $SCRIPT_NAME -k 1234                # Kill process 1234
EOF
    exit 0
}

# --- CLI parsing -------------------------------------------------
while getopts "n:s:f:k:i:chv" opt; do
    case "$opt" in
        n) TOP_COUNT="$OPTARG" ;;
        s) SORT_BY="$OPTARG" ;;
        f) FILTER_NAME="$OPTARG" ;;
        k) KILL_PID="$OPTARG" ;;
        i) REFRESH_INTERVAL="$OPTARG" ;;
        c) NO_COLOR=true ;;
        h) usage ;;
        v) echo "process-monitor-mac.sh v${VERSION}"; exit 0 ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

if ! [[ "$TOP_COUNT" =~ ^[0-9]+$ ]] || [ "$TOP_COUNT" -lt 1 ]; then
    die "Invalid top count: $TOP_COUNT"
fi

# --- Kill mode ---------------------------------------------------
if [ -n "$KILL_PID" ]; then
    if ! kill -0 "$KILL_PID" 2>/dev/null; then
        die "No process with PID $KILL_PID"
    fi
    echo "Killing process $KILL_PID ..."
    kill -9 "$KILL_PID" 2>/dev/null && echo "Done." || die "Failed to kill PID $KILL_PID"
    exit 0
fi

# --- System info collectors (macOS) -------------------------------

get_cpu_usage() {
    # Use top to get CPU usage
    local cpu_line
    cpu_line=$(top -l 1 -n 0 2>/dev/null | grep 'CPU usage' || echo "0.0")
    # Parse: "CPU usage: 5.64% user, 12.34% sys, 82.01% idle"
    local user sys
    user=$(echo "$cpu_line" | sed -n 's/.*CPU usage: \([0-9.]*\)% user.*/\1/p' 2>/dev/null)
    sys=$(echo "$cpu_line" | sed -n 's/.* \([0-9.]*\)% sys.*/\1/p' 2>/dev/null)
    user="${user:-0}"
    sys="${sys:-0}"
    awk -v u="$user" -v s="$sys" 'BEGIN { printf "%.1f", u + s }'
}

get_mem_info() {
    # vm_stat shows pages; page size from sysctl
    local page_size
    page_size=$(sysctl -n hw.pagesize 2>/dev/null || echo 4096)
    page_size=$((page_size / 1024))  # pages to KB factor

    local total_kb
    total_kb=$(sysctl -n hw.memsize 2>/dev/null || echo 0)
    total_kb=$((total_kb / 1024))

    local pages_free=0 pages_active=0 pages_inactive=0 pages_wired=0 pages_spec=0 pages_purgeable=0
    while read -r key val; do
        case "${key%:}" in
            "Pages free")       pages_free="${val%\.}" ;;
            "Pages active")     pages_active="${val%\.}" ;;
            "Pages inactive")   pages_inactive="${val%\.}" ;;
            "Pages wired down") pages_wired="${val%\.}" ;;
            "Pages speculative") pages_spec="${val%\.}" ;;
            "Pages purgeable")  pages_purgeable="${val%\.}" ;;
        esac
    done < <(vm_stat 2>/dev/null)

    local available_pages=$((pages_free + pages_inactive + pages_purgeable + pages_spec))
    local used_pages=$((pages_active + pages_wired))
    local avail_kb=$((available_pages * page_size))
    local used_kb=$((used_pages * page_size))
    local swap_used=0 swap_total=0

    # Swap info
    if command -v sysctl &>/dev/null; then
        local swap_usage
        swap_usage=$(sysctl -n vm.swapusage 2>/dev/null || echo "")
        if [ -n "$swap_usage" ]; then
            # format: "total = 1024.0M  used = 512.0M  free = 512.0M"
            swap_total=$(echo "$swap_usage" | sed -n 's/.*total = \([0-9.]*\)M.*/\1/p' || echo 0)
            swap_used=$(echo "$swap_usage"  | sed -n 's/.*used = \([0-9.]*\)M.*/\1/p'  || echo 0)
            swap_total=$(( ${swap_total%.*} * 1024 ))  # MB to KB
            swap_used=$((  ${swap_used%.*}  * 1024 ))
        fi
    fi

    echo "$total_kb $used_kb $avail_kb ${swap_total:-0} ${swap_used:-0}"
}

get_uptime() {
    local boot_time now uptime_sec
    boot_time=$(sysctl -n kern.boottime 2>/dev/null | sed 's/.*sec = \([0-9]*\).*/\1/')
    if [ -z "$boot_time" ]; then
        uptime_sec=$(uptime 2>/dev/null | sed 's/.*up //' | sed 's/ .*//')
        echo "$uptime_sec"
        return
    fi
    now=$(date +%s)
    uptime_sec=$((now - boot_time))
    local d=$(( uptime_sec / 86400 ))
    local h=$(( (uptime_sec % 86400) / 3600 ))
    local m=$(( (uptime_sec % 3600) / 60 ))
    printf '%dd %dh %dm' "$d" "$h" "$m"
}

get_loadavg() {
    local load
    load=$(sysctl -n vm.loadavg 2>/dev/null || echo "{ 0.00 0.00 0.00 }")
    # Format: "{ 1.23 0.45 0.67 }"
    echo "$load" | tr -d '{}' | awk '{print $1, $2, $3}'
}

get_processes() {
    # Use ps to get process list
    # Format: pid user pcpu pmem rss comm
    ps -eo pid,user,time,rss,comm --no-headers 2>/dev/null | while read -r pid user cpu_time rss comm; do
        local rss_kb=0
        rss_kb=$(echo "$rss" | awk '{print $1+0}')
        [ "$rss_kb" -gt 0 ] || rss_kb=0

        local cpu_sec=0
        if [ -n "$cpu_time" ] && [ "$cpu_time" != "TIME" ]; then
            # time format: "MM:SS" or "HH:MM:SS" or "DD-HH:MM:SS"
            cpu_sec=$(echo "$cpu_time" | awk -F'[: -]' '{
                if (NF==2) print $1*60 + $2;
                else if (NF==3) print $1*3600 + $2*60 + $3;
                else if (NF==4) print $1*86400 + $2*3600 + $3*60 + $4;
                else print 0
            }')
        fi

        echo "$pid|$user|$cpu_sec|$rss_kb|$comm|$comm"
    done
}

# --- Sort helpers ------------------------------------------------
sort_procs() {
    local sort_col="$1"
    case "$sort_col" in
        cpu)  sort -t'|' -k3 -nr ;;
        mem)  sort -t'|' -k4 -nr ;;
        pid)  sort -t'|' -k1 -nr ;;
        name) sort -t'|' -k5 ;;
        *)    sort -t'|' -k3 -nr ;;
    esac
}

# --- Display -----------------------------------------------------
print_header() {
    local cpu_pct="$1" total_mb="$2" used_mb="$3" avail_mb="$4"
    local swap_total="$5" swap_used="$6" uptime_str="$7"
    local load1="$8" load5="$9" load15="${10}"

    local mem_pct="0"
    if [ "$total_mb" -gt 0 ]; then
        mem_pct=$(awk -v u="$used_mb" -v t="$total_mb" 'BEGIN { printf "%.1f", 100 * u / t }')
    fi

    clear

    local hostname_str
    hostname_str=$(hostname 2>/dev/null || echo "unknown")

    echo -e "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${CYAN}║${NC}  ${BOLD}Process Monitor (macOS)${NC} — ${hostname_str} — $(date '+%Y-%m-%d %H:%M:%S')"
    echo -e "${BOLD}${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
    printf "${BOLD}${CYAN}║${NC}  CPU: %s  " "$(color_pct "$cpu_pct" "")"
    printf "Mem: %s / %s MB (%s)  " "$used_mb" "$total_mb" "$(color_pct "$mem_pct" "")"
    printf "Swap: %s / %s MB\n" "$swap_used" "$swap_total"
    printf "${BOLD}${CYAN}║${NC}  Load: ${load1} ${load5} ${load15}  "
    printf "Uptime: %s\n" "$uptime_str"
    echo -e "${BOLD}${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
}

print_table_header() {
    printf "${BOLD}${CYAN}║${NC} ${BOLD}%-8s %-10s %-10s %-7s %-15s %s${NC}\n" \
        "PID" "USER" "CPU TIME" "MEM(MB)" "NAME" "COMMAND"
    echo -e "${BOLD}${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
}

print_process_row() {
    local pid="$1" user="$2" cpu_sec="$3" rss_kb="$4" name="$5" cmd="$6"
    local rss_mb
    rss_mb=$(awk -v r="$rss_kb" 'BEGIN { printf "%.1f", r / 1024 }')
    local cpu_fmt
    cpu_fmt=$(printf '%dh%02dm%02ds' $((cpu_sec/3600)) $(((cpu_sec%3600)/60)) $((cpu_sec%60)))

    printf "${CYAN}║${NC} ${DIM}%-8s${NC} %-10s %-10s %-7s %-15s %s\n" \
        "$pid" "$user" "$cpu_fmt" "${rss_mb}" "${name:0:15}" "${cmd:0:45}"
}

# --- Interactive key reader --------------------------------------
read_key() {
    local key
    [ -t 0 ] || return 0
    IFS= read -rsn1 -t "$REFRESH_INTERVAL" key 2>/dev/null || true
    echo "$key"
}

# --- Main loop ---------------------------------------------------
main() {
    # If stdin is not a terminal, run one iteration and exit
    local once=false
    [ -t 0 ] || once=true

    trap 'echo -e "\n${GREEN}Exiting.${NC}"; exit 0' INT TERM

    while true; do
        local cpu_pct
        cpu_pct=$(get_cpu_usage)

        local mem_data total_kb used_kb avail_kb swap_total_kb swap_used_kb
        read -r total_kb used_kb avail_kb swap_total_kb swap_used_kb < <(get_mem_info)

        local total_mb=$((total_kb / 1024))
        local used_mb=$((used_kb / 1024))
        local avail_mb=$((avail_kb / 1024))
        local swap_total=$((swap_total_kb / 1024))
        local swap_used=$((swap_used_kb / 1024))

        local uptime_str load1 load5 load15
        uptime_str=$(get_uptime)
        read -r load1 load5 load15 < <(get_loadavg)

        local procs_raw
        procs_raw=$(get_processes)
        local procs_filtered="$procs_raw"

        if [ -n "$FILTER_NAME" ]; then
            procs_filtered=$(echo "$procs_raw" | grep -i "$FILTER_NAME")
        fi

        local procs_sorted
        procs_sorted=$(echo "$procs_filtered" | sort_procs "$SORT_BY" | head -n "$TOP_COUNT")

        print_header "$cpu_pct" "$total_mb" "$used_mb" "$avail_mb" \
            "$swap_total" "$swap_used" "$uptime_str" "$load1" "$load5" "$load15"

        print_table_header

        if [ -z "$procs_sorted" ]; then
            printf "${CYAN}║${NC}  ${DIM}No processes found.${NC}\n"
        else
            while IFS='|' read -r pid user cpu_sec rss_kb name cmd; do
                print_process_row "$pid" "$user" "$cpu_sec" "$rss_kb" "$name" "$cmd"
            done <<< "$procs_sorted"
        fi

        echo -e "${BOLD}${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
        echo -e "${DIM}[q]uit [c]pu-sort [m]em-sort [p]id-sort [n]ame-sort   interval=${REFRESH_INTERVAL}s${NC}"

        local key
        key=$(read_key)
        case "$key" in
            q|Q) echo -e "\n${GREEN}Exiting.${NC}"; exit 0 ;;
            c|C) SORT_BY="cpu" ;;
            m|M) SORT_BY="mem" ;;
            p|P) SORT_BY="pid" ;;
            n|N) SORT_BY="name" ;;
        esac
        $once && break
    done
}

main
