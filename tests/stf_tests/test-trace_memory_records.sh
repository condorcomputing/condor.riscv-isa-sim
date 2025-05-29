#!/bin/bash

# tests this option:
#   --stf_trace_memory_records
#                          Include memory records in the STF trace.
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
STATUS=0

export SPIKE_OPTS="--stf_trace_memory_records --stf_macro_tracing"

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

if ! grep -q "MEM WRITE.*12345678" $DUMP_OUT; then
	echo "$testname_basic: $DUMP_OUT does not contain the expected MEM_WRITE" >> $logfile
	STATUS=1
fi

if ! grep -q "MEM READ.*12345678" $DUMP_OUT; then
	echo "$testname_basic: $DUMP_OUT does not contain the expected MEM_READ" >> $logfile
	STATUS=1
fi

if [ $STATUS -ne 0 ]; then
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
fi
exit $STATUS