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

#include "fd_logging.h"

#include <stdio.h>		/* dprintf() and friends */
#include <string.h>		/* strncpy() */
#include <sys/time.h>		/* gmtime_r() */
#include <sys/types.h>
#include <time.h>		/* time() */
#include <stdarg.h>		/* va_start() and friends */
#include <unistd.h>		/* getpid(), gethostname(), write() */

#define MAX_PROG_NAME_LEN 40
#define MAX_HOSTNAME_LEN 20

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The use of global static is tricky and prone to issues in MT (multi-threaded)
 * environments, but then the clogging_binary_init should be called ONLY once.
 * In this case there is no MT issue since
 * each of the thread will have its own copy of the file static variable.
 * The side effect is to call init_logging() separately for each of the
 * threads. This differs from the fact when thread local storage is not
 * used, when only the main thread should call init_logging() and only
 * once.
 *
 */
static __thread char g_binary_progname[MAX_PROG_NAME_LEN] = {0};
static __thread char g_binary_threadname[MAX_PROG_NAME_LEN] = {0};
static __thread char g_binary_hostname[MAX_HOSTNAME_LEN] = {0};
static __thread int g_binary_pid = 0;
static __thread enum LogLevel g_binary_level = DEFAULT_LOG_LEVEL;
static __thread int g_binary_fd = 2;  /* stderr fd is default as 2 */
/* safeguard calling init_logging multiple times */
static __thread int g_binary_is_logging_initialized = 0;

#define TOTAL_MSG_BYTES 1024
/* optimization by having only one instance per thread instead of
 * stack allocation all the time.
 */
static __thread char g_binary_total_message[TOTAL_MSG_BYTES];

/* account for partial write */
static __thread char g_binary_previous_message_offset = 0;
static __thread char g_binary_previous_message_bytes = 0;
static __thread char g_binary_previous_message[TOTAL_MSG_BYTES];

/* store the number of message dropped as a counter for
 * later statistics collection.
 */
static __thread uint64_t g_binary_num_msg_drops = 0;

int
clogging_binary_init(const char *progname, const char *threadname,
		     enum LogLevel level, int fd)
{
	int rc = 0;

	if (g_binary_is_logging_initialized > 0) {
		fprintf(stderr, "logging is already initialized or in the"
			" process of initialization.\n");
		return -1;
	}

	/* first thing to do is block any other thread in running logging
	 * initialization (if that is done, though its bad and should be
	 * done in the main thread ONLY.
	 */
	g_binary_is_logging_initialized = 1;

	/* Intentionally call the method so that any function static variables
	 * in get_log_level_as_cstring() are correctly initialized.
	 * Note that this should be done in any logging implementation (like
	 * many other things), although the argument and the return values
	 * are unimportant.
	 */
	(void)get_log_level_as_cstring(LOG_LEVEL_ERROR);

	strncpy(g_binary_progname, progname, MAX_PROG_NAME_LEN);
	strncpy(g_binary_threadname, threadname, MAX_PROG_NAME_LEN);
	rc = gethostname(g_binary_hostname, MAX_HOSTNAME_LEN);
	if (rc < 0) {
		strncpy(g_binary_hostname, "unknown", MAX_HOSTNAME_LEN);
	}
	g_binary_pid = (int)getpid();
	g_binary_level = level;
	g_binary_fd = fd;

	return 0;
}

void
clogging_binary_set_loglevel(enum LogLevel level)
{
	g_binary_level = level;
}

enum LogLevel
clogging_binary_get_loglevel(void)
{
	return g_binary_level;
}

void
clogging_binary_logmsg(const char *filename, const char *funcname,
		       int linenum, enum LogLevel level,
		       const char *format, ...)
{
	/* ISO 8601 date and time format with sec */
	const int time_str_len = 26;
	char time_str[time_str_len];
	time_t now;
	int remaining_bytes = 0;
	int len = 0;
	int rc = 0;
	const char *level_str = 0;
	char msg[MAX_LOG_MSG_LEN];
	va_list ap;

	/* ignore logs which are filtered out */
	if (level > g_binary_level) {
		return;
	}

	if (g_binary_is_logging_initialized <= 0) {
		fprintf(stderr, "logging is not initialized yet\n");
		++g_binary_num_msg_drops;
		return;
	}

	remaining_bytes =
		(g_binary_previous_message_bytes - g_binary_previous_message_offset);
	if (len > 0) {
		len = write(g_binary_fd,
			    &g_binary_previous_message[
				    g_binary_previous_message_offset],
			    remaining_bytes);
		if (len <= 0) {
			/* cannot write a thing, so drop the current message
			 */
			++g_binary_num_msg_drops;
			return;
		}
		if (len < remaining_bytes) {
			g_binary_previous_message_offset += len;
			/* since this time as well it was partial write
			 * so new message can anyway not be written.
			 * There is no other option other than dropping the
			 * current message.
			 */
			++g_binary_num_msg_drops;
			return;
		} else {
			/* previous message is written completely */
			g_binary_previous_message_bytes = 0;
			g_binary_previous_message_offset = 0;
		}
	}

	time(&now);
	len = time_to_cstr(&now, time_str, time_str_len);
	if (len < 0) {
		/* huh! I'd like to crash at this point but
		 * lets just log the message, which is a must.
		 */
		++g_binary_num_msg_drops;
		return;
	}

	level_str = get_log_level_as_cstring(level);

	va_start(ap, format);
	rc = vsnprintf(msg, MAX_LOG_MSG_LEN, format, ap);
	if (rc < 0) {
		/* cannot recover from this one, so lets just ignore it
		 * for now rather than logging it somewhere.
		 */
		++g_binary_num_msg_drops;
		return;
	}
	va_end(ap);

	/* <HEADER> <MESSAGE>
	 *	<HEADER> = <TIMESTAMP> <HOSTNAME>
	 *	<MESSAGE> = <TAG> <LEVEL> <CONTENT>
	 *		<TAG> = <PROGRAM><THREAD>[<PID>]
	 *		<LEVEL> = DEBUG | INFO | WARNING | ERROR
	 *		<CONTENT> = <FILE>__<FUNCTION/MODULE>(<LINENUM>): <APPLICATION_MESSAGE>
	 */
	/* leave the first two bytes for size */
	len = snprintf(&g_binary_total_message[2], TOTAL_MSG_BYTES,
		       "%s %s %s%s[%d] %s %s__%s(%d): %s\n", time_str,
		       g_binary_hostname, g_binary_progname, g_binary_threadname, g_binary_pid,
		       level_str, filename, funcname, linenum, msg);
	if (len < 0) {
		/* there is nothing much we can do, so return.  */
		++g_binary_num_msg_drops;
		return;
	}
	/* Note that the null character at the end is not part of the len */
	/* encode the length in big-endian format */
	g_binary_total_message[0] = (len >> 8) & 0x00ff;
	g_binary_total_message[1] = (len & 0x00ff);
	write(g_binary_fd, g_binary_total_message, len + 2);
}

uint64_t
clogging_binary_get_num_dropped_messages(void)
{
	return g_binary_num_msg_drops;
}

#ifdef __cplusplus
}
#endif