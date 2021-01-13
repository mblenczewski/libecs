#ifndef TEST_H
#define TEST_H

#include <stdio.h>

#define TEST_BEGIN() \
	printf("*************** %s ***************\n", __FILE__)

#define TEST_END() \
	printf("*************** %s ***************\n", __FILE__)

#define TEST_PASS() return 1;
#define TEST_FAIL() return 0;

#define TEST_ASSERT_EQ_IMPL(value, expected, message, line)			\
	{									\
		if (value != expected) {					\
			printf("(%s:%d) %s", __FILE__, line, message);		\
			TEST_FAIL()						\
		}								\
	}

#define TEST_ASSERT_EQ(value, expected, message) \
	TEST_ASSERT_EQ_IMPL(value, expected, message, __LINE__)

#define TEST_ASSERT_NE_IMPL(value, expected, message, line)			\
	{									\
		if (value == expected) {					\
			printf("(%s:%d) %s", __FILE__, line, message);		\
			TEST_FAIL()						\
		}								\
	}

#define TEST_ASSERT_NE(value, expected, message) \
	TEST_ASSERT_NE_IMPL(value, expected, message, __LINE__)

#define TEST_ASSERT_NULL(value, message) \
	TEST_ASSERT_EQ(value, NULL, message)

#define TEST_ASSERT_NOT_NULL(value, message) \
	TEST_ASSERT_NE(value, NULL, message)

#define TEST_RUN(test)							\
	{									\
		printf("%s: ", #test);						\
		uint32_t error_code = test();					\
		if (error_code == 0) {						\
			printf("FAILED\n");					\
		} else {							\
			printf("OK\n");						\
		}								\
	}

#endif // TEST_H

