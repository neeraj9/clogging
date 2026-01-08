/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifdef _WIN32
#error This file is for non-Windows platforms only
#endif /* _WIN32 */

#include "../src/fd_logging.h"

#include <assert.h>     /* assert() */
#include <arpa/inet.h>  /* inet_aton(), htons() */
#include <fcntl.h>      /* fcntl() */
#include <netinet/in.h> /* inet_in */
#include <pthread.h>    /* pthread_create() and friends */
#include <stdio.h>
#include <stdlib.h>     /* atoi(), malloc(), free() */
#include <string.h>     /* strncmp() */
#include <sys/prctl.h>  /* prctl() */
#include <sys/socket.h> /* socket(), connect() */
#include <sys/wait.h>   /* wait() */
#include <unistd.h>     /* fork() */

#define MAX_BUF_SIZE 4096
#define MAX_THREADS 100000
#define MAX_THREADNAME_SIZE 20
#define MAX_PROCESSNAME_SIZE 32 /* must be at least 16 */

struct context {
  const char *processname;
  int threadindex;
  int num_loops;
  int fd;
};

void *work(void *data) {
  struct context *ctx = (struct context *)data;
  int i = 0;
  char threadname[MAX_THREADNAME_SIZE];
  snprintf(threadname, MAX_THREADNAME_SIZE, "thread-%d", ctx->threadindex);

  FD_INIT_LOGGING(ctx->processname, threadname, LOG_LEVEL_INFO, ctx->fd);

  for (i = 0; i < ctx->num_loops; ++i) {
    FD_LOG_INFO("Some log which gets printed to console.");
  }
  return 0;
}

int runall(const char *pname, int num_processes, int num_threads, int num_loops,
           int fd) {
  /* POSIX implementation: use fork() and threads */
  long i;
  pid_t pid;

  FD_INIT_LOGGING(pname, "", LOG_LEVEL_DEBUG, fd);

  FD_LOG_INFO("Benchmarking starts");
  FD_LOG_INFO("pname = %s, np = %d, nt = %d, nl = %d\n", pname, num_processes,
              num_threads, num_loops);

  for (i = 0; i < num_processes; i++) {
    pid = fork();
    if (pid < 0) {
      FD_LOG_ERROR("fork() failed");
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
        thread_contexts[j].fd = fd;
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
  FD_LOG_INFO("Test complete");

  return 0;
}

void start_dummy_udp_server(int port) {
  char buf[MAX_BUF_SIZE];
  struct sockaddr_in addr;
  struct sockaddr_in clientaddr;
  socklen_t clientaddr_len = sizeof(struct sockaddr_in);
  int fd = 0;
  int rc = 0;
  int bytes_received = 0;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  rc = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  assert(rc == 0);
  while (1) {
    bytes_received = recvfrom(fd, buf, MAX_BUF_SIZE, 0,
                              (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (bytes_received < 0) {
      fprintf(stderr, "goodbye!\n");
      /* this should not happen since this is a blocking
       * socket
       */
      break;
    }
    if (strncmp(buf, "end-of-test", bytes_received) == 0) {
      break;
    }
  }

  close(fd);
}

/*
 *
 * When no arguments are provided then it uses stdout for
 * testing, while use the following command line to test
 * with udp fd for logging.
 *
 * A udp server will started in another process (via fork)
 * which will run till it receives the terminating
 * message.
 *
 * ./test_bench_fd_logging 1 1 10 "127.0.0.1" 20000
 *
 */
int main(int argc, const char *argv[]) {
  int rc = 0;
  char pname[MAX_PROCESSNAME_SIZE] = {0};
  int num_loops = 0;
  int num_processes = 0;
  int num_threads = 0;
  int port = 0;
  int fd = 0;
  int flags = 0;
  int is_udp_testing = 0;
  pid_t udp_server_pid;

  rc = prctl(PR_GET_NAME, (unsigned long)(pname), 0, 0, 0);
  assert(rc == 0);

  if (argc < 6) {
    num_loops = 10;
    num_processes = 1;
    num_threads = 2;
    fd = 1; /* out to stdout */
  } else {
    struct sockaddr_in addr;

    is_udp_testing = 1;

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
    port = atoi(argv[5]);

    udp_server_pid = fork();
    if (udp_server_pid < 0) {
      fprintf(stderr, "fork() failed\n");
      return 1;
    } else if (udp_server_pid == 0) {
      /* child */
      start_dummy_udp_server(port);
      return 0;
    }
    /* else parent */

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    rc = inet_aton(argv[4], &addr.sin_addr);
    if (rc < 0) {
      fprintf(stderr, "invalid IPv4 address ip = %s\n", argv[4]);
      return 3;
    }

    flags = fcntl(fd, F_GETFL, 0);
    assert(flags >= 0);
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    assert(rc >= 0);

    rc = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
      fprintf(stderr,
              "cannot connect to ip = %s, port ="
              "%d\n",
              argv[4], port);
      return 4;
    }
  }

  runall(pname, num_processes, num_threads, num_loops, fd);

  if (is_udp_testing) {
    char endmsg[] = "end-of-test";
    send(fd, endmsg, sizeof(endmsg), 0);
    /* send a couple of times so that a packet loss (if at all)
     * can be worked around and we dont get stuck.
     * Alternatively, we can use kill(2) as well.
     */
    send(fd, endmsg, sizeof(endmsg), 0);
    send(fd, endmsg, sizeof(endmsg), 0);
    /* wait for the udp server process to complete */
    (void)wait(NULL);
  }

  return 0;
}
