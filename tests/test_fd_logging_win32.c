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

int main(int argc, char *argv[]) {
  (void)argc;  /* unused parameter */
  (void)argv;  /* unused parameter */
  int fd = 1; /* fd for stdout */
  char pname[MAX_SIZE] = {0};

  /* Windows: use program name or default */
  clogging_strtcpy(pname, "test_fd_logging", MAX_SIZE);

  /* printf("pname = %s\n", pname); */
  /* printf("argv[0] = %s\n", argv[0]); */
  FD_INIT_LOGGING(pname, MAX_SIZE, "", 0, LOG_LEVEL_DEBUG, clogging_create_handle_from_fd(fd), NULL);
  FD_LOG_DEBUG("A fd debug log looks like this");
  assert(FD_GET_LOG_LEVEL() == LOG_LEVEL_DEBUG);
  FD_SET_LOG_LEVEL(LOG_LEVEL_INFO);
  assert(FD_GET_LOG_LEVEL() == LOG_LEVEL_INFO);
  assert(FD_GET_NUM_DROPPED_MESSAGES() == 0);
  return 0;
}
