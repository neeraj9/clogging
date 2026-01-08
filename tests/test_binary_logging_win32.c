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

#include "test_binary_logging_common.h"

#ifndef __FILE__
#error __FILE__ not defined
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>    /* Windows API */
#include <winsock2.h>   /* socket(), connect(), etc. */
#include <ws2tcpip.h>   /* inet_pton(), etc. */
#pragma comment(lib, "ws2_32.lib")

#include <assert.h>    /* assert() */
#include <stdio.h>
#include <string.h>     /* strlen() */
#include <time.h>       /* time_t */

typedef int socklen_t;

/* maximum size for process name */
#define MAX_SIZE 32

#define MAX_BUF_LEN 1024



int bigendian_to_native(const char *buf, int bytes, char *dst);
int read_nbytes(const char *buf, int bytes, unsigned long long int *llval);
inline int read_length(const char *buf, int *offset, int *bytes);

/* create udp server and return the socket fd */
SOCKET create_udp_server(int port) {
  struct sockaddr_in addr;
  SOCKET fd = INVALID_SOCKET;
  int rc = 0;

  assert(port > 0 && port < 65536);

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  addr.sin_family = AF_INET;
  addr.sin_port = htons((u_short)port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  rc = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  assert(rc == 0);

  return fd;
}

/* receive message from the client */
int receive_msg_from_client(SOCKET fd, char *buf, int buflen,
                            struct sockaddr_in *clientaddr) {
  socklen_t clientaddr_len = sizeof(struct sockaddr_in);
  int bytes_received = 0;

  bytes_received = recvfrom(fd, buf, buflen, 0, (struct sockaddr *)clientaddr,
                            &clientaddr_len);
  return bytes_received;
}

SOCKET create_client_socket(const char *ip, int port) {
  struct sockaddr_in addr;
  SOCKET fd = INVALID_SOCKET;
  int rc = 0;

  assert(port > 0 && port < 65536);

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  addr.sin_family = AF_INET;
  addr.sin_port = htons((u_short)port);
  rc = inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);
  if (rc <= 0) {
    fprintf(stderr, "invalid IPv4 address ip = %s\n", ip);
    return INVALID_SOCKET;
  }
  rc = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (rc < 0) {
    fprintf(stderr,
            "cannot connect to ip = %s, port ="
            "%d\n",
            ip, port);
    return INVALID_SOCKET;
  }
  return fd;
}

int analyze_received_binary_message(const char *format, const char *buf,
                                    int buflen);

int test_static_string(int argc, char *argv[]) {
  (void)argc;  /* unused parameter */
  (void)argv;  /* unused parameter */
  char pname[MAX_SIZE] = {0};
  SOCKET serverfd = INVALID_SOCKET;
  SOCKET clientfd = INVALID_SOCKET;
  int port = 21002;
  int rc = 0;
  int bytes_received = 0;
  char buf[MAX_BUF_LEN];
  char msg[] = "A fd debug log looks like this";
  struct sockaddr_in clientaddr;
  WSADATA wsa_data;

  /* Windows: use program name or default */
  clogging_strtcpy(pname, "test_binary_logging", MAX_SIZE);

  /* Initialize Winsock */
  rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (rc != 0) {
    fprintf(stderr, "WSAStartup() failed\n");
    return 1;
  }

  serverfd = create_udp_server(port);
  clientfd = create_client_socket("127.0.0.1", port);

  /* printf("pname = %s\n", pname); */
  /* printf("argv[0] = %s\n", argv[0]); */
  /* On Windows, use the proper socket handle API */
  BINARY_INIT_LOGGING(pname, MAX_SIZE, "", 0, LOG_LEVEL_DEBUG, clogging_create_handle_from_socket((uint64_t)clientfd));
  assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_DEBUG);
  BINARY_LOG_DEBUG(msg);

  bytes_received =
      receive_msg_from_client(serverfd, buf, MAX_BUF_LEN, &clientaddr);
  printf("msg sent size = %zd\n", strlen(msg));
  printf("bytes_received = %d\n", bytes_received);

  rc = analyze_received_binary_message(msg, buf, bytes_received);

  BINARY_SET_LOG_LEVEL(LOG_LEVEL_INFO);
  assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_INFO);
  assert(BINARY_GET_NUM_DROPPED_MESSAGES() == 0);

  closesocket(serverfd);
  closesocket(clientfd);
  WSACleanup();
  return 0;
}

int test_variable_arguments(int argc, char *argv[]) {
  (void)argc;  /* unused parameter */
  (void)argv;  /* unused parameter */
  char pname[MAX_SIZE] = {0};
  SOCKET serverfd = INVALID_SOCKET;
  SOCKET clientfd = INVALID_SOCKET;
  int port = 21002;
  int rc = 0;
  int bytes_received = 0;
  char buf[MAX_BUF_LEN];
  struct sockaddr_in clientaddr;
  WSADATA wsa_data;
  /* variable arguments with format */
  char format[] =
      "A fd debug log looks like this, int=%d, char=%c,"
      " uint=%u, longint=%ld, longlongint=%lld, unsignedlonglong=%llu,"
      " ptr=%p, str=%s";
  int argint = 1;
  char argchar = 2;
  unsigned int arguint = 3;
  long int arglongint = 4;
  long long int arglonglongint = 5;
  unsigned long long int argulonglongint = 6;
  void *argptr = &argint;
  char *argstr = format;

  /* Windows: use program name or default */
  clogging_strtcpy(pname, "test_binary_logging", MAX_SIZE);

  /* Initialize Winsock */
  rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (rc != 0) {
    fprintf(stderr, "WSAStartup() failed\n");
    return 1;
  }

  serverfd = create_udp_server(port);
  clientfd = create_client_socket("127.0.0.1", port);

  /* printf("pname = %s\n", pname); */
  /* printf("argv[0] = %s\n", argv[0]); */
  /* On Windows, use the proper socket handle API */
  BINARY_INIT_LOGGING(pname, MAX_SIZE, "", 0, LOG_LEVEL_DEBUG, clogging_create_handle_from_socket((uint64_t)clientfd));
  assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_DEBUG);
  BINARY_LOG_DEBUG(format, argint, argchar, arguint, arglongint, arglonglongint,
                   argulonglongint, argptr, argstr);

  bytes_received =
      receive_msg_from_client(serverfd, buf, MAX_BUF_LEN, &clientaddr);
  printf("SENT: format sent size = %zd\n", strlen(format));
  printf("SENT: argptr = %p, argstr = [%s]\n", argptr, argstr);
  printf("bytes_received = %d\n", bytes_received);

  rc = analyze_received_binary_message(format, buf, bytes_received);

  BINARY_SET_LOG_LEVEL(LOG_LEVEL_INFO);
  assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_INFO);
  assert(BINARY_GET_NUM_DROPPED_MESSAGES() == 0);

  closesocket(serverfd);
  closesocket(clientfd);
  WSACleanup();
  return 0;
}

int main(int argc, char *argv[]) {
  return test_variable_arguments(argc, argv);
  // return test_static_string(argc, argv);
}
