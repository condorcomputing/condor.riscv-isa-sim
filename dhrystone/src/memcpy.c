#include <stdint.h>
#include <stddef.h>

#if defined(CONDOR_DRY_OPT2) || defined(CONDOR_DRY_OPT3)
void* memcpy(void* dest, const void* src, size_t len)
{
  if ((((uintptr_t)dest | (uintptr_t)src | len) & (sizeof(uintptr_t)-1)) == 0) {
    const uintptr_t* s = src;
    uintptr_t *d = dest;
    while (d < (uintptr_t*)(dest + len))
      *d++ = *s++;
  } else {
    const char* s = src;
    char *d = dest;
    while (d < (char*)(dest + len))
      *d++ = *s++;
  }
  return dest;
}
#elif defined(CONDOR_DRY_OPT1)
void *memcpy (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}
#else
#error "One of CONDOR_OPT1/2/3 must be defined"
#endif
