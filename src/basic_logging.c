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
 *
 */

#include "basic_logging.h"

#include <stdio.h>		/* fprintf() and friends */
#include <string.h>		/* strncpy() */
#include <sys/time.h>		/* gmtime_r() */
#include <sys/types.h>
#include <time.h>		/* time() */
#include <stdarg.h>		/* va_start() and friends */
#include <unistd.h>		/* getpid(), gethostname() */

#define MAX_PROG_NAME_LEN 40
#define MAX_HOSTNAME_LEN 20


/*
 * The use of global static is tricky and prone to issues in MT (multi-threaded)
 * environments, but then the init_logging should be called ONLY once.
 * Additionally, the set_loglevel() should also be called typically ONLY once
 * and even if its called multiple times the integer assignment of
 * g_level is an atomic write.
 *
 * We are not using mutexes because that will be too much of an
 * overhead while lock/unlock in the logmsg() implementation. Additionally,
 * the set_loglevel() is just an integer assignment so its atomic
 *
 *
 * Alternative approach is to use thread local storage, when the
 * LOGGING_WITH_THREAD_LOCAL_STORAGE is defined and
 * LOGGING_OPTIONAL_TLS is set to __thread (gnu directive) for
 * thread local storage. In this case there is no MT issue since
 * each of the thread will have its own copy of the file static variable.
 * The side effect is to call init_logging() separately for each of the
 * threads. This differs from the fact when thread local storage is not
 * used, when only the main thread should call init_logging() and only
 * once.
 *
 */
static LOGGING_OPTIONAL_TLS char g_progname[MAX_PROG_NAME_LEN] = {0};
static LOGGING_OPTIONAL_TLS char g_threadname[MAX_PROG_NAME_LEN] = {0};
static LOGGING_OPTIONAL_TLS char g_hostname[MAX_HOSTNAME_LEN] = {0};
static LOGGING_OPTIONAL_TLS int g_pid = 0;
static LOGGING_OPTIONAL_TLS enum LogLevel g_level = DEFAULT_LOG_LEVEL;
/* safeguard calling init_logging multiple times */
static LOGGING_OPTIONAL_TLS int g_is_logging_initialized = 0;

/*
 * Get the current time in c string with '\0' termination
 * and returns the length of the string.
 * The current implementation follows the ISO 8601 date and time
 * combined format with a resolution of seconds in the UTC timezone.
 * In case of error -1 is retured.
 */
int
time_to_cstr(time_t *t, char *timestr, int maxlen)
{
	struct tm tms;

	gmtime_r(t, &tms);
	snprintf(timestr, maxlen, "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
		 tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday,
		 tms.tm_hour, tms.tm_min, tms.tm_sec);
}

void
logmsg(const char *funcname, int linenum, enum LogLevel level,
       const char *format, ...)
{
	/* ISO 8601 date and time format with sec */
	const int time_str_len = 26;
	char time_str[time_str_len];
	time_t now;
	int len = 0;
	int rc = 0;
	const char *level_str = 0;
	char msg[MAX_LOG_MSG_LEN];
	va_list ap;

	/* ignore logs which are filtered out */
	if (level > g_level) {
		return;
	}

	if (g_is_logging_initialized <= 0) {
		fprintf(stderr, "logging is not initialized yet\n");
		return;
	}

	/* only when thread local storage is not used the memory
	 * barrier becomes relevant.
	 */
#ifndef LOGGING_WITH_THREAD_LOCAL_STORAGE
	/* read memory barrier */
	__sync_synchronize();
#endif

	time(&now);
	len = time_to_cstr(&now, time_str, time_str_len);
	if (len < 0) {
		/* huh! I'd like to crash at this point but
		 * lets just log the message, which is a must.
		 */
		return;
	}

	level_str = get_log_level_as_cstring(level);

	va_start(ap, format);
	rc = vsnprintf(msg, MAX_LOG_MSG_LEN, format, ap);
	if (rc < 0) {
		/* cannot recover from this one, so lets just ignore it
		 * for now rather than logging it somewhere.
		 */
		return;
	}
	va_end(ap);

	/* <HEADER> <MESSAGE>
	 *	<HEADER> = <TIMESTAMP> <HOSTNAME>
	 *	<MESSAGE> = <TAG> <LEVEL> <CONTENT>
	 *		<TAG> = <PROGRAM><THREAD>[<PID>]
	 *		<LEVEL> = DEBUG | INFO | WARNING | ERROR
	 *		<CONTENT> = <FUNCTION/MODULE>: <APPLICATION_MESSAGE>
	 */
	fprintf(stderr, "%s %s %s%s[%d] %s %s(%d): %s\n", time_str, g_hostname,
		g_progname, g_threadname, g_pid, level_str, funcname, linenum,
		msg);
	/* ignore the error if it's there */
}

int
init_logging(const char *progname, const char *threadname, enum LogLevel level)
{
	int rc = 0;

	if (g_is_logging_initialized > 0) {
		fprintf(stderr, "logging is already initialized or in the"
			" process of initialization.\n");
		return -1;
	}

	/* when thread local storage is used then we shouldn't worry
	 * about memory barrier, but lets not pollute things for initialization
	 * code when thread local storage is enabled.
	 */

	/* read memory barrier, so its guaranteed to read the global
	 * value of g_is_logging_initialized before its set after
	 * the barrier.
	 */
	__sync_synchronize();

	/* first thing to do is block any other thread in running logging
	 * initialization (if that is done, though its bad and should be
	 * done in the main thread ONLY.
	 */
	g_is_logging_initialized = 1;

	/* Intentionally call the method so that any function static variables
	 * in get_log_level_as_cstring() are correctly initialized.
	 * Note that this should be done in any logging implementation (like
	 * many other things), although the argument and the return values
	 * are unimportant.
	 */
	(void)get_log_level_as_cstring(LOG_LEVEL_ERROR);

	strncpy(g_progname, progname, MAX_PROG_NAME_LEN);
	strncpy(g_threadname, threadname, MAX_PROG_NAME_LEN);
	rc = gethostname(g_hostname, MAX_HOSTNAME_LEN);
	if (rc < 0) {
		strncpy(g_hostname, "unknown", MAX_HOSTNAME_LEN);
	}
	g_pid = (int)getpid();
	g_level = level;

	return 0;
}

void
set_loglevel(enum LogLevel level)
{
	g_level = level;
}

enum LogLevel
get_loglevel(void)
{
	return g_level;
}
