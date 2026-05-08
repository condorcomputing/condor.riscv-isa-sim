#include <stdint.h>

extern volatile uint64_t tohost;
extern volatile char     conio;

void __attribute__((noreturn)) tohost_exit(uintptr_t code)
{
  tohost = (code << 1) | 1;
  while (1);
}

uintptr_t __attribute__((weak)) 
handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32])
{
  (void)cause;
  (void)epc;
  (void)regs;
  tohost_exit(1337);
}

void _init(int cid, int nc)
{
  (void)cid;
  (void)nc;
  //init_tls();
  //thread_entry(cid, nc);
  // only single-threaded programs should ever get here.
  int ret = main(0, 0);
  exit(ret);
}
