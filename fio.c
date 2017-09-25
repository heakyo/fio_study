#include<stdio.h>
#include"lib/ffz.h"

int main(int argc, char *argv[])
{
	int bitmask = 0x05;

	printf("v: %d\n", ffz(bitmask));

	return 0;
}
