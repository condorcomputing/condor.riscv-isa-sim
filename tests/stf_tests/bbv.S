    .section .text

enter:
    # initialize trap vector
    la t0, mhandler
    csrw mtvec, t0

    auipc t0, 0
    addi t0, t0, 8
    csrw mepc, t0
    j mhandler

    li t0, 10             # Loop counter
    li t1, 0x0            # Basic Block 1
    li t2, 0x0            # Basic Block 2
    li t3, 0x0            # Basic Block 3

    csrs 0x8c2, 1

loop:
    # Basic Block 1
    addi t1, t1, 1        # Increment BB1 count
    li a0, 100
    li a1, 200
    add a2, a0, a1

    # Basic Block 2
    addi t2, t2, 1        # Increment BB2 count
    li a3, 300
    li a4, 400
    sub a5, a4, a3

    # Basic Block 3
    addi t3, t3, 1        # Increment BB3 count
    li a6, 500
    li a7, 600
    and a6, a6, a7

    # Force Interval Break
    ecall                 # Trigger an environment call (context switch)

    # Loop control
    addi t0, t0, -1
    bnez t0, loop

    csrc 0x8c2, 1
    j fin

mhandler:
    # Set MPP = 1
    li t4, 0x1800
    csrrc t4, mstatus, t4
    li t4, (0 << 11)
    csrs mstatus, t4

    csrr t4, mepc
    addi t4, t4, 4
    csrw mepc, t4
    mret

fin:
    # End program
    li a0, 0x0

