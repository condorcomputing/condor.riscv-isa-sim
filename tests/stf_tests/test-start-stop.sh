#!/bin/bash

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
export SPIKE_OPTS="--stf_macro_tracing"
STATUS=0

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi

# Get the instructions between our start and stop macros and make sure we see them in the trace
EXPECTED_LINES=$(sed -n '/START_TRACE/,/STOP_TRACE/{/START_TRACE/b;/STOP_TRACE/b;p}' $testname_basic.S)

while IFS= read -r instruction; do
	if ! grep -q "$instruction" "$DUMP_OUT"; then
		echo "Expected line '$instruction' missing from $DUMP_OUT" >> $logfile
		STATUS=1
	fi
done <<< "$EXPECTED_LINES"

# Get instructions before the start macro and make sure they're absent
BEFORE_LINES=$(sed '/START_TRACE/,$d' $testname_basic.S)
while IFS= read -r instruction; do
	if grep -q "$instruction" "$DUMP_OUT"; then
		echo "Unexpected instruction '$instruction' from before the start macro was found in $DUMP_OUT" >> $logfile
		STATUS=1
	fi
done <<< "$BEFORE_LINES"

# Get instructions after the stop macro and make sure they're absent
AFTER_LINES=$(sed -n '/STOP_TRACE/,$p' $testname_basic.S | sed '1d')
while IFS= read -r instruction; do
	if grep -q "$instruction" "$DUMP_OUT"; then
		echo "Unexpected instruction '$instruction' from after the stop macro was found in $DUMP_OUT" >> $logfile
		STATUS=1
	fi
done <<< "$AFTER_LINES"

if [ $STATUS -ne 0 ]; then
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
fi
exit $STATUS