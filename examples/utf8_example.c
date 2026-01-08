/*
 * UTF-8 Logging Example
 * 
 * This example demonstrates using clogging with UTF-8 strings.
 * Compile and run with: cmake -DCLOGGING_USE_UTF8_STRINGS=ON
 */

#include "logging.h"

#include <stdio.h>

#define SET_LOG_LEVEL(level) clogging_basic_set_loglevel(level)
#define GET_LOG_LEVEL() clogging_basic_get_loglevel()
#define GET_NUM_DROPPED_MESSAGES()                                       \
  clogging_basic_get_num_dropped_messages()

/* Lets follow the ISO C standard of 1999 and use ## __VA_ARGS__ so as
 * to avoid the neccessity of providing even a single argument after
 * format. That is its possible that the user did not provide any
 * variable arguments and the format is the entier message.
 */
#define LOG_ERROR(format, ...)                                           \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_ERROR, format,     \
                        ##__VA_ARGS__)
#define LOG_WARN(format, ...)                                            \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_WARN, format,      \
                        ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                            \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_INFO, format,      \
                        ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)                                           \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_DEBUG, format,     \
                        ##__VA_ARGS__)

int main(void) {
  printf("UTF-8 Logging Example\n");
  printf("====================\n\n");

  /* Initialize logging with UTF-8 support enabled */
  clogging_basic_init("utf8_demo", "", LOG_LEVEL_INFO, NULL);

  /* Log ASCII text */
  LOG_INFO("Hello World!");

  /* Log with international characters (2-byte UTF-8) */
  LOG_INFO("Caf\xC3\xA9"); /* café */
  LOG_INFO("Se\xC3\xB1or"); /* Señor */

  /* Log with CJK characters (3-byte UTF-8) */
  LOG_INFO("Hello \xE4\xB8\xAD\xE6\x96\x87"); /* Hello 中文 */
  LOG_INFO("Konnichiwa \xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF"); /* こんにちは */

  /* Log with Hebrew/Arabic (3-byte UTF-8) */
  LOG_INFO("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D \xD7\xA2\xD7\x95\xD7\x9C\xD7\x9D"); /* שלום עולם */
  LOG_INFO("\xD8\xAD\xD8\xA7\xD9\x84\xD9\x82 \xD8\xA7\xD9\x84\xD8\xB9\xD8\xA7\xD9\x84\xD9\x85"); /* مرحبا العالم */

  /* Log with emoji (4-byte UTF-8) */
  LOG_INFO("Rocket emoji: \xF0\x9F\x9A\x80"); /* 🚀 */
  LOG_INFO("Smiling face: \xF0\x9F\x98\x80"); /* 😀 */
  LOG_INFO("Party popper: \xF0\x9F\x8E\x89"); /* 🎉 */

  /* Log with various symbols */
  LOG_INFO("Mathematical symbols: \xE2\x88\x9E \xE2\x88\x91 \xE2\x88\xAB"); /* ∞ ∑ ∫ */
  LOG_INFO("Check mark: \xE2\x9C\x93"); /* ✓ */
  LOG_INFO("Cross mark: \xE2\x9C\x97"); /* ✗ */

  /* Format strings with variables */
  LOG_INFO("Status code: %d, Message: %s", 200, "OK");
  LOG_INFO("User: %s from %s", "caf\xC3\xA9", "Spain"); /* café from Spain */

  /* Change log level and test DEBUG output */
  SET_LOG_LEVEL(LOG_LEVEL_DEBUG);
  LOG_DEBUG("Debug message with emoji: \xF0\x9F\x90\x9B"); /* 🐛 */

  printf("\n✓ All examples logged successfully!\n");
  return 0;
}
