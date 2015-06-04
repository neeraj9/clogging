/*
 * Copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
 *
 *  This file is part of clogging.
 *
 *  clogging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  clogging is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with clogging.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../src/binary_logging.h"
#include "../src/logging_common.h"	/* time_to_cstr() */

#ifndef __FILENAME__
#error __FILENAME__ not defined
#endif

#include <assert.h>		/* assert() */
#include <arpa/inet.h>		/* inet_aton(), htons() */
#include <fcntl.h>		/* fcntl() */
#include <sys/prctl.h>		/* prctl() */
#include <stdio.h>
#include <string.h>		/* strlen() */
#include <sys/socket.h>		/* socket(), connect() */
#include <time.h>		/* time_t */

/* as per man prctl(2) the size should be at least 16 bytes */
#define MAX_SIZE 32

#define MAX_BUF_LEN 1024

/* ISO 8601 date and time format with sec */
#define MAX_TIME_STR_LEN 26

#define MAX_NUM_VARIABLE_ARGS 20

struct variable_arg {
	enum VarArgType arg_type;
	int bytes;
	union {
		unsigned long long int llval;
		long double ldbl;
		const char *s;
		void *p;
	};
};

int
bigendian_to_native(const char *buf, int bytes, char *dst)
{
	int i = 0;
	int is_little_endian = 0;

	i = 1;
	is_little_endian = (*((char *)&i)) & 0x00ff;
	/* the input is in big-endian so convert
	 * appropriately.
	 */
	if (is_little_endian) {
		i = bytes - 1;
		while (i >= 0) {
			*dst = buf[i];
			--i;
			++dst;
		}
	} else {
		memcpy(dst, buf, bytes);
	}
	return 0;
}

int
read_nbytes(const char *buf, int bytes, unsigned long long int *llval)
{
	switch (bytes) {
	case 1:
		*llval = buf[0] & 0x00ff;
		break;
	case 2:
		*llval = ((buf[0] & 0x00ff) << 8) | (buf[1] & 0x00ff);
		break;
	case 4:
		*llval = ((buf[0] & 0x00ff) << 24) |
			((buf[1] & 0x00ff) << 16) |
			((buf[2] & 0x00ff) << 8) |
			(buf[3] & 0x00ff);
		break;
	case 8:
		*llval = ((unsigned long long int)(buf[0] & 0x00ff) << 56) |
			((unsigned long long int)(buf[1] & 0x00ff) << 48) |
			((unsigned long long int)(buf[2] & 0x00ff) << 40) |
			((unsigned long long int)(buf[3] & 0x00ff) << 32) |
			((buf[4] & 0x00ff) << 24) |
			((buf[5] & 0x00ff) << 16) |
			((buf[6] & 0x00ff) << 8) |
			(buf[7] & 0x00ff);
		break;
	default:
		return -1;
	}
	return 0;
}

inline int
read_length(const char *buf, int *offset, int *bytes)
{
	int val1 = buf[*offset];
	if (val1 & 0x80) {
		(*offset) += 1;
		*bytes = val1 & 0x7f;
		return 1;
	} /* else */
	*bytes = (val1 << 8) | (buf[(*offset) + 1] & 0x00ff);
	(*offset) += 2;
	return 2;
}

/* create udp server and return the socket fd */
int create_udp_server(int port)
{
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
			    struct sockaddr_in *clientaddr)
{
	socklen_t clientaddr_len = sizeof(struct sockaddr_in);
	int bytes_received = 0;

	bytes_received = recvfrom(fd, buf, buflen, 0,
				  (struct sockaddr *)clientaddr,
				  &clientaddr_len);
	return bytes_received;
}

int create_client_socket(const char *ip, int port)
{
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
		fprintf(stderr, "cannot connect to ip = %s, port ="
			"%d\n", ip, port);
		return -1;
	}
	return fd;
}

int analyze_received_binary_message(const char *msg, const char *buf,
				    int buflen)
{
	int msglen = 0;
	time_t logtime;
	char time_str[MAX_TIME_STR_LEN];
	unsigned long long llval = 0LLU;
	unsigned long long timeval = 0LLU;
	const char *hostname = NULL;
	int hostname_len = 0;
	const char *programname = NULL;
	int programname_len = 0;
	const char *threadname = NULL;
	int threadname_len = 0;
	int pid = 0;
	int loglevel = 0;
	const char *filename = NULL;
	int filename_len = 0;
	const char *funcname = NULL;
	int funcname_len = 0;
	int linenum = 0;
	int offset = 0;
	int bytes = 0;
	int rc = 0;
	int i = 0;
	struct variable_arg args[MAX_NUM_VARIABLE_ARGS];
	int is_little_endian = 0;

	/* determine the endianness */
	rc = 1;
	is_little_endian = (*((char *)&rc)) & 0x00ff;

	printf("received buf[%d] = [", buflen);
	for (i = 0; i < buflen; ++i) {
		printf("%02x, ", buf[i] & 0x00ff);
	}
	printf("]\n");

	/* <length> <timestamp> <hostname> <progname> <threadname> <pid> <loglevel>
	 *   <file> <func> <linenum> [<arg1>, <arg2>, ...]
	 */
	rc = read_nbytes(&buf[offset], 2, &llval);
	offset += 2;
	msglen = (int)(llval & 0xffff);

	rc = read_length(buf, &offset, &bytes);
	rc = read_nbytes(&buf[offset], bytes, &timeval);
	offset += bytes;

	rc = read_length(buf, &offset, &hostname_len);
	hostname = &buf[offset];
	offset += hostname_len;

	rc = read_length(buf, &offset, &programname_len);
	programname = &buf[offset];
	offset += programname_len;

	rc = read_length(buf, &offset, &threadname_len);
	threadname = &buf[offset];
	offset += threadname_len;

	rc = read_length(buf, &offset, &bytes);
	rc = read_nbytes(&buf[offset], bytes, &llval);
	pid = (int)llval;
	offset += bytes;

	rc = read_length(buf, &offset, &bytes);
	rc = read_nbytes(&buf[offset], bytes, &llval);
	loglevel = (int)llval;
	offset += bytes;

	rc = read_length(buf, &offset, &filename_len);
	filename = &buf[offset];
	offset += filename_len;

	rc = read_length(buf, &offset, &funcname_len);
	funcname = &buf[offset];
	offset += funcname_len;

	rc = read_length(buf, &offset, &bytes);
	rc = read_nbytes(&buf[offset], bytes, &llval);
	linenum = (int)llval;
	offset += bytes;

	i = 0;
	/* read variable arguments */
	while (offset < buflen) {
		/* variable type */
		args[i].arg_type = (enum VarArgType) (buf[offset++] & 0x00ff);
		rc = read_length(buf, &offset, &(args[i].bytes));
		if (args[i].bytes <= 0) {
			break;
		}
		if (args[i].arg_type == BINARY_LOG_VAR_ARG_INTEGER) {
			rc = read_nbytes(&buf[offset], args[i].bytes,
					 &(args[i].llval));
			printf("[%d] detected %d bytes integer arg = %llu\n",
			       i, args[i].bytes, args[i].llval);
		} else if (args[i].arg_type == BINARY_LOG_VAR_ARG_DOUBLE) {
			if (args[i].bytes == sizeof(long double)) {
				bigendian_to_native(&buf[offset], args[i].bytes,
						    (char *)&(args[i].ldbl));
				printf("[%d] detected %d bytes long double arg = %Lg\n",
				       i, args[i].bytes, args[i].ldbl);
			} else {
				/* cannot read directly into long double because
				 * of memory architecture issues, so
				 * read in the correct data type first (as raw)
				 * and then type cast. Since the storage
				 * type is of bigger storage, so there is
				 * no issue.
				 */
				double dbl_val;
				bigendian_to_native(&buf[offset], args[i].bytes,
						    (char *)&dbl_val);
				args[i].ldbl = dbl_val;
				printf("[%d] detected %d bytes double arg = %g\n",
				       i, args[i].bytes, dbl_val);
			}
		} else if (args[i].arg_type == BINARY_LOG_VAR_ARG_POINTER) {
			bigendian_to_native(&buf[offset], args[i].bytes,
					    (char *)&(args[i].p));
			printf("[%d] detected %d bytes pointer arg = %p\n",
			       i, args[i].bytes, args[i].p);
		} else if (args[i].arg_type == BINARY_LOG_VAR_ARG_STRING) {
			/* store the reference at present, so there
			 * is no allocation, copy and free required.
			 */
			args[i].s = &buf[offset];
			printf("[%d] detected %d bytes string arg = [%.*s]\n",
			       i, args[i].bytes, args[i].bytes, args[i].s);
		} else {
			/* This is a bug! */
			assert(0);
		}
		offset += args[i].bytes;
		++i;
	}

	logtime = (time_t) timeval;
	rc = time_to_cstr(&logtime, time_str, MAX_TIME_STR_LEN);

	printf("buflen = %d, offset = %d, msglen = %d\n", buflen, offset,
	       msglen);
	printf("timestamp = %llu, time = %s\n", timeval, time_str);
	printf("hostname=[%.*s], programname=[%.*s], threadname=[%.*s]\n",
	       hostname_len, hostname,
	       programname_len, programname,
	       threadname_len, threadname);
	printf("pid = %d, loglevel = %d\n", pid, loglevel);
	printf("filename=[%.*s], funcname=[%.*s]\n",
	       filename_len, filename,
	       funcname_len, funcname);
	printf("linenum = %d\n", linenum);
	return offset;
}

int test_static_string(int argc, char *argv[])
{
	char pname[MAX_SIZE] = {0};
	int serverfd = 0;
	int clientfd = 0;
	int port = 21002;
	int rc = 0;
	int bytes_received = 0;
	char buf[MAX_BUF_LEN];
	char msg[] = "A fd debug log looks like this";
        struct sockaddr_in clientaddr;

	rc = prctl(PR_GET_NAME, (unsigned long)(pname), 0, 0, 0);
	assert(rc == 0);

	serverfd = create_udp_server(port);
	clientfd = create_client_socket("127.0.0.1", port);

	/* printf("pname = %s\n", pname); */
	/* printf("argv[0] = %s\n", argv[0]); */
	BINARY_INIT_LOGGING(pname, "", LOG_LEVEL_DEBUG, clientfd);
	assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_DEBUG);
	BINARY_LOG_DEBUG(msg);

	bytes_received = receive_msg_from_client(serverfd, buf, MAX_BUF_LEN,
						 &clientaddr);
	printf("msg sent size = %d\n", strlen(msg));
	printf("bytes_received = %d\n", bytes_received);

	rc = analyze_received_binary_message(msg, buf, bytes_received);

	BINARY_SET_LOG_LEVEL(LOG_LEVEL_INFO);
	assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_INFO);
	assert(BINARY_GET_NUM_DROPPED_MESSAGES() == 0);
	return 0;
}

int test_variable_arguments(int argc, char *argv[])
{
	char pname[MAX_SIZE] = {0};
	int serverfd = 0;
	int clientfd = 0;
	int port = 21002;
	int rc = 0;
	int bytes_received = 0;
	char buf[MAX_BUF_LEN];
        struct sockaddr_in clientaddr;
	/* variable arguments with format */
	char format[] = "A fd debug log looks like this, int=%d, char=%c,"
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
	BINARY_INIT_LOGGING(pname, "", LOG_LEVEL_DEBUG, clientfd);
	assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_DEBUG);
	BINARY_LOG_DEBUG(format, argint, argchar, arguint, arglongint,
			 arglonglongint, argulonglongint, argptr, argstr);

	bytes_received = receive_msg_from_client(serverfd, buf, MAX_BUF_LEN,
						 &clientaddr);
	printf("SENT: format sent size = %d\n", strlen(format));
	printf("SENT: argptr = %p, argstr = [%s]\n", argptr, argstr);
	printf("bytes_received = %d\n", bytes_received);

	rc = analyze_received_binary_message(format, buf, bytes_received);

	BINARY_SET_LOG_LEVEL(LOG_LEVEL_INFO);
	assert(BINARY_GET_LOG_LEVEL() == LOG_LEVEL_INFO);
	assert(BINARY_GET_NUM_DROPPED_MESSAGES() == 0);
	return 0;
}

int main(int argc, char *argv[])
{
	return test_variable_arguments(argc, argv);
	//return test_static_string(argc, argv);
}
