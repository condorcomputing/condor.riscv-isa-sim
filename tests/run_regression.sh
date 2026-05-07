#!/bin/bash

# Configuration
SPIKE_EXEC="LD_LIBRARY_PATH=../build ../build/spike"
RV64_OPTS="--isa=rv64imafdc_zicntr_zihpm_zba_zbb_zbc_zbs_zfh_smrnmi"
RV32_OPTS="--isa=rv32imafdc_zicntr_zihpm_zba_zbb_zbc_zbs_zfh_smrnmi"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."
TEST_LIST_FILE="$SCRIPT_DIR/isa_tests_list.txt"
TEST_DIR="$ROOT_DIR/install/share/riscv-tests/isa"
LOG_DIR="$SCRIPT_DIR/logs"
TIMEOUT_DURATION=30

# State
RUN_SINGLE=""
PARALLEL=false
total=0
passed=0
failed=0
disabled=0
running_pids=()

# Tracking
failed_tests=()
timed_out_tests=()
disabled_tests=()

# Ctrl+C handler
cleanup() {
    for pid in "${running_pids[@]}"; do
        kill -9 "$pid" 2>/dev/null
    done
    echo -e "\n[ABORTED] Terminated running tests."
    exit 1
}
trap cleanup SIGINT

# Parse options
while [[ $# -gt 0 ]]; do
    case "$1" in
        --test)
            RUN_SINGLE="$2"
            shift 2
            ;;
        --parallel)
            PARALLEL=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--test <testname>] [--parallel]"
            exit 1
            ;;
    esac
done

# Prepare logs
rm -rf "$LOG_DIR"
mkdir -p "$LOG_DIR"

# Core test runner
run_test() {
    local testname="$1"
    local elf="$TEST_DIR/$testname"
    local logfile="$LOG_DIR/$testname.log"

    local opts=""
    if [[ "$testname" == rv32* ]]; then
        opts="$RV32_OPTS"
    elif [[ "$testname" == rv64* ]]; then
        opts="$RV64_OPTS"
    else
        echo "[FAIL]  $testname"
        echo "Unknown ISA for test: $testname" > "$logfile"
        failed_tests+=("$testname")
        return 1
    fi

    if [[ ! -f "$elf" ]]; then
        echo "[FAIL]  $testname"
        echo "ELF not found: $elf" > "$logfile"
        failed_tests+=("$testname")
        return 1
    fi

    local cmd="$SPIKE_EXEC $opts $elf"
    timeout "$TIMEOUT_DURATION"s bash -c "$cmd" > "$logfile" 2>&1 &
    local pid=$!
    running_pids+=("$pid")
    wait $pid
    local status=$?
    running_pids=()

    if [[ $status -eq 0 ]]; then
        echo "[PASS]  $testname"
        return 0
    elif [[ $status -eq 124 ]]; then
        echo "[FAIL]  $testname"
        timed_out_tests+=("$testname")
        return 1
    else
        echo "[FAIL]  $testname"
        failed_tests+=("$testname")
        return 1
    fi
}

# Run single test
if [[ -n "$RUN_SINGLE" ]]; then
    run_test "$RUN_SINGLE"
    exit $?
fi

# Read test list
enabled_tests=()
while IFS= read -r line; do
    line="$(echo "$line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
    [[ -z "$line" || "$line" =~ ^# ]] && continue
    if [[ "$line" =~ ^x[[:space:]]+(.+) ]]; then
        testname="${BASH_REMATCH[1]}"
        disabled_tests+=("$testname")
        ((disabled++))
    else
        enabled_tests+=("$line")
    fi
done < "$TEST_LIST_FILE"

# Run all tests
if $PARALLEL; then
    printf "%s\n" "${enabled_tests[@]}" | \
        xargs -n1 -P$(nproc) -I{} bash -c "cd \"$SCRIPT_DIR\" && ./run-regression.sh --test {}" | \
        while read result_line; do
            [[ "$result_line" =~ \[PASS\] ]] && ((passed++)) || ((failed++))
            ((total++))
            echo "$result_line"
        done
else
    for testname in "${enabled_tests[@]}"; do
        ((total++))
        if run_test "$testname"; then
            ((passed++))
        else
            ((failed++))
        fi
    done
fi

# ========================================================================
# Run STF tests
pushd $(dirname $0)/stf_tests > /dev/null
for script in test-*.sh; do
    testname_basic="${script#test-}"
    testname_basic="${testname_basic%.sh}"
    export STF_EXT=stf
    testname_full="${testname_basic}_uncompressed"
    if ./$script ../../build/spike $testname_basic $testname_full; then
        ((passed++))
        echo "[PASS]  $testname_full"
    else
        ((failed++))
        echo "[FAIL]  $testname_full"
        failed_tests+=("$testname_full")
    fi
    ((total++))
    export STF_EXT=zstf
    testname_full="${testname_basic}_compressed"
    if ./$script ../../build/spike $testname_basic $testname_full; then
        ((passed++))
        echo "[PASS]  $testname_full"
    else
        ((failed++))
        echo "[FAIL]  $testname_full"
        failed_tests+=("$testname_full")
    fi
    ((total++))
done
popd > /dev/null

# Summary
echo "===================================================="
echo "Test Summary:"
echo "  Total tests run   : $total"
echo "  Tests passed      : $passed"
echo "  Tests failed      : $failed"
echo "  Tests disabled    : $disabled"
echo "Log files written to: $LOG_DIR"

# Write summary.log
{
    echo "Summary:"
    echo "  Total tests run   : $total"
    echo "  Tests passed      : $passed"
    echo "  Tests failed      : $failed"
    echo "  Tests disabled    : $disabled"
    echo ""

    echo "Failing tests:"
    for test in "${failed_tests[@]}"; do
        echo "  $test"
    done
    echo ""

    echo "Tests that timed out (${TIMEOUT_DURATION}s):"
    for test in "${timed_out_tests[@]}"; do
        echo "  $test"
    done
    echo ""

    echo "Disabled tests:"
    for test in "${disabled_tests[@]}"; do
        echo "  $test"
    done
} > "$LOG_DIR/summary.log"

echo "Summary log written to: $LOG_DIR/summary.log"

if [[ $failed -eq 0 ]]; then
    exit 0
else
    exit 1
fi
