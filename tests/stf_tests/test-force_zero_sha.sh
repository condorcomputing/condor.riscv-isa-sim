#!/bin/bash

# tests this option:
#   --stf_force_zero_sha   Emit 0 for all SHA's in the STF header.
#                          For regression and other testing purposes
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
export SPIKE_OPTS="--stf_macro_tracing --stf_force_zero_sha"
STATUS=0

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

# The .dump file (from stf_dump) should contain a SHA of zero for the spike repository
SPIKE_SHA_COMMENT=$(grep -o "SPIKE SHA:\s*[^ ]*" "$DUMP_OUT")
if [ -z "$SPIKE_SHA_COMMENT" ]; then
	echo "$testname_basic (with --stf_force_zero_sha): no comment present for Spike SHA (zero or otherwise) in $DUMP_OUT" >> $logfile
else
	SHA_IN_DUMP=$(echo "$SPIKE_SHA_COMMENT" | sed 's/SPIKE SHA:\s*//')
	if [ "$SHA_IN_DUMP" != "0" ]; then
		echo "$testname_basic (with --stf_force_zero_sha): SHA of Spike repository in $DUMP_OUT is \"$SHA_IN_DUMP\"" >> $logfile
		echo "SHA should be 0" >> $logfile
		echo "Spike command:" >> $logfile
		echo $SPIKE_COMMAND >> $logfile
		STATUS=1
	fi
fi

# The .rdump file (from stf_record_dump) should contain a SHA of zero for the stf_lib submodule
STF_SHA_COMMENT=$(grep -o "SPIKE SHA:\s*[^ ]*" "$RECORD_DUMP_OUT")
if [ -z "$STF_SHA_COMMENT" ]; then
	echo "$testname_basic (with --stf_force_zero_sha): no comment present for stf_lib submodule SHA (zero or otherwise) in $RDUMP_OUT" >> $logfile
else
	SHA_IN_DUMP=$(echo "$STF_SHA_COMMENT" | sed 's/.*SHA:\s*//')
	if [ "$SHA_IN_DUMP" != "0" ]; then
		echo "$testname_basic (with --stf_force_zero_sha): SHA of stf_lib submodule in $RECORD_DUMP_OUT is \"$SHA_IN_DUMP\"" >> $logfile
		echo "SHA should be 0" >> $logfile
		echo "Spike command:" >> $logfile
		echo $SPIKE_COMMAND >> $logfile
		STATUS=1
	fi
fi

# Without the option, the SHA should be the correct one
export SPIKE_OPTS="--stf_macro_tracing"

source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi	

# The .dump file (from stf_dump) should contain the SHA of the spike repository
SPIKE_SHA_COMMENT=$(grep -o "SPIKE SHA:\s*[^ ]*" "$DUMP_OUT")
if [ -z "$SPIKE_SHA_COMMENT" ]; then
	echo "$testname_basic (without --stf_force_zero_sha): no comment present for Spike SHA (zero or otherwise) in $DUMP_OUT" >> $logfile
else
	SPIKE_GIT_SHA=$(git rev-parse HEAD)
	SHA_IN_DUMP=$(echo "$SPIKE_SHA_COMMENT" | sed 's/SPIKE SHA:\s*//')
	if [ "$SHA_IN_DUMP" != "$SPIKE_GIT_SHA" ]; then
		echo "$testname_basic (without --stf_force_zero_sha): SHA of Spike repository in $DUMP_OUT is \"$SHA_IN_DUMP\"" >> $logfile
		echo "SHA should be $SPIKE_GIT_SHA" >> $logfile
		echo "Spike command:" >> $logfile
		echo $SPIKE_COMMAND >> $logfile
		STATUS=1
	fi
fi

# The .rdump file (from stf_record_dump) should contain the SHA of the stf_lib submodule
STF_SHA_COMMENT=$(grep -o "STF.*SHA:\s*[^ ]*" "$RECORD_DUMP_OUT")
if [ -z "$STF_SHA_COMMENT" ]; then
	echo "$testname_basic (without --stf_force_zero_sha): no comment present for stf_lib submodule SHA (zero or otherwise) in $RDUMP_OUT" >> $logfile
else
	STF_LIB_GIT_SHA=$(git -C ../../stf_lib rev-parse HEAD)
	SHA_IN_DUMP=$(echo "$STF_SHA_COMMENT" | sed 's/.*SHA:\s*//')
	if [ "$SHA_IN_DUMP" != "$STF_LIB_GIT_SHA" ]; then
		echo "$testname_basic (without --stf_force_zero_sha): SHA of stf_lib submodule in $RECORD_DUMP_OUT is \"$SHA_IN_DUMP\"" >> $logfile
		echo "SHA should be $STF_LIB_GIT_SHA" >> $logfile
		echo "Spike command:" >> $logfile
		echo $SPIKE_COMMAND >> $logfile
		STATUS=1
	fi
fi

exit $STATUS
