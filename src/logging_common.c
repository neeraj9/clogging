/*
 * Copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

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
