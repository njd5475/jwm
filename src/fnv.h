#ifndef FNV_H
#define FNV_H

#include <sys/types.h>

/*
 * 32 bit magic FNV-0 and FNV-1 prime
 */
typedef u_int32_t Fnv32_t;

Fnv32_t fnvbuf(const void *buf, size_t len);
Fnv32_t fnvstr(const char *str);

#endif
