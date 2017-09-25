#include <stdio.h>
#include "arch/arch.h"
#include "smalloc.h"

/* find first zero bit in the word from index start 
 * e.g 
 * 	word = 0x1D(00011101b),  start = 3
 * 	
 * 	00011101b
 * 	    ^
 * 	    |(start:3)
 *
 * return value
 * 	5
 */
static int find_next_zero(int word, int start)
{
	word >>= start;	
	return ffz(word) + start;
}

int main()
{
	int word = 0x1D;
	int start = 3;

	printf("%d\n", find_next_zero(word, start));
	return 0;
}
