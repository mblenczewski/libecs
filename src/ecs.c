#include "stdio.h"

#include "ecs.h"

unsigned int calculate(unsigned int num) {
	unsigned int a = 0;
	unsigned int b = 1;
	unsigned int c = 1;

	if (num == 0) {
		return 0;
	} else if (num == 1) {
		return 1;
	}

	for (unsigned int i = 0; i < num; i++)
	{
		c = a + b;
		a = b;
		b = c;
	}

	return a;
}

unsigned int calculate2(unsigned int num) {
	return 42;
}
