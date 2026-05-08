#!/bin/bash

TRACE_DUMP=stf_dump
TRACE_RECORD_DUMP=stf_record_dump

THIS_SCRIPT=$(basename "$0")
TEST_NAME_BASIC=$1

if [ -z "$TEST_NAME_BASIC" ]; then
    echo "Usage: $THIS_SCRIPT <test_name>"
    exit 1
fi

if [ -n "$2" ]; then
	TEST_NAME_FULL=$2
else
	TEST_NAME_FULL=$TEST_NAME_BASIC
fi

if [ -z "$SPIKE" ]; then
    echo "SPIKE is $SPIKE"
	echo "Set SPIKE environment variable to the path of the spike binary"
	exit 1
fi

if [ -z "$SPIKE_OPTS" ]; then
	echo "Set SPIKE_OPTS environment variable to the options for spike"
	exit 1
fi

if [ -z "$STF_EXT" ]; then
	STF_EXT="stf"
fi

if [ -z "$TEMP_OUTPUT_DIR" ]; then
	TEMP_OUTPUT_DIR=$(dirname "$0")/temp
fi

if [ ! -d "$TEMP_OUTPUT_DIR" ]; then
	mkdir -p $TEMP_OUTPUT_DIR
fi

BASE_FILENAME=$TEMP_OUTPUT_DIR/$TEST_NAME_FULL
STF_OUT=$BASE_FILENAME.$STF_EXT
RISCV_INSTALL_DIR=../../install/share/stf_tests
export RISCV_BIN=$RISCV_INSTALL_DIR/$TEST_NAME_BASIC.riscv

OBJDUMP_OUT=$BASE_FILENAME.objdump
riscv64-unknown-elf-objdump -Mnumeric -S $RISCV_BIN > $OBJDUMP_OUT

# Run Spike
SPIKE_ALL_OPTS="--isa=rv64gc_H_zba_zbb_zbc_zbs --stf_trace $STF_OUT $SPIKE_OPTS"
export SPIKE_COMMAND="$SPIKE $SPIKE_ALL_OPTS $RISCV_BIN"
SPIKE_OUTPUT=$($SPIKE_COMMAND 2>&1)

SPIKE_EXIT_CODE=$?
if [ $SPIKE_EXIT_CODE -ne 0 ]; then
	echo "$THIS_SCRIPT: $SPIKE failed with exit code $SPIKE_EXIT_CODE"
	echo "Spike command: "
	echo $SPIKE_COMMAND
	echo "Spike output: "
	echo $SPIKE_OUTPUT
	exit 1
fi
# Check if the output file exists
if [ ! -f $STF_OUT ]; then
	echo "$THIS_SCRIPT: $STF_OUT not found"
	echo "Spike command: "
	echo $SPIKE_COMMAND
	exit 1
fi
# Check if the output file is not empty
if [ ! -s $STF_OUT ]; then
	echo "$THIS_SCRIPT: $STF_OUT is empty"
	echo "Spike command: "
	echo $SPIKE_COMMAND
	exit 1
fi

# Generate the "regular" dump of the STF output
export DUMP_OUT=$BASE_FILENAME.dump
DUMP_COMMAND="$TRACE_DUMP $STF_OUT"
$DUMP_COMMAND > $DUMP_OUT

if [ $? -ne 0 ]; then
	echo "$THIS_SCRIPT: $TRACE_DUMP failed"
	echo "stf_dump command: "
	echo $DUMP_COMMAND
	exit 1
fi
# Check if the output file exists
if [ ! -f $DUMP_OUT ]; then
	echo "$THIS_SCRIPT: $DUMP_OUT not found"
	echo "stf_dump command: "
	echo $DUMP_COMMAND
	exit 1
fi
# Check if the output file is not empty
if [ ! -s $DUMP_OUT ]; then
	echo "$THIS_SCRIPT: $DUMP_OUT is empty"
	echo "stf_dump command: "
	echo $DUMP_COMMAND
	exit 1
fi

# Generate the "record" dump of the STF output
export RECORD_DUMP_OUT=$BASE_FILENAME.rdump
RECORD_DUMP_COMMAND="$TRACE_RECORD_DUMP $STF_OUT"
$RECORD_DUMP_COMMAND > $RECORD_DUMP_OUT

if [ $? -ne 0 ]; then
	echo "$THIS_SCRIPT: $TRACE_RECORD_DUMP failed"
	echo "stf_record_dump command: "
	echo $RECORD_DUMP_COMMAND
	exit 1
fi
# Check if the output file exists
if [ ! -f $RECORD_DUMP_OUT ]; then
	echo "$THIS_SCRIPT: $RECORD_DUMP_OUT not found"
	echo "stf_record_dump command: "
	echo $RECORD_DUMP_COMMAND
	exit 1
fi
# Check if the output file is not empty
if [ ! -s $RECORD_DUMP_OUT ]; then
	echo "$THIS_SCRIPT: $RECORD_DUMP_OUT is empty"
	echo "stf_record_dump command: "
	echo $RECORD_DUMP_COMMAND
	exit 1
fi
