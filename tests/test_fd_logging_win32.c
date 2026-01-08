/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifndef _WIN32
#error This file is for Windows platforms only
#endif /* !_WIN32 */

#include "../src/fd_logging.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* maximum size for process name */
#define MAX_SIZE 32

/* Lets follow the ISO C standard of 1999 and use ## __VA_ARGS__ so as
 * to avoid the neccessity of providing even a single argument after
 * format. That is its possible that the user did not provide any
 * variable arguments and the format is the entier message.
 */
#define LOG_ERROR(format, ...)                                         \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_ERROR, format,      \
                        ##__VA_ARGS__)
#define LOG_WARN(format, ...)                                          \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_WARN, format,       \
                        ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                          \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_INFO, format,       \
                        ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)                                         \
  clogging_fd_logmsg(__func__, __LINE__, LOG_LEVEL_DEBUG, format,      \
                        ##__VA_ARGS__)

int main(int argc, char *argv[]) {
  (void)argc;  /* unused parameter */
  (void)argv;  /* unused parameter */
  int fd = 1; /* fd for stdout */
  char pname[MAX_SIZE] = {0};

  /* Windows: use program name or default */
  clogging_strtcpy(pname, "test_fd_logging", MAX_SIZE);

  /* printf("pname = %s\n", pname); */
  /* printf("argv[0] = %s\n", argv[0]); */
  clogging_fd_init(pname, MAX_SIZE, "", 0, LOG_LEVEL_DEBUG, clogging_create_handle_from_fd(fd), NULL);
  LOG_DEBUG("A fd debug log looks like this");
  assert(clogging_fd_get_loglevel() == LOG_LEVEL_DEBUG);
  clogging_fd_set_loglevel(LOG_LEVEL_INFO);
  assert(clogging_fd_get_loglevel() == LOG_LEVEL_INFO);
  assert(clogging_fd_get_num_dropped_messages() == 0);
  return 0;
}
