#include "trace_macros.asm"
START_TRACE_ASM
add         x0,x1,x2
add         x0,x2,x2
add         x0,x3,x2
add         x0,x4,x2
beq t0,t0, a_branch_target
add         x0,x5,x3

a_branch_target:
add         x0,x5,x2

STOP_TRACE_ASM
