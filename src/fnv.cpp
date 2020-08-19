
#include "fnv.h"

#define FNV_32_PRIME ((Fnv32_t)0x01000193)
#define FNV1_32_INIT ((Fnv32_t)0x811c9dc5)


/*
 * fnv_32_buf - perform a 32 bit Fowler/Noll/Vo hash on a buffer
 *
 * input:
 *  buf - start of buffer to hash
 *  len - length of buffer in octets
 *
 * returns:
 *  32 bit hash as a static hash type
 */
Fnv32_t fnvbuf(const void *buf, size_t len) {
  unsigned char *bp = (unsigned char *) buf; /* start of buffer */
  unsigned char *be = bp + len; /* beyond end of buffer */

  Fnv32_t hval = FNV1_32_INIT;
  while (bp < be) {
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_32_PRIME;
#else
    hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8)
        + (hval << 24);
#endif
    hval ^= (Fnv32_t) *bp++;
  }

  return hval;
}

/*
 * fnv_32_str - perform a 32 bit Fowler/Noll/Vo hash on a string
 *
 * input:
 *  str - string to hash
 *
 * returns:
 *  32 bit hash as a static hash type
 *
 */
Fnv32_t fnvstr(const char *str) {
  unsigned char *s = (unsigned char *) str; /* unsigned string */

  Fnv32_t hval = FNV1_32_INIT;
  while (*s) {
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_32_PRIME;
#else
    hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8)
        + (hval << 24);
#endif

    hval ^= (Fnv32_t) *s++;
  }

  return hval;
}
