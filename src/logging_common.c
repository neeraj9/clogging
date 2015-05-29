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

#ifndef LOGGING_OPTIONAL_TLS
#define LOGGING_OPTIONAL_TLS
#endif

#include "logging_common.h"

/*
 * Get the string representation of logging level.
 */
const char *
get_log_level_as_cstring(enum LogLevel level)
{
	/* Initialization is not thread-safe when thread local storage
	 * is not enabled via LOGGING_WITH_THREAD_LOCAL_STORAGE, but the
	 * following explanation should hopefully suffice.
	 *
	 * Although the level_to_str[] is a static array and in MT
	 * programs this is an issue when called simultaneously,
	 * so a crude workaround (instead of if/else is to
	 * call this method as first thing when the logging is getting
	 * initialized. This will set the function static correctly for
	 * other threads to use without any race condition.
	 *
	 * An alternate approach would be to use if/else, but I like
	 * lookup better anyways.
	 *
	 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is defined.
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
	static const LOGGING_OPTIONAL_TLS char *level_to_str[] = {
		"ERROR",	/* 0 */
		"WARN",		/* 1 */
		"INFO",		/* 2 */
		"DEBUG"		/* 3 */
	};

	return level_to_str[level];
}
