#include "trace_macros.asm"

# traverse priv modes: M->H->VU->VS
machine_start:
    # =================== MACHINE MODE =====================
    # Run instructions in M-mode
    START_TRACE_ASM
    add         x0,x1,x2
    STOP_TRACE_ASM

    # Set MPP = 1
    li t0, 0x1800
    csrrc t1, mstatus, t0
    li t0, (1 << 11)
    csrs mstatus, t0

    li t0, (1 << 17)        # Set MPV
    csrs mstatus, t0

    li t0, (1 << 8)
    csrrs t0, medeleg, t0

    la t0, hypervisor_start
    csrw mepc, t0
    mret

# =================== HYPERVISOR MODE ======================
hypervisor_start:
    # Run instructions in H-mode
    START_TRACE_ASM
    add         x0,x2,x2
    STOP_TRACE_ASM

    # SPVP=1 for VS-mode
    csrr t0, hstatus
    li t1, (1 << 7 | 1 << 8)
    or t0, t0, t1
    csrw hstatus, t0

    # Handle U-mode ecalls in VS mode
    li t1, (1<<8)
    csrrs t1, hedeleg, t1

    la t0, supervisor_start
    andi t0, t0, -4
    csrw vstvec, t0

    la t0, user_start
    csrw sepc, t0
    sret

# =================== USER MODE ============================
user_start:
    # Run instructions in U-mode
    START_TRACE_ASM
    add         x0,x4,x2
    STOP_TRACE_ASM

    scall # env call to VS mode

# =================== SUPERVISOR MODE ======================
.align 4
supervisor_start:
    # Run instructions in S-mode
    START_TRACE_ASM
    add         x0,x3,x2
    STOP_TRACE_ASM

