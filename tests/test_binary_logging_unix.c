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

#include "test_binary_logging_common.h"

#ifndef __FILE__
#error __FILE__ not defined
#endif

#include <arpa/inet.h> /* inet_aton(), htons() */
#include <assert.h>    /* assert() */
#include <fcntl.h>     /* fcntl() */
#include <stdio.h>
#include <string.h>     /* strlen() */
#include <sys/prctl.h>  /* prctl() */
#include <sys/socket.h> /* socket(), connect() */
#include <time.h>       /* time_t */

/* as per man prctl(2) the size should be at least 16 bytes */
#define MAX_SIZE 32

#define MAX_BUF_LEN 1024

/* create udp server and return the socket fd */
int create_udp_server(int port) {
  struct sockaddr_in addr;
  int fd = 0;
  int rc = 0;
  int flags = 0;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  rc = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  assert(rc == 0);

  flags = fcntl(fd, F_GETFL, 0);
  assert(flags >= 0);
  rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  assert(rc >= 0);

  return fd;
}

/* receive message from the client */
int receive_msg_from_client(int fd, char *buf, int buflen,
                            struct sockaddr_in *clientaddr) {
  socklen_t clientaddr_len = sizeof(struct sockaddr_in);
  int bytes_received = 0;

  bytes_received = recvfrom(fd, buf, buflen, 0, (struct sockaddr *)clientaddr,
                            &clientaddr_len);
  return bytes_received;
}

int create_client_socket(const char *ip, int port) {
  struct sockaddr_in addr;
  int fd = 0;
  int flags = 0;
  int rc = 0;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  rc = inet_aton(ip, &addr.sin_addr);
  if (rc < 0) {
    fprintf(stderr, "invalid IPv4 address ip = %s\n", ip);
    return -1;
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
            ip, port);
    return -1;
  }
  return fd;
}

int test_static_string(int argc, char *argv[]) {
  (void)argc;  /* unused parameter */
  (void)argv;  /* unused parameter */

  char pname[MAX_SIZE] = {0};
  int serverfd = 0;
  int clientfd = 0;
  int port = 21002;
  int rc = 0;
  int bytes_received = 0;
  char buf[MAX_BUF_LEN];
  char format[] = "A fd debug log looks like this";
  struct sockaddr_in clientaddr;

  rc = prctl(PR_GET_NAME, (unsigned long)(pname), 0, 0, 0);
  assert(rc == 0);

  serverfd = create_udp_server(port);
  clientfd = create_client_socket("127.0.0.1", port);

  /* printf("pname = %s\n", pname); */
  /* printf("argv[0] = %s\n", argv[0]); */
  clogging_binary_init(pname, MAX_SIZE, "", 0, LOG_LEVEL_DEBUG, clogging_create_handle_from_fd(clientfd));
  assert(clogging_binary_get_loglevel() == LOG_LEVEL_DEBUG);
  LOG_DEBUG(format);

  bytes_received =
      receive_msg_from_client(serverfd, buf, MAX_BUF_LEN, &clientaddr);
  printf("format sent size = %zd\n", strlen(format));
  printf("bytes_received = %d\n", bytes_received);

  rc = analyze_received_binary_message(format, buf, bytes_received);

  clogging_binary_set_loglevel(LOG_LEVEL_INFO);
  assert(clogging_binary_get_loglevel() == LOG_LEVEL_INFO);
  assert(clogging_binary_get_num_dropped_messages() == 0);
  return 0;
}

int test_variable_arguments(int argc, char *argv[]) {
  (void)argc;  /* unused parameter */
  (void)argv;  /* unused parameter */

  char pname[MAX_SIZE] = {0};
  int serverfd = 0;
  int clientfd = 0;
  int port = 21002;
  int rc = 0;
  int bytes_received = 0;
  char buf[MAX_BUF_LEN];
  struct sockaddr_in clientaddr;
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

  rc = prctl(PR_GET_NAME, (unsigned long)(pname), 0, 0, 0);
  assert(rc == 0);

  serverfd = create_udp_server(port);
  clientfd = create_client_socket("127.0.0.1", port);

  /* printf("pname = %s\n", pname); */
  /* printf("argv[0] = %s\n", argv[0]); */
  clogging_binary_init(pname, MAX_SIZE, "", 0, LOG_LEVEL_DEBUG, clogging_create_handle_from_fd(clientfd));
  assert(clogging_binary_get_loglevel() == LOG_LEVEL_DEBUG);
  LOG_DEBUG(format, argint, argchar, arguint, arglongint, arglonglongint,
                   argulonglongint, argptr, argstr);

  bytes_received =
      receive_msg_from_client(serverfd, buf, MAX_BUF_LEN, &clientaddr);
  printf("SENT: format sent size = %zd\n", strlen(format));
  printf("SENT: argptr = %p, argstr = [%s]\n", argptr, argstr);
  printf("bytes_received = %d\n", bytes_received);

  rc = analyze_received_binary_message(format, buf, bytes_received);

  clogging_binary_set_loglevel(LOG_LEVEL_INFO);
  assert(clogging_binary_get_loglevel() == LOG_LEVEL_INFO);
  assert(clogging_binary_get_num_dropped_messages() == 0);
  return 0;
}

int main(int argc, char *argv[]) {
  return test_variable_arguments(argc, argv);
  // return test_static_string(argc, argv);
}
