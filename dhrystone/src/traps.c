#include <stdint.h>
extern volatile uint64_t tohost;

void __attribute__((noreturn)) tohost_exit(uintptr_t code)
{
  tohost = (code << 1) | 1;
  while (1);
}

//The arg list is defined so it's the same as _init
//is defined in microbench/common/syscalls.c
uintptr_t __attribute__((weak))
handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32])
{
  (void) cause;
  (void) epc;
  (void) regs;
  tohost_exit(1337);
}

void exit(int code)
{
  tohost_exit(code);
}

//The arg list is defined so it's the same as _init
//is defined in microbench/common/syscalls.c
void _init(int cid, int nc)
{
  (void) cid;
  (void) nc;
  int ret = main(0, 0);
  exit(ret);
}


