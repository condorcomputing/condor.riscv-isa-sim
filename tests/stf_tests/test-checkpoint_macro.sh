#!/bin/bash

source test_common.sh

TRACE_DUMP=/data/tools/bin/stf_dump

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

logfile=$(realpath "$(dirname "$0")/../logs/stf_$testname_full.log")
: > $logfile
TEMP_OUTPUT_DIR=$(dirname "$0")/temp
SPIKE=$(realpath $1)
SPIKE_BASE_OPTS="--isa=rv64gc_H_zba_zbb_zbc_zbs"
STATUS=0

#source ./runner.sh $testname_basic $testname_full >> $logfile 2>&1

RISCV_INSTALL_DIR=$(realpath "../../install/share/stf_tests")
RISCV_BIN=$RISCV_INSTALL_DIR/$testname_basic.riscv

BASE_FILENAME=$(realpath $TEMP_OUTPUT_DIR/$testname_full)
OBJDUMP_OUT=$BASE_FILENAME.objdump
riscv64-unknown-elf-objdump -Mnumeric -S $RISCV_BIN > $OBJDUMP_OUT

cd $TEMP_OUTPUT_DIR
rm -f checkpoint_*.json mem*.bin
rm -f checkpoint_*.json.macro mem*.bin.macro

if [ -z "$STF_EXT" ]; then
	STF_EXT="stf"
fi

# 1. create a checkpoint via instruction macro
SPIKE_OPTS="$SPIKE_BASE_OPTS --checkpoint_macro_enable"
SPIKE_COMMAND="$SPIKE $SPIKE_OPTS $RISCV_BIN"
echo $SPIKE_COMMAND >> $logfile

$SPIKE_COMMAND >> $logfile 2>&1

if [ $? -ne 0 ]; then
	exit 1
fi	

checkpoint=$(ls -1 checkpoint_*.json)
instruction_num="${checkpoint#checkpoint_}"
instruction_num="${instruction_num%.json}"
next_instruction_num=$(($instruction_num + 1))

mv "$checkpoint" "${checkpoint}.macro"
mv "mem80000000_${instruction_num}.bin" "mem80000000_${instruction_num}.bin.macro" 


# 2. create a checkpoint based on instruction number
SPIKE_OPTS="$SPIKE_BASE_OPTS --checkpoint_instruction $instruction_num"
SPIKE_COMMAND="$SPIKE $SPIKE_OPTS $RISCV_BIN"
echo $SPIKE_COMMAND >> $logfile

$SPIKE_COMMAND >> $logfile 2>&1

if [ $? -ne 0 ]; then
	exit 1
fi


# 3. Compare checkpoints from steps 1 and 2. They should be identical.
diff=$(diff "checkpoint_${instruction_num}.json" "checkpoint_${instruction_num}.json.macro" | grep -e "^[<>]" | grep -v real_time)

if [[ -n $diff ]]; then
   echo "-E: Checkpoints differ:" >> $logfile
   echo $diff >> $logfile
   exit 1
fi

diff -q "mem80000000_${instruction_num}.bin" "mem80000000_${instruction_num}.bin.macro"
if [ $? -ne 0 ]; then
   echo "-E: Memory images differ" >> $logfile
   exit 1
fi	


# 4. Collect a trace from full execution
STF_OUT=$BASE_FILENAME.$STF_EXT
rm -f $STF_OUT
SPIKE_OPTS="$SPIKE_BASE_OPTS --stf_trace $STF_OUT --stf_insn_num_tracing --stf_insn_start ${next_instruction_num} --stf_insn_count $(( $instruction_num + 5 ))"
SPIKE_COMMAND="$SPIKE $SPIKE_OPTS $RISCV_BIN"
echo $SPIKE_COMMAND >> $logfile

$SPIKE_COMMAND >> $logfile 2>&1

if [ $? -ne 0 ]; then
	exit 1
fi

#trace_full_run=$($TRACE_DUMP $STF_OUT | grep -e "^INST\(16\|32\)" | tail -n 4)


# 5. Collect a trace from the checkpoint
CP_STF_OUT="${BASE_FILENAME}_checkpoint_${instruction_num}.${STF_EXT}"
rm -f $CP_STF_OUT
SPIKE_OPTS="$SPIKE_BASE_OPTS --stf_trace $CP_STF_OUT --stf_insn_num_tracing --stf_insn_start ${next_instruction_num} --stf_insn_count $(( $instruction_num + 5 ))"
SPIKE_COMMAND="$SPIKE $SPIKE_OPTS --restore_checkpoint ${checkpoint}"
echo $SPIKE_COMMAND >> $logfile

$SPIKE_COMMAND >> $logfile 2>&1

if [ $? -ne 0 ]; then
	exit 1
fi

#trace_checkpoint=$($TRACE_DUMP $STF_OUT | grep -e "^INST\(16\|32\)" | tail -n 4)


# 6. Trace files from 4 and 5 should be the same:
diff -q "$CP_STF_OUT" "$STF_OUT"
if [ $? -ne 0 ]; then
   echo "-E Trace files differ." >> $logfile
   exit 1
fi


# 7. Trace the last instruction before the checkpoint. It should be the checkpoint macro instruction.
STF_OUT=$BASE_FILENAME.$STF_EXT
rm -f $STF_OUT
SPIKE_OPTS="$SPIKE_BASE_OPTS --stf_trace $STF_OUT --stf_insn_num_tracing --stf_insn_start $instruction_num --stf_insn_count 1"
SPIKE_COMMAND="$SPIKE $SPIKE_OPTS $RISCV_BIN"
echo $SPIKE_COMMAND >> $logfile
$SPIKE_COMMAND >> $logfile 2>&1

if [ $? -ne 0 ]; then
	exit 1
fi

trace=$($TRACE_DUMP $STF_OUT | grep -e "^INST\(16\|32\)")

if [[ ! $trace == *"00214033"* ]]; then
   echo "-E instruction $instruction_num is not the trace macro" >> $logfile
   exit 1;
fi

exit $STATUS
