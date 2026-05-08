#!/bin/bash

source test_common.sh

# tests this option:
#   --stf_include_macros   Include the trace macros in the trace.
#                          These are:  START:  xor x0,x0,x0
#                                      STOP:   xor x0,x1,x1
#                          (default false)

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
export SPIKE_OPTS="--stf_macro_tracing --stf_include_macros"
STATUS=0

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

# Get the address of a traced instruction from the objdump, and check that
# the traced address matches:
INST_TO_MATCH="00208033"
if ! check_inst_trace_addr $INST_TO_MATCH $DUMP_OUT $RISCV_BIN $testname_basic; then
	STATUS=1
fi

if ! grep -q "xor.*x0,x0,x0" $DUMP_OUT; then
	echo "$testname_basic: $DUMP_OUT does not contain the expected START_TRACE statement" >> $logfile
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
	STATUS=1
fi

if ! grep -q "xor.*x0,x1,x1" $DUMP_OUT; then
	echo "$testname_basic: $DUMP_OUT does not contain the expected STOP_TRACE statement" >> $logfile
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
	STATUS=1
fi

# With the option turned off, the macros should not be included in the trace.
export SPIKE_OPTS="--stf_macro_tracing"

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

# Get the address of a traced instruction from the objdump, and check that
# the traced address matches:
INST_TO_MATCH="00208033"
if ! check_inst_trace_addr "$INST_TO_MATCH" "$DUMP_OUT" "$RISCV_BIN" "$testname_basic"; then
	STATUS=1
fi

if grep -q "xor.*x0,x0,x0" $DUMP_OUT; then
	echo "$testname_basic: $DUMP_OUT contains the START_TRACE statement when it should not" >> $logfile
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
	STATUS=1
fi

if grep -q "xor.*x0,x1,x1" $DUMP_OUT; then
	echo "$testname_basic: $DUMP_OUT contains the STOP_TRACE statement when it should not" >> $logfile
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
	STATUS=1
fi

exit $STATUS
