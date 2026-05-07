#!/bin/bash

source test_common.sh

# tests this option:
#   --stf_trace_register_state
#                          Include changes to register state through 
#                          instruction execution in the STF output.
#                          (default false)
#                          Note: changes in control flow emit full
#                          register state regardless of this setting.

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
export SPIKE_OPTS="--stf_trace_register_state --stf_macro_tracing"
STATUS=0

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

for i in {1..31}; do
	expected_hex=$(printf "%08x%08x" $i $i)
	if ! grep -q "state REG_$i\s\+$expected_hex" "$DUMP_OUT"; then
		echo "$testname_basic: Missing or incorrect state for REG_$i in $DUMP_OUT, expected $expected_hex" >> $logfile
		STATUS=1
	fi

	expected_hex=$(printf "%08x%08x" $i $i)
	if ! grep -q "state REG_F$i\s\+$expected_hex" "$DUMP_OUT"; then
		echo "$testname_basic: Missing or incorrect state for REG_F$i in $DUMP_OUT, expected $expected_hex" >> $logfile
		STATUS=1
	fi

	expected_hex=$(printf "%08x%08x" 0xc001c0de $i)
	if ! grep -q "dst REG_$i\s\+$expected_hex" "$DUMP_OUT"; then
		echo "$testname_basic: Missing or incorrect state for REG_$i in $DUMP_OUT, expected $expected_hex" >> $logfile
		STATUS=1
	fi

	expected_hex=$(printf "%08x%08x" 0xc001c0de $i)
	if ! grep -q "dst REG_F$i\s\+$expected_hex" "$DUMP_OUT"; then
		echo "$testname_basic: Missing or incorrect state for REG_F$i in $DUMP_OUT, expected $expected_hex" >> $logfile
		STATUS=1
	fi
done

# Get the address of a traced instruction from the objdump, and check that
# the traced address matches:
INST_TO_MATCH="06f6869b"
if ! check_inst_trace_addr $INST_TO_MATCH $DUMP_OUT $RISCV_BIN $testname_basic; then
	STATUS=1
fi

if [ $STATUS -ne 0 ]; then
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
fi
exit $STATUS
