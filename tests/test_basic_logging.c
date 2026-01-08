/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#include "logging.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  (void)argc;  /* unused parameter */
  (void)argv;  /* unused parameter */

  clogging_basic_init(argv[0], (uint8_t)(strlen(argv[0]) + 1), "", 0, LOG_LEVEL_DEBUG, NULL);
  LOG_DEBUG("A basic debug log looks like this");
  assert(GET_LOG_LEVEL() == LOG_LEVEL_DEBUG);
  SET_LOG_LEVEL(LOG_LEVEL_INFO);
  assert(GET_LOG_LEVEL() == LOG_LEVEL_INFO);
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  return 0;
}
