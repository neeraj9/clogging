/*
 * Copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
 *
 *  This file is part of clogging.
 *
 *  clogging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
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

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The use of global static is tricky and prone to issues in MT (multi-threaded)
 * environments, but then the clogging_basic_init should be called ONLY once.
 * In this case there is no MT issue since
 * each of the thread will have its own copy of the file static variable.
 * The side effect is to call init_logging() separately for each of the
 * threads. This differs from the fact when thread local storage is not
 * used, when only the main thread should call init_logging() and only
 * once.
 *
 */
static __thread char g_progname[MAX_PROG_NAME_LEN] = {0};
static __thread char g_threadname[MAX_PROG_NAME_LEN] = {0};
static __thread char g_hostname[MAX_HOSTNAME_LEN] = {0};
static __thread int g_pid = 0;
static __thread enum LogLevel g_level = DEFAULT_LOG_LEVEL;
/* safeguard calling init_logging multiple times */
static __thread int g_is_logging_initialized = 0;

/* store the number of message dropped as a counter for
 * later statistics collection.
 */
static __thread uint64_t g_basic_num_msg_drops = 0;

int
clogging_basic_init(const char *progname, const char *threadname,
		    enum LogLevel level)
{
	int rc = 0;

	if (g_is_logging_initialized > 0) {
		fprintf(stderr, "logging is already initialized or in the"
			" process of initialization.\n");
		return -1;
	}

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
clogging_basic_set_loglevel(enum LogLevel level)
{
	g_level = level;
}

enum LogLevel
clogging_basic_get_loglevel(void)
{
	return g_level;
}

void
clogging_basic_logmsg(const char *funcname, int linenum, enum LogLevel level,
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

	time(&now);
	len = time_to_cstr(&now, time_str, time_str_len);
	if (len < 0) {
		/* huh! I'd like to crash at this point but
		 * lets just log the message, which is a must.
		 */
		++g_basic_num_msg_drops;
		return;
	}

	level_str = get_log_level_as_cstring(level);

	va_start(ap, format);
	rc = vsnprintf(msg, MAX_LOG_MSG_LEN, format, ap);
	if (rc < 0) {
		/* cannot recover from this one, so lets just ignore it
		 * for now rather than logging it somewhere.
		 */
		++g_basic_num_msg_drops;
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
	rc = fprintf(stderr, "%s %s %s%s[%d] %s %s(%d): %s\n", time_str,
		     g_hostname, g_progname, g_threadname, g_pid, level_str,
		     funcname, linenum, msg);
	/* ignore the error if it's there */
	if (rc < 0) {
		++g_basic_num_msg_drops;
	}
	/* Note that when less number of characters are written, even then
	 * we dont keep track of that.
	 */
}

uint64_t
clogging_basic_get_num_dropped_messages(void)
{
	return g_basic_num_msg_drops;
}

#ifdef __cplusplus
}
#endif
