
#!/bin/bash

check_inst_trace_addr() {
  local INST_TO_MATCH=$1
  local DUMP_OUT=$2
  local RISCV_BIN=$3
  local testname=$4

  # Get the address of a traced instruction from the objdump, and check that
  # the traced address matches:
  local OBJDUMP_OUTP=$(riscv64-unknown-elf-objdump -Mnumeric -S $RISCV_BIN)
  local ELF_INSTR=$(echo "$OBJDUMP_OUTP" | grep "$INST_TO_MATCH")

  if [[ ! -n "$ELF_INSTR" ]]; then
    echo "$testname: $RISCV_BIN Failed to find match instruction ${INST_TO_MATCH} in objdump" >> $logfile
    return 1
  fi

  local address=$(sed -E 's/^[[:space:]]*([0-9A-Fa-f]+):.*$/\1/' <<< "$ELF_INSTR")

  if ! grep -q "${address}.*${INST_TO_MATCH}" $DUMP_OUT; then
    echo "$testname: $DUMP_OUT traced instruction addresses do not match expected values" >> $logfile
    echo "Spike command:" >> $logfile
    echo $SPIKE_COMMAND >> $logfile
    return 1
  fi

  return 0
}

