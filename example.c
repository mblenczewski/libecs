#include "ecs.h"
#include "version.h"

#include <stdio.h>

int main(int argc, char **argv) {
	printf("(libecs %s) Hello, World!\n", ECS_VERSION);

	for (u32 i = 0; i < (u32)argc; i++) {
		printf("Arg %d: %s\n", i, argv[i]);
	}

	return 0;
}

