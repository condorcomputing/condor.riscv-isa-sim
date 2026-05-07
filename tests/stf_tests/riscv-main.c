
#include <stdint.h>
#include <sys/signal.h>

// For linux define these locally, for bare metal
// they are present in linker control and crt.S
#ifdef __linux
#include <stdio.h>
volatile uint64_t tohost   __attribute__((used));
volatile uint64_t fromhost __attribute__((used));
volatile char     conio    __attribute__((used));
void _pass() __attribute__((used,noinline));
void _pass() {}
#else
extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;
extern volatile char     conio;
extern void _pass() __attribute__((used,noinline));
#endif

#define BEGIN 1
#define END   0

void __attribute__((noreturn)) tohost_exit(uintptr_t code)
{
  tohost = (code << 1) | 1;
  while (1);
}

void exit(int code)
{
  tohost_exit(code);
}

void abort()
{
  exit(128 + SIGABRT);
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------

int main(int ac,char **av)
{
  (void)ac;
  (void)av;

  asm volatile 
  (

// Stringify ASM_INCLUDE, which is defined on the compiler command line and specifies
// the name of the file with the test-specific assembly instructions.
#ifdef ASM_INCLUDE
#define STR(x) STR2(x)
#define STR2(x) #x
#define ASM_INCLUDE_STR STR(ASM_INCLUDE)
#include ASM_INCLUDE_STR
#else
#error "ASM_INCLUDE not defined"
"nop"
#endif
  );

  return 0;
}

void _init(int cid, int nc)
{
	int ret = main(0, 0);
	exit(ret);
}

uintptr_t __attribute__((weak)) handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32])
{
  tohost_exit(cause);
}
