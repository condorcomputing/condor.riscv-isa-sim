#!/bin/bash

THIS_SCRIPT=$(basename "$0")
TEST_NAME=$2

export SPIKE=$1

if [ -z "$BBV_EXT" ]; then
	BBV_EXT="bbv"
fi

if [ -z "$TEMP_OUTPUT_DIR" ]; then
	TEMP_OUTPUT_DIR=$(dirname "$0")/temp
fi

if [ ! -d "$TEMP_OUTPUT_DIR" ]; then
	mkdir -p $TEMP_OUTPUT_DIR
fi

check_output() {
  if [ $SPIKE_EXIT_CODE -ne 0 ]; then
	echo "$THIS_SCRIPT: $SPIKE failed with exit code $SPIKE_EXIT_CODE"
	echo "Spike command: "
	echo $SPIKE_COMMAND
	echo "Spike output: "
	echo $SPIKE_OUTPUT
	exit 1
  fi
  # Check if the output file exists
  if [ ! -f $BBV_OUT ]; then
	echo "$THIS_SCRIPT: $BBV_OUT not found"
	echo "Spike command: "
	echo $SPIKE_COMMAND
	exit 1
  fi
  # Check if the output file looks right:
  if [ ! $(grep "T:1:2 :2:10 :3:12" $BBV_OUT | wc -l) == "10" ]; then
	echo "$THIS_SCRIPT: $BBV_OUT doesn't contain the expected values."
	echo "Spike command: "
	echo $SPIKE_COMMAND
	exit 1
  fi

  if [ ! $(grep -v "T:1:2 :2:10 :3:12" $BBV_OUT | wc -l) == "0" ]; then
	echo "$THIS_SCRIPT: $BBV_OUT contains unexpected lines."
	echo "Spike command: "
	echo $SPIKE_COMMAND
	exit 1
  fi
}

BASE_FILENAME=$TEMP_OUTPUT_DIR/$TEST_NAME
BBV_ARG=$BASE_FILENAME.$BBV_EXT
BBV_OUT="${BASE_FILENAME}.${BBV_EXT}_cpu0"
LOG_OUT="${BASE_FILENAME}.log"

RISCV_INSTALL_DIR=../../install/share/stf_tests
RISCV_BIN=$RISCV_INSTALL_DIR/$TEST_NAME.riscv

OBJDUMP_OUT=$BASE_FILENAME.objdump
riscv64-unknown-elf-objdump -Mnumeric -S $RISCV_BIN > $OBJDUMP_OUT

SPIKE_ALL_OPTS="--isa=rv64gc_H_zba_zbb_zbc_zbs --en_bbv --bb_file $BBV_ARG --simpoint_size 24"
export SPIKE_COMMAND="$SPIKE $SPIKE_ALL_OPTS $RISCV_BIN"


# Run Spike without STF logging:
SPIKE_OUTPUT=$($SPIKE_COMMAND 2>&1 | tee "$LOG_OUT")

SPIKE_EXIT_CODE=$?

check_output


# Run Spike with STF logging:
SPIKE_ALL_OPTS="${SPIKE_ALL_OPTS} -l"
export SPIKE_COMMAND="$SPIKE $SPIKE_ALL_OPTS $RISCV_BIN"
SPIKE_OUTPUT=$($SPIKE_COMMAND 2>&1 | tee "$LOG_OUT")

SPIKE_EXIT_CODE=$?

check_output

