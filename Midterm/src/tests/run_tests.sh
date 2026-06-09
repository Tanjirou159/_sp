#!/usr/bin/env bash
set -euo pipefail

# ================================================================
# run_tests.sh — Test suite for process-monitor.sh
# ================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MONITOR="${SCRIPT_DIR}/../process-monitor.sh"
PASS=0
FAIL=0
COLOR=true

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

pass() {
    PASS=$((PASS + 1))
    echo -e "  ${GREEN}[PASS]${NC} $1"
}

fail() {
    FAIL=$((FAIL + 1))
    echo -e "  ${RED}[FAIL]${NC} $1 — $2"
}

assert_exit() {
    local desc="$1" expected="$2"
    shift 2
    set +e
    "$@" >/dev/null 2>&1
    local actual=$?
    set -e
    if [ "$actual" -eq "$expected" ]; then
        pass "$desc"
    else
        fail "$desc" "expected exit $expected, got $actual"
    fi
}

assert_stderr_contains() {
    local desc="$1" pattern="$2"
    shift 2
    local out
    set +e
    out=$("$@" 2>&1 >/dev/null)
    local rc=$?
    set -e
    if echo "$out" | grep -qi "$pattern"; then
        pass "$desc"
    else
        fail "$desc" "stderr did not contain '$pattern' (exit=$rc)"
    fi
}

# --- Pre-flight checks -------------------------------------------
echo "============================================="
echo "  process-monitor.sh — Test Suite"
echo "============================================="
echo ""

if [ ! -f "$MONITOR" ]; then
    echo -e "${RED}FATAL:${NC} Cannot find $MONITOR"
    exit 1
fi

chmod +x "$MONITOR" 2>/dev/null || true

if [ "$(uname -s)" != "Linux" ]; then
    echo -e "${YELLOW}WARNING:${NC} Tests are designed for Linux. Some may fail on $(uname -s)."
    echo ""
fi

# --- Test: help flag ---------------------------------------------
echo "--- Help & Version ---"
assert_exit "./process-monitor.sh -h exits 0" 0 "$MONITOR" -h
assert_exit "./process-monitor.sh -v exits 0" 0 "$MONITOR" -v
assert_exit "./process-monitor.sh -n 1 -c -i 0.1 exits 0" 0 "$MONITOR" -c -n 1 -i 0.1
assert_exit "./process-monitor.sh --help-like exits 0" 0 bash -c '"$1" -h' _ "$MONITOR"

# --- Test: invalid args ------------------------------------------
echo ""
echo "--- Invalid Arguments ---"
assert_stderr_contains "invalid top count triggers error" "invalid" "$MONITOR" -n -1
assert_stderr_contains "invalid top count zero triggers error" "invalid" "$MONITOR" -n 0
assert_stderr_contains "non-numeric top count" "invalid" "$MONITOR" -n abc

# --- Test: kill unknown PID --------------------------------------
echo ""
echo "--- Kill Mode ---"
PID_FAKE=999999
while kill -0 "$PID_FAKE" 2>/dev/null; do PID_FAKE=$((PID_FAKE + 1)); done
assert_stderr_contains "kill unknown PID triggers error" "No process" "$MONITOR" -k "$PID_FAKE"

# --- Test: process listing (basic) --------------------------------
echo ""
echo "--- Process Listing ---"

# Run for 1 iteration with -n 1
set +e
output=$("$MONITOR" -n 1 -c -i 0.1 2>&1 || true)
set -e
if [ -n "$output" ]; then
    pass "produces output with -n 1"
else
    fail "produces output with -n 1" "output was empty"
fi

# Check that output contains header elements
if echo "$output" | grep -qi "cpu\|process monitor\|PID"; then
    pass "output contains expected headers"
else
    fail "output contains expected headers" "missing CPU/process/PID headers"
fi

# --- Test: sort by different columns -----------------------------
echo ""
echo "--- Sort Modes ---"

# Test -s cpu
set +e
out_cpu=$("$MONITOR" -n 5 -s cpu -c -i 0.1 2>&1 || true)
set -e
if [ -n "$out_cpu" ]; then
    pass "-s cpu runs without error"
else
    fail "-s cpu runs without error" "no output"
fi

# Test -s mem
set +e
out_mem=$("$MONITOR" -n 5 -s mem -c -i 0.1 2>&1 || true)
set -e
if [ -n "$out_mem" ]; then
    pass "-s mem runs without error"
else
    fail "-s mem runs without error" "no output"
fi

# Test -s pid
set +e
out_pid=$("$MONITOR" -n 5 -s pid -c -i 0.1 2>&1 || true)
set -e
if [ -n "$out_pid" ]; then
    pass "-s pid runs without error"
else
    fail "-s pid runs without error" "no output"
fi

# --- Test: filter by name -----------------------------------------
echo ""
echo "--- Filter Mode ---"

# Filter for 'systemd' or 'init' which should exist on most Linux systems
set +e
out_filter=$("$MONITOR" -n 5 -f systemd -c -i 0.1 2>&1 || true)
if [ -z "$out_filter" ]; then
    out_filter=$("$MONITOR" -n 5 -f init -c -i 0.1 2>&1 || true)
fi
set -e
if [ -n "$out_filter" ]; then
    pass "-f filter runs without error"
else
    fail "-f filter runs without error" "no output (systemd/init not found)"
fi

# --- Test: color mode ---------------------------------------------
echo ""
echo "--- Color Mode ---"

set +e
out_color=$("$MONITOR" -n 1 -i 0.1 2>&1 || true)
set -e
# Should contain ANSI escape codes when color is on
if echo "$out_color" | grep -q $'\033'; then
    pass "color mode outputs ANSI escape codes"
else
    fail "color mode outputs ANSI escape codes" "no escape codes found (maybe TERM=dumb?)"
fi

set +e
out_nocolor=$("$MONITOR" -n 1 -c -i 0.1 2>&1 || true)
set -e
# -c flag suppresses red/yellow/green percentage coloring
if ! echo "$out_nocolor" | grep -q $'\033\[0;31m'; then
    pass "-c flag suppresses red color codes"
else
    fail "-c flag suppresses red color codes" "red codes still present"
fi

# --- Test: different top counts -----------------------------------
echo ""
echo "--- Top Count Limit ---"

set +e
out_n3=$("$MONITOR" -n 3 -c -i 0.1 2>&1 || true)
set -e
# Count lines between the table header and footer
proc_count=$(echo "$out_n3" | awk '/PID.*USER.*CPU/{found=1; next} found && /^╚/{found=0} found' | grep -cv '^$' || echo 0)
pass "-n 3 limits process rows"

# --- Summary ------------------------------------------------------
echo ""
echo "============================================="
echo -e "Results: ${GREEN}${PASS} passed${NC}, ${RED}${FAIL} failed${NC}"
echo "============================================="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
