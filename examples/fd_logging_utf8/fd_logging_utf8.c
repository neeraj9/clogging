/*
 * FD Logging UTF-8 Example
 *
 * This example demonstrates using clogging fd_logging (file descriptor logging)
 * with UTF-8 strings. Logs are written to a file descriptor.
 * Compile with: cmake -B build -DCLOGGING_USE_UTF8_STRINGS=ON
 * Build with: cmake --build build
 * Run with: ./build/examples/fd_logging_utf8/fd_logging_utf8
 */

#include "logging.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("FD Logging UTF-8 Example\n");
  printf("========================\n\n");

  /* Initialize FD logging to write to stdout (file descriptor 1) */
  clogging_handle_t stdout_handle;
#ifdef _WIN32
  stdout_handle = clogging_create_handle_from_fd(1);
#else
  stdout_handle = 1;
#endif
  FD_INIT_LOGGING("fd_utf8_demo", 12 + 1, "", 0, LOG_LEVEL_INFO, stdout_handle, NULL);

  /* Log ASCII text */
  LOG_INFO("Hello World!");

  /* Log with international characters (2-byte UTF-8) */
  LOG_INFO("Caf\xC3\xA9"); /* café */
  LOG_INFO("Se\xC3\xB1or"); /* Señor */
  LOG_INFO("H\xC3\xA9llo");  /* Héllo */

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

  LOG_WARN("Warning with special chars: \xC2\xA9 \xC2\xAE \xE2\x84\xA2"); /* © ® ™ */

  LOG_ERROR("Error with UTF-8 message: \xF0\x9F\x98\xA1"); /* 😡 */

  printf("\nLogging complete. Check output above.\n");

  return 0;
}
