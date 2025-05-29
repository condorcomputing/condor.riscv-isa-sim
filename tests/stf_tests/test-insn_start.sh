#!/bin/bash

# tests these options:
#   --stf_insn_num_tracing Enable STF tracing on instruction count.
#                          stf_macro_tracing and stf_insn_num_tracing
#                          are exclusive.
#                          (default false)
#   --stf_insn_start <N>   Start STF tracing after N instructions.

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

logfile=$(dirname "$0")/../logs/stf_$testname_full.log
: > $logfile
export SPIKE=$1
STATUS=0

# Get a trace starting with the 6th instruction. Instruction numbers 2 and 5 are specially designated
# and handled differently in spike.
INSN_START_IDX=6
INSN_EXPECTED_COUNT=10
export SPIKE_OPTS="--stf_insn_num_tracing --stf_insn_start $INSN_START_IDX --stf_insn_count $INSN_EXPECTED_COUNT"
source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi

# Now increment the start number in a loop and check that the first instruction in each trace is the expected one
for INSN_START_IDX in 7 8 9 10 11 12; do
	# Get the second instruction line from the previous trace, which should be the first one in the next trace
	NEXT_INSN_LINE=$(awk '/^(INST16|INST32)/ {getline; print; exit}' $DUMP_OUT)
	# We need to omit the instruction number (2, in this case) because it will be 1 in the next iteration.
	# Parse NEXT_INSN_LINE into whitespace-separated tokens, take the ones at indices 1, 3, 4 5, and 6, 
	# and put them into a new variable separated by single spaces
	INSN_EXPECTED_START=$(echo "$NEXT_INSN_LINE" | awk '{print $1, $3, $4, $5, $6}')
	export SPIKE_OPTS="--stf_insn_num_tracing --stf_insn_start $INSN_START_IDX --stf_insn_count $INSN_EXPECTED_COUNT"
	source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
	if [ $? -ne 0 ]; then
		STATUS=1
	fi

	# Get the first instruction line from the new trace and parse as before
	FIRST_INSN_LINE=$(awk '/^(INST16|INST32)/ {print; exit}' $DUMP_OUT)
	INSN_ACTUAL_START=$(echo "$FIRST_INSN_LINE" | awk '{print $1, $3, $4, $5, $6}')
	if [ "$INSN_ACTUAL_START" != "$INSN_EXPECTED_START" ]; then
		echo "$0: expected instruction trace to start at $INSN_EXPECTED_START but found $INSN_ACTUAL_START" >> $logfile
		echo "Spike command:" >> $logfile
		echo $SPIKE_COMMAND >> $logfile
		STATUS=1
	fi
done

if [ $STATUS -ne 0 ]; then
	echo "$0: FAILED" >> $logfile
fi
exit $STATUS