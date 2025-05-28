#!/bin/bash

source test_common.sh

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

# Define expected instructions for each mode (see privmodes.S)
expected_M="add.*x0,x1,x2"
expected_H="add.*x0,x2,x2"
expected_S="add.*x0,x3,x2"
expected_U="add.*x0,x4,x2"

# Check for expected instructions and print error messages
check_trace_dump()
{
        for letter in M H S U; do
            # grab the correct expected_ variable
            expected_var="expected_$letter"
            pattern=${!expected_var}

            if [[ $mode == *"$letter"* ]]; then
                # we expect a trace for this mode
                if ! grep -q "$pattern" "$DUMP_OUT"; then
                    echo "Instruction trace ($pattern) not found from mode $letter" >> "$logfile"
                    STATUS=1
		#else
	        #    echo "Expected $letter trace $pattern, and found it..." >> $logfile
		fi

                # Get the address of a traced instruction from the objdump, and check that
                INST_TO_MATCH=$pattern
		if ! check_inst_trace_addr $INST_TO_MATCH $DUMP_OUT $RISCV_BIN $testname_basic; then
                  STATUS=1
		#else
		#   echo "Found traced instruction at expected address ${address}"
                fi
            else
                # we do _not_ expect a trace here
                if grep -q -- "$pattern" "$DUMP_OUT"; then
                    echo "Instruction trace found from mode $letter. Expected traces from mode(s): $mode" >> "$logfile"
                    STATUS=1
		#else
	        #    echo "Didn't expect to see $letter trace $pattern, and didn't find it..." >> $logfile
		fi
            fi
        done
}

# To make sure we cover all the combinations of the 4 modes, use the bits of numbers 1-15
for i in {1..15}; do
	mode=""
	((i & 8)) && mode+="M"
	((i & 4)) && mode+="H"
	((i & 2)) && mode+="S"
	((i & 1)) && mode+="U"
	export SPIKE_OPTS="--stf_macro_tracing --stf_priv_modes $mode"

	source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
	if [ $? -eq 0 ]; then
		#echo "Checking trace file for mode string $mode..." >> $logfile
		check_trace_dump
	else
		STATUS=1
	fi

	if [ $STATUS -ne 0 ]; then
		echo "Spike command:" >> $logfile
		echo $SPIKE_COMMAND >> $logfile
	fi
done

export SPIKE_OPTS="--stf_insn_num_tracing --stf_insn_start 0"

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1

if [ $? -eq 0 ]; then
	#echo "Checking trace file for mode string $mode..." >> $logfile
        INST_TO_MATCH=$expected_U
	if ! check_inst_trace_addr $INST_TO_MATCH $DUMP_OUT $RISCV_BIN $testname_basic; then
          STATUS=1
	#else
	#   echo "Found traced instruction at expected address ${address}"
        fi
else
	STATUS=1
fi

exit $STATUS
