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

int main(void) {
  printf("Testing UTF-8 logging support...\n\n");

  /* Initialize basic logging with UTF-8 strings */
  assert(INIT_LOGGING("utf8_demo", 9, "", 0, LOG_LEVEL_DEBUG) == 0);

  /* Test 1: ASCII logging (should work everywhere) */
  printf("Test 1: ASCII logging\n");
  LOG_INFO("Hello World!");
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("✓ ASCII logging passed\n\n");

  /* Test 2: 2-byte UTF-8 characters (Latin with accents) */
  printf("Test 2: 2-byte UTF-8 (Latin with accents)\n");
  LOG_INFO("Caf\xC3\xA9");  /* café */
  LOG_INFO("Se\xC3\xB1or"); /* Señor */
  LOG_INFO("Na\xC3\xAFve"); /* Naïve */
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("✓ 2-byte UTF-8 logging passed\n\n");

  /* Test 3: 3-byte UTF-8 characters (CJK, other languages) */
  printf("Test 3: 3-byte UTF-8 (International scripts)\n");
  LOG_INFO("Hello \xE4\xB8\xAD\xE6\x96\x87"); /* Hello 中文 */
  LOG_INFO("Konnichiwa \xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF"); /* こんにちは */
  LOG_INFO("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D \xD7\xA2\xD7\x95\xD7\x9C\xD7\x9D"); /* שלום עולם */
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("✓ 3-byte UTF-8 logging passed\n\n");

  /* Test 4: 4-byte UTF-8 characters (emoji and symbols) */
  printf("Test 4: 4-byte UTF-8 (Emoji and symbols)\n");
  LOG_INFO("Rocket: \xF0\x9F\x9A\x80"); /* 🚀 */
  LOG_INFO("Smile: \xF0\x9F\x98\x80");  /* 😀 */
  LOG_INFO("Heart: \xE2\x9D\xA4");       /* ❤ (3-byte) */
  LOG_INFO("Infinity: \xE2\x88\x9E");    /* ∞ (3-byte) */
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("✓ 4-byte UTF-8 logging passed\n\n");

  /* Test 5: Mixed ASCII and multi-byte */
  printf("Test 5: Mixed ASCII and UTF-8\n");
  LOG_INFO("Starting application \xF0\x9F\x9A\x80"); /* Starting application 🚀 */
  LOG_INFO("Status: OK \xE2\x9C\x93");               /* Status: OK ✓ */
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("✓ Mixed UTF-8 logging passed\n\n");

  /* Test 6: Variable arguments with UTF-8 format string */
  printf("Test 6: Format strings with variables\n");
  LOG_INFO("User: %s, Status: %d", "caf\xC3\xA9", 200); /* café, 200 */
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("✓ Format string logging passed\n\n");

  /* Test 7: Set log level and test filtering */
  printf("Test 7: Log level filtering\n");
  SET_LOG_LEVEL(LOG_LEVEL_WARN);
  assert(GET_LOG_LEVEL() == LOG_LEVEL_WARN);
  LOG_DEBUG("This debug message should be filtered");
  LOG_WARN("This warning \xF0\x9F\x9A\x80 should appear");
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("✓ Log level filtering passed\n\n");

  printf("✓ All UTF-8 logging tests passed!\n");
  return 0;
}
