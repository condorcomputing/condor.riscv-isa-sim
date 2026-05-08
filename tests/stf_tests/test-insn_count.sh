#!/bin/bash

# tests these options:
#   --stf_insn_num_tracing Enable STF tracing on instruction count.
#                          stf_macro_tracing and stf_insn_num_tracing
#                          are exclusive.
#                          (default false)
#   --stf_insn_count <N>   Terminate STF tracing after N instructions
#                          from stf_insn_start.

if [ -n "$2" ]; then
	testname_basic=$2
else
	testname_basic="${0#./test-}"
    testname_basic="${testname_basic%.sh}"
fi

if [ -n "$3" ]; then
	testname_full=$3
else
	testname_full=$testname_basic
fi

source test_common.sh

logfile=$(dirname "$0")/../logs/stf_$testname_full.log
: > $logfile
export SPIKE=$1
STATUS=0

test_insn_trace_count() {
  local STATUS=0

  source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1

  if [ $? -ne 0 ]; then
    STATUS=1
  fi

  # Count lines starting with "INST16" or "INST32" in the dump file
  INSN_ACTUAL_COUNT=$(awk '/^(INST16|INST32)/ {count++} END {print count}' $DUMP_OUT)
  if (( $INSN_ACTUAL_COUNT != $INSN_EXPECTED_COUNT )); then
    echo "$0: expected $INSN_EXPECTED_COUNT opcodes in instruction trace but found $INSN_ACTUAL_COUNT" >> $logfile
    echo "Spike command:" >> $logfile
    echo $SPIKE_COMMAND >> $logfile
    STATUS=1
  fi

  return $STATUS
}

# Test that we get the expected number of instructions in the trace (--stf_insn_count)
for INSN_START_IDX in 0 1 2 12; do
   for INSN_EXPECTED_COUNT in 1 2 3 4 5 50 70; do
	export SPIKE_OPTS="--stf_insn_num_tracing --stf_insn_start $INSN_START_IDX --stf_insn_count $INSN_EXPECTED_COUNT"
	if ! test_insn_trace_count; then
          STATUS=1
	fi
   done
done

# Test with -l and --log (CAWS-36)
for INSN_START_IDX in 0; do
   for INSN_EXPECTED_COUNT in 1; do
        tracelog=$TEMP_OUTPUT_DIR/$testname_full.trace.log
        export SPIKE_OPTS="--stf_insn_num_tracing --stf_insn_start $INSN_START_IDX --stf_insn_count $INSN_EXPECTED_COUNT -l --log=${tracelog}"
	if ! test_insn_trace_count; then
          STATUS=1
	fi
   done
done

# Get the address of a traced instruction from the objdump, and check that
# the traced address matches:
INSN_START_IDX=85
export SPIKE_OPTS="--stf_insn_num_tracing --stf_insn_start $INSN_START_IDX"
source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
INST_TO_MATCH="00208033"
if ! check_inst_trace_addr $INST_TO_MATCH $DUMP_OUT $RISCV_BIN $testname_basic; then
	STATUS=1
fi

if [ $STATUS -ne 0 ]; then
	echo "$0: FAILED" >> $logfile
fi
exit $STATUS
