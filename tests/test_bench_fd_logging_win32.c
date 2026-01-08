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

#include <assert.h>     /* assert() */
#include <stdio.h>
#include <stdlib.h>     /* atoi(), malloc(), free() */
#include <string.h>     /* strncmp() */
#include <stdint.h>     /* intptr_t */

/* Use the WIN32_LEAN_AND_MEAN macro before including windows.h. This tells the
Windows header to exclude less commonly used APIs, including the old winsock.h:
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>    /* CreateThread(), WaitForMultipleObjects() */
#include <winsock2.h>   /* socket(), connect(), etc. */
#include <ws2tcpip.h>   /* inet_pton(), etc. */
#pragma comment(lib, "ws2_32.lib")

typedef int socklen_t;

#define MAX_BUF_SIZE 4096
#define MAX_THREADS 100000
#define MAX_THREADNAME_SIZE 20
#define MAX_PROCESSNAME_SIZE 32 /* must be at least 16 */
#define MAX_PROCESSES 1  /* Windows implementation uses single process */

struct context {
  const char *processname;
  uint8_t processname_len;
  int threadindex;
  int num_loops;
  SOCKET fd;
};

DWORD WINAPI work(LPVOID data) {
  struct context *ctx = (struct context *)data;
  int i = 0;
  char threadname[MAX_THREADNAME_SIZE];
  int threadname_len = snprintf(threadname, MAX_THREADNAME_SIZE, "thread-%d", ctx->threadindex);
  assert(threadname_len > 0 && threadname_len < MAX_THREADNAME_SIZE);

  FD_INIT_LOGGING(ctx->processname, ctx->processname_len, threadname, (uint8_t)threadname_len, LOG_LEVEL_INFO, (int)ctx->fd);

  for (i = 0; i < ctx->num_loops; ++i) {
    FD_LOG_INFO("Some log which gets printed to console.");
  }
  return 0;
}

int runall(const char *pname, uint8_t processname_len, int num_processes, int num_threads, int num_loops,
           SOCKET fd) {
  /* Windows implementation: no fork(), use threads only */
  int j = 0;
  HANDLE *thread_handles = (HANDLE *)malloc(sizeof(HANDLE) * num_threads);
  struct context *thread_contexts =
      (struct context *)malloc(sizeof(struct context) * num_threads);

  FD_INIT_LOGGING(pname, processname_len, "", 0, LOG_LEVEL_DEBUG, (int)fd);

  FD_LOG_INFO("Benchmarking starts");
  FD_LOG_INFO("pname = %s, np = %d, nt = %d, nl = %d\n", pname, num_processes,
              num_threads, num_loops);

  for (j = 0; j < num_threads; j++) {
    thread_contexts[j].processname = pname;
    thread_contexts[j].processname_len = processname_len;
    thread_contexts[j].threadindex = j;
    thread_contexts[j].num_loops = num_loops;
    thread_contexts[j].fd = fd;
    thread_handles[j] = CreateThread(NULL, 0, work, (void *)&thread_contexts[j],
                                     0, NULL);
    if (thread_handles[j] == NULL) {
      FD_LOG_ERROR("CreateThread() failed");
    }
  }

  for (j = 0; j < num_threads; j++) {
    WaitForSingleObject(thread_handles[j], INFINITE);
    CloseHandle(thread_handles[j]);
  }

  free(thread_contexts);
  free(thread_handles);
  FD_LOG_INFO("Test complete");

  return 0;
}

void start_dummy_udp_server(int port) {
  char buf[MAX_BUF_SIZE];
  struct sockaddr_in addr;
  struct sockaddr_in clientaddr;
  socklen_t clientaddr_len = sizeof(struct sockaddr_in);
  SOCKET fd = INVALID_SOCKET;
  int rc = 0;
  int bytes_received = 0;

  assert(port > 0 && port < 65536);

  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2, 2), &wsa_data);

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons((u_short)port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  rc = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  assert(rc == 0);
  while (1) {
    bytes_received = recvfrom(fd, buf, MAX_BUF_SIZE, 0,
                              (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (bytes_received < 0) {
      fprintf(stderr, "goodbye!\n");
      break;
    }
    if (strncmp(buf, "end-of-test", bytes_received) == 0) {
      break;
    }
  }

  closesocket(fd);
  WSACleanup();
}

/*
 *
 * When no arguments are provided then it uses stdout for
 * testing, while use the following command line to test
 * with udp fd for logging.
 *
 * A udp server will started in another thread
 * which will run till it receives the terminating
 * message.
 *
 * test_bench_fd_logging.exe 1 1 10 "127.0.0.1" 20000
 *
 */
int main(int argc, const char *argv[]) {
  int rc = 0;
  char pname[MAX_PROCESSNAME_SIZE] = {0};
  int num_loops = 0;
  int num_processes = 0;
  int num_threads = 0;
  int port = 0;
  SOCKET fd = INVALID_SOCKET;
  int is_udp_testing = 0;
  HANDLE udp_server_thread_handle = NULL;

  /* Windows: use program name or default */
  clogging_strtcpy(pname, "test_bench_fd_logging", MAX_PROCESSNAME_SIZE);

  if (argc < 6) {
    num_loops = 10;
    num_processes = 1;
    num_threads = 2;
    fd = 1; /* out to stdout */
  } else {
    struct sockaddr_in addr;
    WSADATA wsa_data;

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

    assert(port > 0 && port < 65536);

    /* Windows: Initialize Winsock */
    rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (rc != 0) {
      fprintf(stderr, "WSAStartup() failed\n");
      return 1;
    }

    /* Windows: Create a thread for UDP server instead of fork */
    udp_server_thread_handle = CreateThread(NULL, 0, 
                                            (LPTHREAD_START_ROUTINE)start_dummy_udp_server,
                                            (LPVOID)(intptr_t)port, 0, NULL);
    if (udp_server_thread_handle == NULL) {
      fprintf(stderr, "CreateThread() failed\n");
      return 1;
    }

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);

    /* Windows: use inet_pton for IPv4 address parsing */
    rc = inet_pton(AF_INET, argv[4], &addr.sin_addr.s_addr);
    if (rc <= 0) {
      fprintf(stderr, "invalid IPv4 address ip = %s\n", argv[4]);
      closesocket(fd);
      WSACleanup();
      return 3;
    }

    /* Windows: Set non-blocking mode */
    unsigned long nonblock_mode = 1;
    rc = ioctlsocket(fd, FIONBIO, &nonblock_mode);
    if (rc == SOCKET_ERROR) {
      fprintf(stderr, "ioctlsocket() failed\n");
      closesocket(fd);
      WSACleanup();
      return 4;
    }

    rc = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
      fprintf(stderr,
              "cannot connect to ip = %s, port ="
              "%d\n",
              argv[4], port);
      closesocket(fd);
      WSACleanup();
      return 4;
    }
  }

  runall(pname, MAX_PROCESSNAME_SIZE, num_processes, num_threads, num_loops, fd);

  if (is_udp_testing) {
    char endmsg[] = "end-of-test";
    send(fd, endmsg, sizeof(endmsg), 0);
    /* send a couple of times so that a packet loss (if at all)
     * can be worked around and we dont get stuck.
     */
    send(fd, endmsg, sizeof(endmsg), 0);
    send(fd, endmsg, sizeof(endmsg), 0);
    /* Windows: wait for thread and cleanup */
    WaitForSingleObject(udp_server_thread_handle, 2000);
    CloseHandle(udp_server_thread_handle);
    closesocket(fd);
    WSACleanup();
  }

  return 0;
}
