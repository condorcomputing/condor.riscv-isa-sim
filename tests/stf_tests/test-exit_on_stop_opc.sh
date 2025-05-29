#!/bin/bash

# tests this options:
# --stf_exit_on_stop_opc Terminate the simulation after detecting a
#                          STOP_TRACE opcode. Using this switch
#                          disables non-contiguous region tracing.
#                          (default false)

export SPIKE=$1
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
export SPIKE_OPTS=" --stf_macro_tracing --stf_exit_on_stop_opc"
STATUS=0

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

# Get the instructions in the first start-stop section and make sure we see them in the trace
FIRST_GROUP=$(awk '/START_TRACE/ {flag=1; next} /STOP_TRACE/ {flag=0; exit} flag' $testname_basic.S)
while IFS= read -r instruction; do
	if ! grep -q "$instruction" "$DUMP_OUT"; then
		echo "Expected line '$instruction' missing from $DUMP_OUT" >> $logfile
		STATUS=1
	fi
done <<< "$FIRST_GROUP"

# Get the instructions in the second start-stop section and make sure they're absent
SECOND_GROUP=$(awk '
		/START_TRACE/ { start_count++; if (start_count == 2) in_range = 1; next }
		/STOP_TRACE/ { if (in_range) exit }
		in_range
	' "$testname_basic.S")

while IFS= read -r instruction; do
	if grep -q "$instruction" "$DUMP_OUT"; then
		echo "Unexpected instruction '$instruction' from after the stop macro was found in $DUMP_OUT" >> $logfile
		STATUS=1
	fi
done <<< "$SECOND_GROUP"

if [ $STATUS -ne 0 ]; then
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
fi

# Again with the option off
export SPIKE_OPTS=" --stf_macro_tracing"
source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

# With the exit option off, make sure we see the second group in the trace
while IFS= read -r instruction; do
	if ! grep -q "$instruction" "$DUMP_OUT"; then
		echo "Expected line '$instruction' missing from $DUMP_OUT" >> $logfile
		STATUS=1
	fi
done <<< "$SECOND_GROUP"

if [ $STATUS -ne 0 ]; then
	echo "Spike command:" >> $logfile
	echo $SPIKE_COMMAND >> $logfile
fi
exit $STATUS