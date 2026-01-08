/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifndef CLOGGING_TEST_BINARY_LOGGING_COMMON_H
#define CLOGGING_TEST_BINARY_LOGGING_COMMON_H

#include "../src/binary_logging.h"
#include <stdio.h>
#include <string.h>

/* ISO 8601 date and time format with sec */
#define MAX_TIME_STR_LEN 26

#define MAX_NUM_VARIABLE_ARGS 20

/* Lets follow the ISO C standard of 1999 and use ## __VA_ARGS__ so as
 * to avoid the neccessity of providing even a single argument after
 * format. That is its possible that the user did not provide any
 * variable arguments and the format is the entier message.
 */
#define LOG_ERROR(format, ...)                                           \
  clogging_binary_logmsg(__FILE__, __func__, __LINE__, LOG_LEVEL_ERROR, format,    \
                        ##__VA_ARGS__)
#define LOG_WARN(format, ...)                                            \
  clogging_binary_logmsg(__FILE__, __func__, __LINE__, LOG_LEVEL_WARN, format,     \
                        ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                            \
  clogging_binary_logmsg(__FILE__, __func__, __LINE__, LOG_LEVEL_INFO, format,     \
                        ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)                                           \
  clogging_binary_logmsg(__FILE__, __func__, __LINE__, LOG_LEVEL_DEBUG, format,    \
                        ##__VA_ARGS__)

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

/* Convert big-endian bytes to native endianness */
int bigendian_to_native(const char *buf, int bytes, char *dst);

/* Read N bytes from buffer and convert to unsigned long long */
int read_nbytes(const char *buf, int bytes, unsigned long long int *llval);

/* Read variable-length encoded size from buffer */
int read_length(const char *buf, int *offset, int *bytes);

/* Parse format string and extract expected argument types */
int parse_format_specifiers(const char *format,
                            enum VarArgType *expected_types,
                            int max_args);

/* Validate that received variable arguments match the format string */
int validate_variable_arguments(const char *format,
                                struct variable_arg *args,
                                int arg_count);

/* Analyze received binary log message and extract all fields */
int analyze_received_binary_message(const char *format, const char *buf,
                                    int buflen);

#endif /* CLOGGING_TEST_BINARY_LOGGING_COMMON_H */
