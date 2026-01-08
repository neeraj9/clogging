/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifndef CLOGGING_LOGGING_H
#define CLOGGING_LOGGING_H

#include "fd_logging.h"

#define SET_LOG_LEVEL(level) clogging_fd_set_loglevel(level)
#define GET_LOG_LEVEL() clogging_fd_get_loglevel()
#define GET_NUM_DROPPED_MESSAGES()                                       \
  clogging_fd_get_num_dropped_messages()

 /* Lets follow the ISO C standard of 1999 and use ## __VA_ARGS__ so as
 * to avoid the neccessity of providing even a single argument after
 * format. That is its possible that the user did not provide any
 * variable arguments and the format is the entier message.
 */
#define LOG_ERROR(format, ...)                                           \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_ERROR, format,     \
                        ##__VA_ARGS__)
#define LOG_WARN(format, ...)                                            \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_WARN, format,      \
                        ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                            \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_INFO, format,      \
                        ##__VA_ARGS__)

                        
/* If DISABLE_DEBUG_LOGS is defined then the DEBUG logs are compiled out
 * and cannot be switched on dynamically. This is done to ensure that
 * production system have DEBUG logs compiled out and save critical
 * cycles which otherwise would be wasted in function call to basic_logmsg()
 * and then returning when log level is below DEBUG.
 */
#ifndef DISABLE_DEBUG_LOGS
#define LOG_DEBUG(format, ...)                                           \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_DEBUG, format,     \
                        ##__VA_ARGS__)
#else
/* If not verbose then dont even compile the DEBUG logs, which are typically
 * cpu intensive function calls in the critical path.
 * If you want dynamicity then use INFO instead and set the log level to
 * WARN, which can be changed to INFO dynamically anytime.
 */
#define LOG_DEBUG(format, ...)
#endif


#endif /* CLOGGING_LOGGING_H */
