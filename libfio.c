#include<stdio.h>

enum {
	ENDIAN_INVALID_BE = 1,
	ENDIAN_INVALID_LE,
	ENDIAN_INVALID_CONFIG,
	ENDIAN_BROKEN,
};

int endian_check()
{
	union {
		unsigned char c[8];
		unsigned long long v;
	} u;
	int le = 0, be = 0;

	u.v = 0x12;
	if (u.c[0] == 0x12)
		le = 1;
	else if (u.c[7] == 0x12)
		be = 1;

#if defined(CONFIG_LITTLE_ENDIAN)
	if (be)
		return ENDIAN_INVALID_BE;
#elif defined(CONFIG_BIG_ENDIAN)
	if (le)
		return ENDIAN_INVALID_LE;
#else
	return ENDIAN_INVALID_CONFIG; 
#endif

	if (!le && !be)
		return ENDIAN_BROKEN;

	return 0;
}
