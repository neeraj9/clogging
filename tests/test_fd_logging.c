/*
 * Copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
 *
 *  This file is part of clogging.
 *
 *  clogging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  clogging is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with clogging.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../src/fd_logging.h"

#include <sys/prctl.h>
#include <assert.h>
#include <stdio.h>

/* as per man prctl(2) the size should be at least 16 bytes */
#define MAX_SIZE 32

int main(int argc, char *argv[])
{
	int fd = 1;  /* fd for stdout */
	int rc = 0;
	char pname[MAX_SIZE] = {0};

	rc = prctl(PR_GET_NAME, (unsigned long)(pname), 0, 0, 0);
	assert(rc == 0);
	/* printf("pname = %s\n", pname); */
	/* printf("argv[0] = %s\n", argv[0]); */
	FD_INIT_LOGGING(pname, "", LOG_LEVEL_DEBUG, fd);
	FD_LOG_DEBUG("A fd debug log looks like this");
	assert(FD_GET_LOG_LEVEL() == LOG_LEVEL_DEBUG);
	FD_SET_LOG_LEVEL(LOG_LEVEL_INFO);
	assert(FD_GET_LOG_LEVEL() == LOG_LEVEL_INFO);
	assert(FD_GET_NUM_DROPPED_MESSAGES() == 0);
	return 0;
}
