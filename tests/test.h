#ifndef TEST_H
#define TEST_H

#include <stdio.h>

#define TEST_BEGIN() \
	printf("*************** %s ***************\n", __FILE__);

#define TEST_END() \
	printf("*************** %s ***************\n", __FILE__);

#define PASS() return 1;
#define FAIL() return 0;

#define TEST_RUN(test) \
{ \
	printf("%s: ", #test); \
	if (test() == 0) { \
		printf("FAILED\n"); \
	} else { \
		printf("OK\n"); \
	} \
}

#endif // TEST_H

