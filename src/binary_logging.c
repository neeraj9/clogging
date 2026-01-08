/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "binary_logging.h"

/* Cross-platform endianness detection */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
  /* GCC/Clang style - most reliable */
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define IS_LITTLE_ENDIAN 1
    #define IS_BIG_ENDIAN 0
  #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define IS_LITTLE_ENDIAN 0
    #define IS_BIG_ENDIAN 1
  #else
    #error "Unknown byte order"
  #endif
#elif defined(_WIN32) || defined(_WIN64)
  /* Windows x86/x64 is always little-endian */
  #define IS_LITTLE_ENDIAN 1
  #define IS_BIG_ENDIAN 0
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) || \
      defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL)
  #define IS_LITTLE_ENDIAN 1
  #define IS_BIG_ENDIAN 0
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
      defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB)
  #define IS_LITTLE_ENDIAN 0
  #define IS_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && defined(__BIG_ENDIAN)
  /* Fallback to POSIX endian.h macros */
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    #define IS_LITTLE_ENDIAN 1
    #define IS_BIG_ENDIAN 0
  #elif __BYTE_ORDER == __BIG_ENDIAN
    #define IS_LITTLE_ENDIAN 0
    #define IS_BIG_ENDIAN 1
  #else
    #error "Unknown byte order"
  #endif
#else
  #error "Cannot detect byte order on this platform"
#endif

/* Handle platform-specific headers and definitions */
#ifdef _WIN32
/* Windows doesn't have __thread, use thread_local from C11 */
#define THREAD_LOCAL __declspec(thread)
/* Windows doesn't have ssize_t, define it */
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
/* Use the WIN32_LEAN_AND_MEAN macro before including windows.h. This tells the
Windows header to exclude less commonly used APIs, including the old winsock.h:
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  /* GetCurrentProcessId(), GetComputerNameExA() */
#else
#include <endian.h>   /* __LITTLE_ENDIAN and friends */
#include <unistd.h> /* getpid(), gethostname(), write() */
#define THREAD_LOCAL __thread
#endif

#include <stdarg.h>   /* va_start() and friends */
#include <stddef.h>   /* ptrdiff_t */
#include <stdio.h>    /* dprintf() and friends */
#include <string.h>   /* strlen() */
#include <sys/types.h>
#include <time.h>   /* time() */
#include <sys/types.h>

#define MAX_PROG_NAME_LEN 40
#define MAX_HOSTNAME_LEN 20

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The use of global static is tricky and prone to issues in MT (multi-threaded)
 * environments, but then the clogging_binary_init should be called ONLY once.
 * In this case there is no MT issue since
 * each of the thread will have its own copy of the file static variable.
 * The side effect is to call init_logging() separately for each of the
 * threads. This differs from the fact when thread local storage is not
 * used, when only the main thread should call init_logging() and only
 * once.
 *
 */
static THREAD_LOCAL char g_binary_progname[MAX_PROG_NAME_LEN] = {0};
static THREAD_LOCAL int g_binary_progname_length = 0;
static THREAD_LOCAL char g_binary_threadname[MAX_PROG_NAME_LEN] = {0};
static THREAD_LOCAL int g_binary_threadname_length = 0;
static THREAD_LOCAL char g_binary_hostname[MAX_HOSTNAME_LEN] = {0};
static THREAD_LOCAL int g_binary_hostname_length = 0;
static THREAD_LOCAL int g_binary_pid = 0;
static THREAD_LOCAL enum LogLevel g_binary_level = DEFAULT_LOG_LEVEL;
#ifdef _WIN32
static THREAD_LOCAL clogging_handle_t g_binary_handle = {CLOGGING_HANDLE_TYPE_CRT, {.crt_fd = 2}};
#else
static THREAD_LOCAL clogging_handle_t g_binary_handle = 2; /* stderr fd is default as 2 */
#endif
/* safeguard calling init_logging multiple times */
static THREAD_LOCAL int g_binary_is_logging_initialized = 0;

#define TOTAL_MSG_BYTES 1024
/* optimization by having only one instance per thread instead of
 * stack allocation all the time.
 */
/* account for partial write */
static THREAD_LOCAL ssize_t g_binary_previous_message_offset = 0;
static THREAD_LOCAL ssize_t g_binary_previous_message_bytes = 0;
static THREAD_LOCAL char g_binary_previous_message[TOTAL_MSG_BYTES];

/* store the number of message dropped as a counter for
 * later statistics collection.
 */
static THREAD_LOCAL uint64_t g_binary_num_msg_drops = 0;

enum length_specifier {
  LS_NONE = 0,
  LS_H,
  LS_HH,
  LS_L,
  LS_LL,
  LS_J,
  LS_Z,
  LS_T,
  LS_CAP_L
};

#define PCOPY_DATA_TYPE(store, offsetptr, llval, bytes)                        \
  PCOPY_DATA_TYPE_ #bytes(store, offsetptr, llval)

/* function prototypes */
static ssize_t fill_variable_arguments(char *store, ssize_t offset, const char *format,
                                       va_list ap);

/* Think about an optimized approach instead of using this generic
 * implementation in the future.
 *
 * This implementation stores multi-byte value in big-endian
 * format for portability.
 *
 * TODO FIXME: should we use htobe16(), htobe32() or htobe64() instead?
 */
int portable_copy(char *store, ssize_t *offset, unsigned long long val,
                         ssize_t bytes) {
  ssize_t newoffset = *offset;

  switch (bytes) {
  case 1: /* 8 bits */
    store[newoffset] = val & 0x00ff;
    break;
  case 2: /* 16 bits */
    store[newoffset] = (val >> 8) & 0x00ff;
    store[newoffset + 1] = val & 0x00ff;
    break;
  case 4: /* 32 bits */
    store[newoffset] = (val >> 24) & 0x00ff;
    store[newoffset + 1] = (val >> 16) & 0x00ff;
    store[newoffset + 2] = (val >> 8) & 0x00ff;
    store[newoffset + 3] = val & 0x00ff;
    break;
  case 8: /* 64 bits */
    store[newoffset] = (val >> 56) & 0x00ff;
    store[newoffset + 1] = (val >> 48) & 0x00ff;
    store[newoffset + 2] = (val >> 40) & 0x00ff;
    store[newoffset + 3] = (val >> 32) & 0x00ff;
    store[newoffset + 4] = (val >> 24) & 0x00ff;
    store[newoffset + 5] = (val >> 16) & 0x00ff;
    store[newoffset + 6] = (val >> 8) & 0x00ff;
    store[newoffset + 7] = val & 0x00ff;
    break;
  default:
    return -1;
  }
  *offset = newoffset + bytes;
  return 0;
}

int clogging_binary_init(const char *progname, uint8_t progname_len,
                         const char *threadname, uint8_t threadname_len,
                         enum LogLevel level, clogging_handle_t handle) {
  if (g_binary_is_logging_initialized > 0) {
    fprintf(stderr, "logging is already initialized or in the"
                    " process of initialization.\n");
    return -1;
  }

  /* first thing to do is block any other thread in running logging
   * initialization (if that is done, though its bad and should be
   * done in the main thread ONLY.
   */
  g_binary_is_logging_initialized = 1;

  /* Intentionally call the method so that any function static variables
   * in get_log_level_as_cstring() are correctly initialized.
   * Note that this should be done in any logging implementation (like
   * many other things), although the argument and the return values
   * are unimportant.
   */
  (void)get_log_level_as_cstring(LOG_LEVEL_ERROR);

  clogging_strtcpy(g_binary_progname, progname, progname_len);
  g_binary_progname_length = strlen(g_binary_progname) & 0x7f;
  clogging_strtcpy(g_binary_threadname, threadname, threadname_len);
  g_binary_threadname_length = strlen(g_binary_threadname) & 0x7f;
#ifdef _WIN32
  {
    DWORD size = MAX_HOSTNAME_LEN;
    if (!GetComputerNameExA(ComputerNameDnsHostname, g_binary_hostname, &size)) {
      clogging_strtcpy(g_binary_hostname, "unknown", MAX_HOSTNAME_LEN);
    }
  }
#else
  int rc = gethostname(g_binary_hostname, MAX_HOSTNAME_LEN);
  if (rc < 0) {
    clogging_strtcpy(g_binary_hostname, "unknown", MAX_HOSTNAME_LEN);
  }
#endif
  g_binary_hostname_length = strlen(g_binary_hostname) & 0x7f;
  #ifdef _WIN32
    g_binary_pid = (int)GetCurrentProcessId();
  #else
    g_binary_pid = (int)getpid();
  #endif
  g_binary_level = level;
  g_binary_handle = handle;

  return 0;
}

void clogging_binary_set_loglevel(enum LogLevel level) {
  g_binary_level = level;
}

enum LogLevel clogging_binary_get_loglevel(void) { return g_binary_level; }

void clogging_binary_logmsg(const char *filename, const char *funcname,
                            int linenum, enum LogLevel level,
                            const char *format, ...) {
  time_t now;
  ssize_t remaining_bytes = 0;
  ssize_t len = 0;
  int rc = 0;
  va_list ap;
  char *store = g_binary_previous_message;
  ssize_t offset = 0;
  ssize_t bytes_written = 0;
  int filenamelen = strlen(filename) & 0x7f;
  int funcnamelen = strlen(funcname) & 0x7f;

  /* ignore logs which are filtered out */
  if (level > g_binary_level) {
    return;
  }

  if (g_binary_is_logging_initialized <= 0) {
    fprintf(stderr, "logging is not initialized yet\n");
    ++g_binary_num_msg_drops;
    return;
  }

  remaining_bytes =
      (g_binary_previous_message_bytes - g_binary_previous_message_offset);
  if (remaining_bytes > 0) {
    ssize_t len = (ssize_t)clogging_handle_write(g_binary_handle,
                &g_binary_previous_message[g_binary_previous_message_offset],
                remaining_bytes);
    if (len <= 0) {
      /* cannot write a thing, so drop the current message,
       * but still keep the previous message since
       * the size is already written (and keep it for
       * another try later).
       */
      ++g_binary_num_msg_drops;
      return;
    }
    if (len < remaining_bytes) {
      g_binary_previous_message_offset += len;
      /* since this time as well it was partial write
       * so new message can anyway not be written.
       * There is no other option other than dropping the
       * current message.
       */
      ++g_binary_num_msg_drops;
      return;
    } else {
      /* previous message is written completely */
      g_binary_previous_message_bytes = 0;
      g_binary_previous_message_offset = 0;
    }
  }

  /* <length> <timestamp> <hostname> <progname>
   * <threadname> <pid> <loglevel> <file> <func> <linenum>
   * [<arg1>, <arg2>, ...] */

  /* first two bytes are used to store the overall length */
  offset = 2;
  /* number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
   */
  now = time(NULL);
  if (now == ((time_t)-1)) {
    /* cannot write a thing, so drop the current message
     */
    ++g_binary_num_msg_drops;
    return;
  }
  store[offset++] = 0x80 | sizeof(now);
  rc = portable_copy(store, &offset, now, sizeof(now));
  if (rc < 0) {
    /* cannot write a thing, so drop the current message
     */
    ++g_binary_num_msg_drops;
    return;
  }
  store[offset++] = 0x00 | ((g_binary_hostname_length >> 8) & 0x007f);
  store[offset++] = g_binary_hostname_length & 0x00ff;
  memcpy(&store[offset], g_binary_hostname, g_binary_hostname_length);
  offset += g_binary_hostname_length;
  store[offset++] = 0x00 | ((g_binary_progname_length >> 8) & 0x007f);
  store[offset++] = g_binary_progname_length & 0x00ff;
  memcpy(&store[offset], g_binary_progname, g_binary_progname_length);
  offset += g_binary_progname_length;
  store[offset++] = 0x00 | ((g_binary_threadname_length >> 8) & 0x007f);
  store[offset++] = g_binary_threadname_length & 0x00ff;
  memcpy(&store[offset], g_binary_threadname, g_binary_threadname_length);
  offset += g_binary_threadname_length;
  store[offset++] = 0x80 | sizeof(g_binary_pid);
  rc = portable_copy(store, &offset, g_binary_pid, sizeof(g_binary_pid));
  store[offset++] = 0x80 | sizeof(g_binary_level);
  rc = portable_copy(store, &offset, g_binary_level, sizeof(g_binary_level));
  store[offset++] = 0x00 | ((filenamelen >> 8) & 0x007f);
  store[offset++] = filenamelen & 0x00ff;
  memcpy(&store[offset], filename, filenamelen);
  offset += filenamelen;
  store[offset++] = 0x00 | ((funcnamelen >> 8) & 0x007f);
  store[offset++] = funcnamelen & 0x00ff;
  memcpy(&store[offset], funcname, funcnamelen);
  offset += funcnamelen;
  store[offset++] = 0x80 | sizeof(linenum);
  rc = portable_copy(store, &offset, linenum, sizeof(linenum));

  /* process format and store msg accordingly */
  va_start(ap, format);
  offset = fill_variable_arguments(store, offset, format, ap);
  va_end(ap);

  /* now that the total length is known so lets fill the
   * size of the payload (without the bytes occupied
   * by the length itself).
   */
  /* encode the length in big-endian format */
  /* offset includes the size of length itself so subtract that. */
  len = offset - 2;
  store[0] = (len >> 8) & 0x00ff;
  store[1] = (len & 0x00ff);

  bytes_written = clogging_handle_write(g_binary_handle, store, offset);
  if (bytes_written < 0) {
    /* just ignore stuff because we cannot write a
     * single bit.
     */
    ++g_binary_num_msg_drops;
    return;
  } else if (bytes_written < offset) {
    g_binary_previous_message_offset = offset - bytes_written;
    g_binary_previous_message_bytes = offset;
  } else {
    g_binary_previous_message_offset = 0;
    g_binary_previous_message_bytes = 0;
  }
}

uint64_t clogging_binary_get_num_dropped_messages(void) {
  return g_binary_num_msg_drops;
}

/* return the modified offset back to the caller indicating
 * the number of bytes written in the store
 */
static ssize_t fill_variable_arguments(char *store, ssize_t offset, const char *format,
                                       va_list ap) {
  long double ldbl = (long double)0.0;
  double dbl = 0.0;
  unsigned long long llval = 0LLU;
  const char *tmp = format;
  int is_type_specifier = 0;
  enum length_specifier lspecifier = LS_NONE;
  char *tmp_s = NULL;
  void *tmp_p = NULL;
  int *tmp_n = NULL;
  int rc = 0;
  int i = 0;
  char *tmp_d = NULL;

  /* TODO FIXME cross validate that the format specifier
   * contains the same number of specifiers as the variable arguments.
   * This should be done for additional protection.
   */
  while (*tmp != '\0') {
    if (!is_type_specifier) {
      if (*tmp != '%') {
        ++tmp;
        continue;
      }
      /* else */
      is_type_specifier = 1;
      ++tmp;
      continue;
    }
    /* designed while keeping the following url info handy
     * http://www.cplusplus.com/reference/cstdio/printf/
     */
    switch (*tmp) {
    case 'c': /* char */
      /* need a cast here since va_arg only
      takes fully promoted types */
      llval = va_arg(ap, unsigned long long);
      store[offset++] = BINARY_LOG_VAR_ARG_INTEGER & 0x00ff;
      if (lspecifier == LS_L) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(int);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(int));
        /* offset is automatically updated */
      } else {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | 1;
        ++offset;
        store[offset] = llval & 0x00ff;
        ++offset;
      }
      is_type_specifier = 0;
      lspecifier = LS_NONE;
      break;
    case 'u':
    case 'o':
    case 'x':
    case 'X': /* unsigned */
    case 'd':
    case 'i': /* int */
      llval = va_arg(ap, unsigned long long);
      store[offset++] = BINARY_LOG_VAR_ARG_INTEGER & 0x00ff;
      if (lspecifier == LS_HH) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | 1;
        ++offset;
        store[offset] = llval & 0x00ff;
        ++offset;
      } else if (lspecifier == LS_H) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(short int);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(short int));
      } else if (lspecifier == LS_L) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(long int);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(long int));
      } else if (lspecifier == LS_LL) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(long long int);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(long long int));
      } else if (lspecifier == LS_J) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(intmax_t);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(intmax_t));
      } else if (lspecifier == LS_Z) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(size_t);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(size_t));
      } else if (lspecifier == LS_T) {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(ptrdiff_t);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(ptrdiff_t));
      } else {
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(int);
        ++offset;
        rc = portable_copy(store, &offset, llval, sizeof(int));
      }
      is_type_specifier = 0;
      lspecifier = LS_NONE;
      break;
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
    case 'a':
    case 'A': /* double */
/*
  * The IEEE 754 specifies the following encoding.
  * This is just for reference rather than anything else.
  *
  * +==========+===========+======+========+=======+=======+==========+===========+
  * | Name     | Precision | Base | Digits | E min | E max | Decimal  | Decimal   |
  * |          |           |      |        |       |       | digits   | E max     |
  * +==========+===========+======+========+=======+=======+==========+===========+
  * | binary64 | Double    | 2    | 53     | −1022 | +1023 | 15.95    | 307.95    |
  * +----------+-----------+------+--------+-------+-------+----------+-----------+
  * | binary128|Quadruple  | 2    | 113    | −16382| +16383| 34.02    | 4931.77   |
  * +----------+-----------+------+--------+-------+-------+----------+-----------+
  * */
      store[offset++] = BINARY_LOG_VAR_ARG_DOUBLE & 0x00ff;
      if (lspecifier == LS_CAP_L) {
        ldbl = va_arg(ap, long double);
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(long double);
        ++offset;
        /* always store in big-endian format */
#if IS_LITTLE_ENDIAN
        tmp_d = (char *)&ldbl;
        i = sizeof(long double) - 1;
        while (i >= 0) {
          store[offset++] = tmp_d[i] & 0x00ff;
          --i;
        }
#else /* ?IS_LITTLE_ENDIAN */
        memcpy(&store[offset], &ldbl, sizeof(long double));
        offset += sizeof(long double);
#endif /* IS_LITTLE_ENDIAN */
      } else {
        dbl = va_arg(ap, double);
        /* store the size in bytes before the value */
        store[offset] = 0x80 | sizeof(double);
        ++offset;
        /* always store in big-endian format */
#if IS_LITTLE_ENDIAN
        tmp_d = (char *)&dbl;
        i = sizeof(double) - 1;
        while (i >= 0) {
          store[offset++] = tmp_d[i] & 0x00ff;
          --i;
        }
#else /* ?IS_LITTLE_ENDIAN */
        memcpy(&store[offset], &dbl, sizeof(double));
        offset += sizeof(double);
#endif /* IS_LITTLE_ENDIAN */
      }
      is_type_specifier = 0;
      lspecifier = LS_NONE;
      break;
    case 's': /* char* */
      /* TODO FIXME, alternatively why not use
       * writev() instead of copying the contents of the
       * string into private buffer?
       * This approach is optimal though results into
       * an issue with partial write, which can be
       * complex to fix.
       */
      tmp_s = va_arg(ap, char *);

      store[offset++] = BINARY_LOG_VAR_ARG_STRING & 0x00ff;

      /* There is a chance that things can crash if the
       * string is not null terminated.
       * TODO FIXME add a mechanism to restict
       * to a maximum size if going out of bounds.
       */
      int s_len = strlen(tmp_s);
      /* store the size in bytes before the value */
      /* TODO FIXME the size of the string cannot be
       * greater than 2^15-1, which is a large
       * value so should not matter but something
       * which should be documented.
       */
      s_len = s_len & 0x7fff; /* max len of 15 bits */
      store[offset++] = 0x00 | ((s_len >> 8) & 0x7f);
      store[offset++] = s_len & 0x00ff;
      /* TODO FIXME validate whether the contents will
       * fit in the store. At present lets assume that
       * there is no issue with size and continue
       * at all places.
       */
      /* copy but dont include '\0' character at the end */
      memcpy(&store[offset], tmp_s, s_len);
      offset += s_len;
      is_type_specifier = 0;
      lspecifier = LS_NONE;
      break;
    case 'p': /* void* */
      tmp_p = va_arg(ap, void *);
      store[offset++] = BINARY_LOG_VAR_ARG_POINTER & 0x00ff;
      /* The idea is to print the address stored within
       * tmp_p for logging rather than access the
       * contents there.
       */
      /* store the size in bytes before the value */
      store[offset] = 0x80 | sizeof(tmp_p);
      ++offset;
      /* always store in big-endian */
#if IS_LITTLE_ENDIAN
      tmp_d = (char *)&tmp_p;
      i = sizeof(tmp_p) - 1;
      while (i >= 0) {
        store[offset++] = tmp_d[i] & 0x00ff;
        --i;
      }
#else /* ?IS_LITTLE_ENDIAN */
      memcpy(&store[offset], &tmp_p, sizeof(tmp_p));
      offset += sizeof(tmp_p);
#endif /* IS_LITTLE_ENDIAN */
      is_type_specifier = 0;
      lspecifier = LS_NONE;
      break;
    case 'n': /* int* */
      tmp_n = va_arg(ap, int *);
      /* actually we are supposed to store
       * the bytes written so far in this
       * location.
       */
      *tmp_n = offset;
      is_type_specifier = 0;
      lspecifier = LS_NONE;
      break;
    case '.': /* special case when precision is specified */
      if (*(tmp + 1) == '*') {
        /* user supplied %.* which specifies
         * that width is specified as variable
         * argument.
         */
        (void)va_arg(ap, int);  /* width */
        /* TODO FIXME, shouldn't we care. Maybe
         * later. Ideally we should be passing
         * that as with other variable arguments,
         * but lets not do it now.
         */
        ++tmp;
        /* already preocessed within this
         * block
         */
      }
      break;
    case '%':
      /* no type specifier because user said %% */
      is_type_specifier = 0;
      lspecifier = LS_NONE;
      break;

      /* length specifiers starts from here */

    case 'h':
      if (lspecifier == LS_NONE) {
        lspecifier = LS_H;
      } else if (lspecifier == LS_H) {
        lspecifier = LS_HH;
      } /* else its not a valid thing so ignore */
      break;
    case 'l':
      if (lspecifier == LS_NONE) {
        lspecifier = LS_L;
      } else if (lspecifier == LS_L) {
        lspecifier = LS_LL;
      } /* else its not a valid thing so ignore */
      break;
    case 'j':
      if (lspecifier == LS_NONE) {
        lspecifier = LS_J;
      } /* else its not a valid thing so ignore */
      break;
    case 'z':
      if (lspecifier == LS_NONE) {
        lspecifier = LS_Z;
      } /* else its not a valid thing so ignore */
      break;
    case 't':
      if (lspecifier == LS_NONE) {
        lspecifier = LS_T;
      } /* else its not a valid thing so ignore */
      break;
    case 'L':
      if (lspecifier == LS_NONE) {
        lspecifier = LS_CAP_L;
      } /* else its not a valid thing so ignore */
      break;
    default:
      /* TODO FIXME how about length indicators like 'h' or
       * 'l' or 'll' or 'L' or 'z' or 't' or 'hh' or 'j'
       */
      break;
    }
    ++tmp;
  }
  return offset;
}

#ifdef __cplusplus
}
#endif
