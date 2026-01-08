/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#include "../src/basic_logging.h"

#include <assert.h>  /* assert() */
#include <pthread.h> /* pthread_create() and friends */
#include <stdio.h>
#include <stdlib.h>    /* atoi(), malloc(), free() */
#include <sys/prctl.h> /* prctl() */
#include <sys/wait.h>  /* wait() */
#include <unistd.h>    /* fork() */

#define MAX_THREADS 100000
#define MAX_THREADNAME_SIZE 20
#define MAX_PROCESSNAME_SIZE 32 /* must be at least 16 */

struct context {
  const char *processname;
  int threadindex;
  int num_loops;
};

void *work(void *data) {
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
  long i;
  pid_t pid;

  BASIC_INIT_LOGGING(pname, "", LOG_LEVEL_DEBUG);

  BASIC_LOG_INFO("Benchmarking starts");
  BASIC_LOG_INFO("pname = %s, np = %d, nt = %d, nl = %d\n", pname,
                 num_processes, num_threads, num_loops);

  for (i = 0; i < num_processes; i++) {
    pid = fork();
    if (pid < 0) {
      BASIC_LOG_ERROR("fork() failed");
    } else if (pid == 0) {
      /* within child process */
      pthread_t tids[num_threads];
      struct context *thread_contexts =
          (struct context *)malloc(sizeof(struct context) * MAX_THREADS);
      int j = 0;

      for (j = 0; j < num_threads; j++) {
        thread_contexts[j].processname = pname;
        thread_contexts[j].threadindex = j;
        thread_contexts[j].num_loops = num_loops;
        pthread_create(&(tids[j]), NULL, work, (void *)&thread_contexts[j]);
      }
      for (j = 0; j < num_threads; j++) {
        pthread_join(tids[j], NULL);
      }
      free(thread_contexts);
      return 0;
    }
  }

  for (i = 0; i < num_processes; i++) {
    pid = wait(NULL);
  }
  BASIC_LOG_INFO("Test complete");

  return 0;
}

int main(int argc, char **argv) {
  int rc = 0;
  char pname[MAX_PROCESSNAME_SIZE] = {0};
  int num_loops = 0;
  int num_processes = 0;
  int num_threads = 0;

  rc = prctl(PR_GET_NAME, (unsigned long)(pname), 0, 0, 0);
  assert(rc == 0);

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
