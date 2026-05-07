#include "trace_macros.asm"

la x5, __test_data_start
li x6, 0x12345678

START_TRACE_ASM
# Write to memory
sw x6, 0(x5)            # Store word from x6 into memory at address in x5

# Read from memory
lw x7, 0(x5)            # Load word from memory at address in x5 into x7
STOP_TRACE_ASM
