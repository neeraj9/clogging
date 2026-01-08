/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#include "test_binary_logging_common.h"
#include "../src/logging_common.h" /* time_to_cstr() */
#include <assert.h>
#include <time.h>

int bigendian_to_native(const char *buf, int bytes, char *dst) {
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

int read_nbytes(const char *buf, int bytes, unsigned long long int *llval) {
  switch (bytes) {
  case 1:
    *llval = buf[0] & 0x00ff;
    break;
  case 2:
    *llval = ((buf[0] & 0x00ff) << 8) | (buf[1] & 0x00ff);
    break;
  case 4:
    *llval = ((buf[0] & 0x00ff) << 24) | ((buf[1] & 0x00ff) << 16) |
             ((buf[2] & 0x00ff) << 8) | (buf[3] & 0x00ff);
    break;
  case 8:
    *llval = ((unsigned long long int)(buf[0] & 0x00ff) << 56) |
             ((unsigned long long int)(buf[1] & 0x00ff) << 48) |
             ((unsigned long long int)(buf[2] & 0x00ff) << 40) |
             ((unsigned long long int)(buf[3] & 0x00ff) << 32) |
             ((buf[4] & 0x00ff) << 24) | ((buf[5] & 0x00ff) << 16) |
             ((buf[6] & 0x00ff) << 8) | (buf[7] & 0x00ff);
    break;
  default:
    return -1;
  }
  return 0;
}

int read_length(const char *buf, int *offset, int *bytes) {
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

int parse_format_specifiers(const char *format,
                            enum VarArgType *expected_types,
                            int max_args) {
  int arg_count = 0;
  const char *p = format;

  while (*p && arg_count < max_args) {
    if (*p == '%') {
      p++;
      if (*p == '%') {
        /* escaped % */
        p++;
        continue;
      }
      /* skip flags, width, precision */
      while (*p && (*p == '-' || *p == '+' || *p == ' ' || *p == '#' ||
                    *p == '0' || (*p >= '0' && *p <= '9') || *p == '.')) {
        p++;
      }
      /* skip length modifiers */
      if (*p == 'l') {
        p++;
        if (*p == 'l') {
          p++; /* long long */
        }
      } else if (*p == 'h') {
        p++;
        if (*p == 'h') {
          p++; /* short or char */
        }
      }
      /* get conversion specifier */
      if (*p == 'd' || *p == 'i' || *p == 'u' || *p == 'x' || *p == 'X' ||
          *p == 'o') {
        expected_types[arg_count++] = BINARY_LOG_VAR_ARG_INTEGER;
      } else if (*p == 'f' || *p == 'F' || *p == 'e' || *p == 'E' ||
                 *p == 'g' || *p == 'G') {
        expected_types[arg_count++] = BINARY_LOG_VAR_ARG_DOUBLE;
      } else if (*p == 'p') {
        expected_types[arg_count++] = BINARY_LOG_VAR_ARG_POINTER;
      } else if (*p == 's' || *p == 'c') {
        if (*p == 's') {
          expected_types[arg_count++] = BINARY_LOG_VAR_ARG_STRING;
        } else {
          /* %c is treated as integer (char) */
          expected_types[arg_count++] = BINARY_LOG_VAR_ARG_INTEGER;
        }
      }
      p++;
    } else {
      p++;
    }
  }
  return arg_count;
}

int validate_variable_arguments(const char *format,
                                struct variable_arg *args,
                                int arg_count) {
  enum VarArgType expected_types[MAX_NUM_VARIABLE_ARGS];
  int expected_count = 0;
  int i = 0;

  expected_count = parse_format_specifiers(format, expected_types, MAX_NUM_VARIABLE_ARGS);

  if (arg_count != expected_count) {
    fprintf(stderr,
            "ERROR: Argument count mismatch! Expected %d, got %d\n",
            expected_count, arg_count);
    return -1;
  }

  for (i = 0; i < arg_count; ++i) {
    if (args[i].arg_type != expected_types[i]) {
      fprintf(stderr,
              "ERROR: Argument type mismatch at index %d! "
              "Expected type %d, got type %d\n",
              i, expected_types[i], args[i].arg_type);
      return -1;
    }
  }

  printf("SUCCESS: All %d variable arguments match the format string\n",
         arg_count);
  return 0;
}

int analyze_received_binary_message(const char *format, const char *buf,
                                    int buflen) {
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
  int var_arg_count = 0;

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
    args[i].arg_type = (enum VarArgType)(buf[offset++] & 0x00ff);
    rc = read_length(buf, &offset, &(args[i].bytes));
    if (args[i].bytes <= 0) {
      break;
    }
    if (args[i].arg_type == BINARY_LOG_VAR_ARG_INTEGER) {
      rc = read_nbytes(&buf[offset], args[i].bytes, &(args[i].llval));
      printf("[%d] detected %d bytes integer arg = %llu\n", i, args[i].bytes,
             args[i].llval);
    } else if (args[i].arg_type == BINARY_LOG_VAR_ARG_DOUBLE) {
      if (args[i].bytes == sizeof(long double)) {
        bigendian_to_native(&buf[offset], args[i].bytes,
                            (char *)&(args[i].ldbl));
        printf("[%d] detected %d bytes long double arg = %Lg\n", i,
               args[i].bytes, args[i].ldbl);
      } else {
        /* cannot read directly into long double because
         * of memory architecture issues, so
         * read in the correct data type first (as raw)
         * and then type cast. Since the storage
         * type is of bigger storage, so there is
         * no issue.
         */
        double dbl_val;
        bigendian_to_native(&buf[offset], args[i].bytes, (char *)&dbl_val);
        args[i].ldbl = dbl_val;
        printf("[%d] detected %d bytes double arg = %g\n", i, args[i].bytes,
               dbl_val);
      }
    } else if (args[i].arg_type == BINARY_LOG_VAR_ARG_POINTER) {
      bigendian_to_native(&buf[offset], args[i].bytes, (char *)&(args[i].p));
      printf("[%d] detected %d bytes pointer arg = %p\n", i, args[i].bytes,
             args[i].p);
    } else if (args[i].arg_type == BINARY_LOG_VAR_ARG_STRING) {
      /* store the reference at present, so there
       * is no allocation, copy and free required.
       */
      args[i].s = &buf[offset];
      printf("[%d] detected %d bytes string arg = [%.*s]\n", i, args[i].bytes,
             args[i].bytes, args[i].s);
    } else {
      /* This is a bug! */
      assert(0);
    }
    offset += args[i].bytes;
    ++i;
  }

  var_arg_count = i;

  /* Validate that received arguments match the format string */
  rc = validate_variable_arguments(format, args, var_arg_count);
  if (rc < 0) {
    fprintf(stderr, "Variable argument validation failed\n");
    return -1;
  }

  logtime = (time_t)timeval;
  rc = time_to_cstr(&logtime, time_str, MAX_TIME_STR_LEN);
  if (rc < 0) {
    snprintf(time_str, MAX_TIME_STR_LEN, "invalid-time");
    return offset;
  }

  printf("buflen = %d, offset = %d, msglen = %d\n", buflen, offset, msglen);
  printf("timestamp = %llu, time = %s\n", timeval, time_str);
  printf("hostname=[%.*s], programname=[%.*s], threadname=[%.*s]\n",
         hostname_len, hostname, programname_len, programname, threadname_len,
         threadname);
  printf("pid = %d, loglevel = %d\n", pid, loglevel);
  printf("filename=[%.*s], funcname=[%.*s]\n", filename_len, filename,
         funcname_len, funcname);
  printf("linenum = %d\n", linenum);
  return offset;
}
