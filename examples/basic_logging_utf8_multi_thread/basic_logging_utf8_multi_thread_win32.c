/*
 * Basic Logging UTF-8 Multi-Thread Example (Windows)
 *
 * This example demonstrates using clogging basic_logging with UTF-8 strings
 * in multiple threads. Each thread initializes logging with its own thread name.
 * Compile with: cmake -B build -DCLOGGING_USE_UTF8_STRINGS=ON
 * Build with: cmake --build build
 * Run with: .\build\examples\basic_logging_utf8_multi_thread\basic_logging_utf8_multi_thread
 */

#include "logging.h"

#ifndef _WIN32
#error This file is for Windows platforms only
#endif /* !_WIN32 */

#include <stdio.h>
#include <windows.h>
#include <string.h>

typedef struct {
  int thread_id;
  const char *thread_name;
  const char *program_name;
} thread_args_t;

DWORD WINAPI thread_function(LPVOID lpParam) {
  thread_args_t* args = (thread_args_t*)lpParam;
  
  /* Initialize basic logging with UTF-8 support enabled for this thread */
  clogging_basic_init(args->program_name, args->thread_name, LOG_LEVEL_INFO, NULL);

  LOG_INFO("--- Thread %d (%s) started ---", args->thread_id, args->thread_name);

  /* Log ASCII text */
  LOG_INFO("[%s] Hello World!", args->thread_name);

  /* Log with international characters (2-byte UTF-8) */
  LOG_INFO("[%s] Caf\xC3\xA9", args->thread_name); /* café */
  LOG_INFO("[%s] Se\xC3\xB1or", args->thread_name); /* Señor */

  /* Log with CJK characters (3-byte UTF-8) */
  LOG_INFO("[%s] Hello \xE4\xB8\xAD\xE6\x96\x87", args->thread_name); /* Hello 中文 */
  LOG_INFO("[%s] Konnichiwa \xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF", args->thread_name); /* こんにちは */

  /* Log with emoji (4-byte UTF-8) */
  LOG_INFO("[%s] Rocket emoji: \xF0\x9F\x9A\x80", args->thread_name); /* 🚀 */
  LOG_INFO("[%s] Smiling face: \xF0\x9F\x98\x80", args->thread_name); /* 😀 */

  /* Format strings with variables */
  LOG_INFO("[%s] Status: %d, Message: %s", args->thread_name, 200, "OK");
  LOG_INFO("[%s] User: %s from %s", args->thread_name, "caf\xC3\xA9", "Spain");

  LOG_INFO("--- Thread %d (%s) completed ---", args->thread_id, args->thread_name);

  return 0;
}

int main(int argc, char *argv[]) {
  printf("Basic Multi-Thread Logging UTF-8 Example\n");
  printf("==========================================\n");

  clogging_basic_init(argv[0], "-main", LOG_LEVEL_INFO, NULL);

  HANDLE thread1, thread2;
  thread_args_t args1 = {1, "thread_1_utf8", argv[0]};
  thread_args_t args2 = {2, "thread_2_utf8", argv[0]};

  /* Create first thread */
  thread1 = CreateThread(NULL, 0, thread_function, &args1, 0, NULL);
  if (thread1 == NULL) {
    fprintf(stderr, "Error creating thread 1\n");
    return 1;
  }

  /* Create second thread */
  thread2 = CreateThread(NULL, 0, thread_function, &args2, 0, NULL);
  if (thread2 == NULL) {
    fprintf(stderr, "Error creating thread 2\n");
    CloseHandle(thread1);
    return 1;
  }

  /* Wait for both threads to complete */
  WaitForSingleObject(thread1, INFINITE);
  WaitForSingleObject(thread2, INFINITE);

  /* Close thread handles */
  CloseHandle(thread1);
  CloseHandle(thread2);

  LOG_INFO("All threads completed. Logging demonstration finished.");

  return 0;
}
