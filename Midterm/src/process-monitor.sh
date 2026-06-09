#!/usr/bin/env bash
set -euo pipefail

# ================================================================
# process-monitor.sh — Linux Process Monitor
# ================================================================
# Reads /proc filesystem to display real-time system & process info.
# Usage: ./process-monitor.sh [-n N] [-s col] [-f name] [-k pid] [-i interval]
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
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

# --- Helper functions --------------------------------------------
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
${BOLD}process-monitor.sh${NC} v${VERSION} — Linux real-time process monitor

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
  $SCRIPT_NAME -s cpu -n 5 -i 1       # Top 5 by CPU, refresh every second
  $SCRIPT_NAME -f firefox             # Show only firefox processes
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
        v) echo "process-monitor.sh v${VERSION}"; exit 0 ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

if ! [[ "$TOP_COUNT" =~ ^[0-9]+$ ]] || [ "$TOP_COUNT" -lt 1 ]; then
    die "Invalid top count: $TOP_COUNT (must be positive integer)"
fi

if ! [[ "$REFRESH_INTERVAL" =~ ^[0-9]+\.?[0-9]*$ ]]; then
    die "Invalid interval: $REFRESH_INTERVAL (must be a number)"
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

# --- System info collectors --------------------------------------

get_cpu_usage() {
    local cpu_line
    cpu_line=$(grep '^cpu ' /proc/stat)
    read -r _ user nice system idle iowait irq softirq steal _ <<< "$cpu_line"
    local total=$((user + nice + system + idle + iowait + irq + softirq + steal))
    local idle_total=$((idle + iowait))
    echo "$total $idle_total"
}

# Returns CPU usage as a percentage (requires two calls)
read_cpu_pct() {
    local prev_total prev_idle
    read -r prev_total prev_idle < <(get_cpu_usage)
    sleep 0.15  # Small delay for delta
    local curr_total curr_idle
    read -r curr_total curr_idle < <(get_cpu_usage)

    local total_diff=$((curr_total - prev_total))
    local idle_diff=$((curr_idle - prev_idle))

    if [ "$total_diff" -eq 0 ]; then
        echo "0.0"
    else
        awk -v td="$total_diff" -v id="$idle_diff" 'BEGIN { printf "%.1f", 100 * (td - id) / td }'
    fi
}

get_mem_info() {
    local total_kb=0 free_kb=0 avail_kb=0 buffers_kb=0 cached_kb=0 swap_total=0 swap_free=0

    while read -r key val unit; do
        case "$key" in
            MemTotal:)     total_kb=$val ;;
            MemFree:)      free_kb=$val ;;
            MemAvailable:) avail_kb=$val ;;
            Buffers:)      buffers_kb=$val ;;
            Cached:)       cached_kb=$val ;;
            SwapTotal:)    swap_total=$val ;;
            SwapFree:)     swap_free=$val ;;
        esac
    done < /proc/meminfo

    local used_kb=$((total_kb - avail_kb))
    local used_swap=$((swap_total - swap_free))
    echo "$total_kb $used_kb $avail_kb $swap_total $used_swap"
}

get_uptime() {
    local uptime_sec
    read -r uptime_sec _ < /proc/uptime
    uptime_sec=${uptime_sec%%.*}
    local d=$(( uptime_sec / 86400 ))
    local h=$(( (uptime_sec % 86400) / 3600 ))
    local m=$(( (uptime_sec % 3600) / 60 ))
    printf '%dd %dh %dm' "$d" "$h" "$m"
}

get_loadavg() {
    read -r load1 load5 load15 _ _ < /proc/loadavg
    echo "$load1 $load5 $load15"
}

# Read processes from /proc
get_processes() {
    local procs=()
    for pid_dir in /proc/[0-9]*; do
        [ -d "$pid_dir" ] || continue
        local pid="${pid_dir##*/}"
        [ -r "$pid_dir/cmdline" ] || continue

        # Read process name (comm)
        local name=""
        if [ -r "$pid_dir/comm" ]; then
            name=$(tr -d '\0' < "$pid_dir/comm" 2>/dev/null)
        fi

        # Read command line
        local cmd=""
        if [ -r "$pid_dir/cmdline" ]; then
            cmd=$(tr '\0' ' ' < "$pid_dir/cmdline" 2>/dev/null)
            [ -n "$cmd" ] || cmd="[$name]"
            cmd="${cmd:0:80}"  # Truncate
        fi

        # Read stat
        local stat_data=""
        [ -r "$pid_dir/stat" ] && stat_data=$(<"$pid_dir/stat")

        # Parse utime, stime, cutime, cstime from stat (fields 14-17)
        local utime=0 stime=0 rss=0
        if [ -n "$stat_data" ]; then
            # Fields up to comm may contain spaces; extract from closing paren
            local after_paren="${stat_data##*)}"
            read -r state _ _ _ _ _ _ _ _ _ _ utime stime cutime cstime _ _ _ _ _ _ rss _ <<< "$after_paren"
        fi

        # Calculate total CPU time (user + system + child user + child system)
        local cpu_time=$((utime + stime + cutime + cstime))

        # rss is in pages; convert to KB (pages are typically 4KB)
        local rss_kb=$((rss * 4))

        # Read UID and username
        local uid=""
        local user=""
        if [ -r "$pid_dir/status" ]; then
            uid=$(awk '/^Uid:/{print $2}' "$pid_dir/status" 2>/dev/null)
            user=$(getent passwd "$uid" 2>/dev/null | cut -d: -f1 || echo "$uid")
        fi

        echo "$pid|$user|$cpu_time|$rss_kb|$name|$cmd"
    done
}

# --- Display -----------------------------------------------------

print_header() {
    local cpu_pct="$1"
    local total_mb="$2" used_mb="$3" avail_mb="$4"
    local swap_total="$5" swap_used="$6"
    local uptime_str="$7"
    local load1="$8" load5="$9" load15="${10}"

    local mem_pct="0"
    if [ "$total_mb" -gt 0 ]; then
        mem_pct=$(awk -v u="$used_mb" -v t="$total_mb" 'BEGIN { printf "%.1f", 100 * u / t }')
    fi

    clear

    local hostname_str
    hostname_str=$(hostname 2>/dev/null || echo "unknown")

    echo -e "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${CYAN}║${NC}  ${BOLD}Process Monitor${NC} — ${hostname_str} — $(date '+%Y-%m-%d %H:%M:%S')"
    echo -e "${BOLD}${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
    printf "${BOLD}${CYAN}║${NC}  CPU: %s  " "$(color_pct "$cpu_pct" "")"
    printf "Mem: %s / %s MB (%s)  " "$used_mb" "$total_mb" "$(color_pct "$mem_pct" "")"
    printf "Swap: %s / %s MB\n" "$swap_used" "$swap_total"
    printf "${BOLD}${CYAN}║${NC}  Load: ${load1} ${load5} ${load15}  "
    printf "Uptime: %s  " "$uptime_str"
    printf "Procs: %s\n" "$(ls -d /proc/[0-9]* 2>/dev/null | wc -l)"
    echo -e "${BOLD}${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
}

print_table_header() {
    printf "${BOLD}${CYAN}║${NC} ${BOLD}%-8s %-10s %-10s %-7s %-15s %s${NC}\n" \
        "PID" "USER" "CPU% (t)" "MEM(MB)" "NAME" "COMMAND"
    echo -e "${BOLD}${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
}

print_process_row() {
    local pid="$1" user="$2" cpu_sec="$3" rss_kb="$4" name="$5" cmd="$6"

    # Display CPU time accumulated rather than %
    local rss_mb=$(awk -v r="$rss_kb" 'BEGIN { printf "%.1f", r / 1024 }')
    local cpu_fmt
    cpu_fmt=$(printf '%dh%02dm%02ds' $((cpu_sec/3600)) $(((cpu_sec%3600)/60)) $((cpu_sec%60)))

    printf "${CYAN}║${NC} ${DIM}%-8s${NC} %-10s %-10s %-7s %-15s %s\n" \
        "$pid" "$user" "$cpu_fmt" "${rss_mb}" "${name:0:15}" "${cmd:0:45}"
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

# --- Interactive key reader --------------------------------------
# (for single-char input without Enter: uses `stty` trick)

read_key() {
    local key
    [ -t 0 ] || return 0
    IFS= read -rsn1 -t "$REFRESH_INTERVAL" key 2>/dev/null || true
    echo "$key"
}

# --- Main loop ---------------------------------------------------
main() {
    local total_cpu_sec=0
    local prev_total_cpu=0
    local prev_pid_times=""

    # If stdin is not a terminal, run one iteration and exit
    local once=false
    [ -t 0 ] || once=true

    # Trap SIGINT for clean exit
    trap 'echo -e "\n${GREEN}Exiting.${NC}"; exit 0' INT TERM

    while true; do
        # --- Collect system info ---
        local cpu_pct
        cpu_pct=$(read_cpu_pct)

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

        # --- Collect process data ---
        local procs_raw
        procs_raw=$(get_processes)
        local procs_filtered="$procs_raw"

        # Apply filter if set
        if [ -n "$FILTER_NAME" ]; then
            procs_filtered=$(echo "$procs_raw" | grep -i "$FILTER_NAME")
        fi

        # Sort, take top N
        local procs_sorted
        procs_sorted=$(echo "$procs_filtered" | sort_procs "$SORT_BY" | head -n "$TOP_COUNT")

        # Calculate total CPU seconds across procs for percentage
        total_cpu_sec=$(echo "$procs_filtered" | awk -F'|' '{sum+=$3}END{print sum}')

        # --- Render ---
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

        # --- Handle keyboard input ---
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
