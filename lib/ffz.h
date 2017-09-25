#ifndef FIO_FFZ_H
#define FIO_FFZ_H

#include <inttypes.h>

/* find the first set bit int parameter word 
 * e.g.
 * 	word = 0x60
 * return value:
 * 	5
 */
static inline int ffs64(uint64_t word)
{
	int r = 0;

	if ((word & 0xffffffff) == 0) {
		r += 32;
		word >>= 32;
	}
	if (!(word & 0xffff)) {
		word >>= 16;
		r += 16;
	}
	if (!(word & 0xff)) {
		word >>= 8;
		r += 8;
	}
	if (!(word & 0xf)) {
		word >>= 4;
		r += 4;
	}
	if (!(word & 3)) {
		word >>= 2;
		r += 2;
	}
	if (!(word & 1))
		r += 1;

	return r;
}

/* find the first zero bit int parameter word 
 * e.g.
 * 	word = 0x05
 * return value:
 * 	1
 *
 * ffz: find first zero
 */
static inline int ffz(unsigned long bitmask)
{
	return ffs64(~bitmask);
}

static inline int ffz64(uint64_t bitmask)
{
	return ffs64(~bitmask);
}

#endif
