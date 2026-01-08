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

#include "../src/basic_logging.h"

#include <assert.h>  /* assert() */
#include <stdio.h>
#include <stdlib.h>    /* atoi(), malloc(), free() */
#include <string.h>    /* memset() */

/* Use the WIN32_LEAN_AND_MEAN macro before including windows.h. This tells the
Windows header to exclude less commonly used APIs, including the old winsock.h.
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   /* CreateThread(), WaitForMultipleObjects() */

#define MAX_THREADS 100000
#define MAX_THREADNAME_SIZE 20
#define MAX_PROCESSNAME_SIZE 32 /* must be at least 16 */
#define MAX_PROCESSES 1  /* Windows implementation uses single process */

struct context {
  const char *processname;
  int threadindex;
  int num_loops;
};

DWORD WINAPI work(LPVOID data) {
  struct context *ctx = (struct context *)data;
  int i = 0;
  char threadname[MAX_THREADNAME_SIZE];
  snprintf(threadname, MAX_THREADNAME_SIZE, "thread-%d", ctx->threadindex);

  BASIC_INIT_LOGGING(ctx->processname, threadname, LOG_LEVEL_INFO);

  for (i = 0; i < ctx->num_loops; ++i) {
    BASIC_LOG_INFO("Some log which gets printed to console.");
  }
  return 0;
}

int runall(const char *pname, int num_processes, int num_threads,
           int num_loops) {
  /* Windows implementation: no fork(), use threads only */
  int j = 0;
  HANDLE *thread_handles = (HANDLE *)malloc(sizeof(HANDLE) * num_threads);
  struct context *thread_contexts =
      (struct context *)malloc(sizeof(struct context) * num_threads);

  BASIC_INIT_LOGGING(pname, "", LOG_LEVEL_DEBUG);

  BASIC_LOG_INFO("Benchmarking starts");
  BASIC_LOG_INFO("pname = %s, np = %d, nt = %d, nl = %d\n", pname,
                 num_processes, num_threads, num_loops);

  for (j = 0; j < num_threads; j++) {
    thread_contexts[j].processname = pname;
    thread_contexts[j].threadindex = j;
    thread_contexts[j].num_loops = num_loops;
    thread_handles[j] = CreateThread(NULL, 0, work, (void *)&thread_contexts[j],
                                     0, NULL);
    if (thread_handles[j] == NULL) {
      BASIC_LOG_ERROR("CreateThread() failed");
    }
  }

  for (j = 0; j < num_threads; j++) {
    WaitForSingleObject(thread_handles[j], INFINITE);
    CloseHandle(thread_handles[j]);
  }

  free(thread_contexts);
  free(thread_handles);
  BASIC_LOG_INFO("Test complete");

  return 0;
}

int main(int argc, char **argv) {
  char pname[MAX_PROCESSNAME_SIZE] = {0};
  int num_loops = 0;
  int num_processes = 0;
  int num_threads = 0;

  /* Windows: use program name or default */
  strncpy_s(pname, MAX_PROCESSNAME_SIZE, "test_bench_basic_logging", MAX_PROCESSNAME_SIZE - 1);

  if (argc < 4) {
    num_loops = 10;
    num_processes = 1;
    num_threads = 2;
  } else {
    num_loops = atoi(argv[3]);
    num_processes = atoi(argv[1]);
    num_threads = atoi(argv[2]);
    if (num_threads > MAX_THREADS) {
      fprintf(stderr,
              "The maximum number of threads (%d)"
              " exceeded. Please provide a lower value\n",
              MAX_THREADS);
      return 2;
    }
  }

  runall(pname, num_processes, num_threads, num_loops);

  return 0;
}
