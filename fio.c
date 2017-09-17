#include<stdio.h>

int main(int argc, char *argv[])
{
	printf("hello fio\n");
	printf("%d\n", sizeof(unsigned long long));

	printf("check:%d\n", endian_check());
	return 0;
}
