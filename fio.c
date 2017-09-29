#include <stdio.h>

#include "lib/ffz.h"
#include "smalloc.h"

int main(int argc, char *argv[])
{
	void *ptr = NULL;

	sinit();

	ptr = smalloc(100);

	printf("ptr: %p\n", ptr);

	return 0;
}
