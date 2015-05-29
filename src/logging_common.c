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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logging_common.h"

#include <stdio.h>		/* snprintf() */
#include <time.h>		/* gmtime_r() */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get the string representation of logging level.
 */
const char *
get_log_level_as_cstring(enum LogLevel level)
{
	/*
	 * A better alternative would be to use thread local storage,
	 * via the __thread gnu directive. This will add a cost to
	 * it though, which is inherently due to the way thread local
	 * storage is relaized by the gnu c compiler.
	 * In case this function is called infrequently, which will
	 * ultimately depend on calls to logging api then use the
	 * thread local approach.
	 *
	 */

	/* The mapping is based on the enumeration values as in
	 * logging_common.h, so update this mapping table to
	 * match that.
	 */
	static const __thread char *level_to_str[] = {
		"ERROR",	/* 0 */
		"WARN",		/* 1 */
		"INFO",		/* 2 */
		"DEBUG"		/* 3 */
	};

	return level_to_str[level];
}

int
time_to_cstr(time_t *t, char *timestr, int maxlen)
{
	struct tm tms;

	gmtime_r(t, &tms);
	snprintf(timestr, maxlen, "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
		 tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday,
		 tms.tm_hour, tms.tm_min, tms.tm_sec);
}


#ifdef __cplusplus
}
#endif
