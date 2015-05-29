#include "../src/logging.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
	init_logging(argv[0], "", LOG_LEVEL_DEBUG);
	LOG_DEBUG("A basic debug log looks like this");
	return 0;
}
