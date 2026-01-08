/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 * See LICENSE file for licensing information.
 */

#ifndef CLOGGING_LOGGING_H
#define CLOGGING_LOGGING_H

#include "basic_logging.h"

#define INIT_LOGGING BASIC_INIT_LOGGING
#define SET_LOG_LEVEL BASIC_SET_LOG_LEVEL
#define GET_LOG_LEVEL BASIC_GET_LOG_LEVEL
#define GET_NUM_DROPPED_MESSAGES BASIC_GET_NUM_DROPPED_MESSAGES

/* If DISABLE_DEBUG_LOGS is defined then the DEBUG logs are compiled out
 * and cannot be switched on dynamically. This is done to ensure that
 * production system have DEBUG logs compiled out and save critical
 * cycles which otherwise would be wasted in function call to basic_logmsg()
 * and then returning when log level is below DEBUG.
 */

#define LOG_ERROR BASIC_LOG_ERROR
#define LOG_WARN BASIC_LOG_WARN
#define LOG_INFO BASIC_LOG_INFO

#ifndef DISABLE_DEBUG_LOGS
#define LOG_DEBUG BASIC_LOG_DEBUG
#else
/* If not verbose then dont even compile the DEBUG logs, which are typically
 * cpu intensive function calls in the critical path.
 * If you want dynamicity then use INFO instead and set the log level to
 * WARN, which can be changed to INFO dynamically anytime.
 */
#define LOG_DEBUG(format, ...)
#endif


#endif /* CLOGGING_LOGGING_H */
