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

#include "logging_common.h"

#include <string.h>   /* strncpy(), strnlen() */
#include <stdio.h>    /* snprintf() */
#include <time.h>     /* gmtime_r() */
#include <errno.h>    /* errno */

#ifdef _WIN32
#include <io.h>       /* _write() */
#include <windows.h>  /* WriteFile() */
#else
#include <unistd.h>   /* write() */
#include <sys/stat.h> /* fstat(), S_ISSOCK, S_ISFIFO */
#include <sys/socket.h> /* for socket detection on Unix */
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define THREAD_LOCAL __declspec(thread)
/* Windows doesn't have gmtime_r, provide a wrapper */
#ifndef HAVE_GMTIME_R
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result) {
  struct tm *ret = gmtime(timep);
  if (ret && result) {
    *result = *ret;
    return result;
  }
  return NULL;
}
#endif
#else
#define THREAD_LOCAL __thread
#endif

uint8_t clogging_str_capsize_u8(const char *str) {
  return (uint8_t)strnlen(str, UINT8_MAX - 1) + 1;
}

/*
 * Get the string representation of logging level.
 */
const char *get_log_level_as_cstring(enum LogLevel level) {
  /*
   * A better alternative would be to use thread local storage,
   * via the __thread gnu directive. This will add a cost to
   * it though, which is inherently due to the way thread local
   * storage is relaized by the gnu c compiler.
   * In case this function is called infrequently, which will
   * ultimately depend on calls to logging api then use the
   * thread local approach.
   *
   */

  /* The mapping is based on the enumeration values as in
   * logging_common.h, so update this mapping table to
   * match that.
   */
  static THREAD_LOCAL const char *level_to_str[] = {
      "ERROR", /* 0 */
      "WARN",  /* 1 */
      "INFO",  /* 2 */
      "DEBUG"  /* 3 */
  };

  return level_to_str[level];
}

int time_to_cstr(time_t *t, char *timestr, int maxlen) {
  struct tm tms;

  gmtime_r(t, &tms);
  return snprintf(timestr, maxlen, "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
           tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday, tms.tm_hour,
           tms.tm_min, tms.tm_sec);
}

char *clogging_strtcpy(char *dest, const char *src, size_t dsize) {
  /* Its better to check for corner cases beforehand to avoid
   * any undefined behavior.
   * As per cppreference.com the following applies.
   * The following errors are detected at runtime and call the currently installed constraint handler function:
   * - src or dest is a null pointer
   * - destsz is zero or greater than RSIZE_MAX
   * - count is greater than RSIZE_MAX
   * - count is greater or equal destsz, but destsz is less or equal strnlen_s(src, count), in other words, truncation would occur
   * - overlap would occur between the source and the destination strings
   */
  if (dest == NULL || src == NULL) {
    return dest;
  }
  if (dsize == 0) {
    return dest;
  }
#ifdef _WIN32
  strncpy_s(dest, dsize, src, _TRUNCATE);
#else
  strncpy(dest, src, dsize - 1);
  dest[dsize - 1] = '\0';
#endif
  return dest;
}

/* Cross-platform handle creation and management functions */

#ifdef _WIN32

clogging_handle_t clogging_create_handle_from_fd(int fd) {
  clogging_handle_t handle = {CLOGGING_HANDLE_TYPE_CRT, {.crt_fd = fd}};
  return handle;
}

clogging_handle_t clogging_create_handle_from_native(void *handle) {
  clogging_handle_t h = {CLOGGING_HANDLE_TYPE_HANDLE, {.handle = handle}};
  return h;
}

clogging_handle_t clogging_create_handle_from_socket(uint64_t socket) {
  clogging_handle_t h = {CLOGGING_HANDLE_TYPE_SOCKET, {.socket = socket}};
  return h;
}

int clogging_handle_is_valid(clogging_handle_t handle) {
  switch (handle.type) {
    case CLOGGING_HANDLE_TYPE_CRT:
      return handle.value.crt_fd >= 0;
    case CLOGGING_HANDLE_TYPE_HANDLE:
      return handle.value.handle != NULL && handle.value.handle != INVALID_HANDLE_VALUE;
    case CLOGGING_HANDLE_TYPE_SOCKET:
      return handle.value.socket != (uint64_t)INVALID_SOCKET;
    default:
      return 0;
  }
}

int clogging_handle_is_socket(clogging_handle_t handle) {
  if (handle.type == CLOGGING_HANDLE_TYPE_SOCKET) {
    return 1;
  }
  
  if (handle.type == CLOGGING_HANDLE_TYPE_CRT) {
    /* Check if CRT fd is a socket using getsockname */
    struct sockaddr addr;
    int addrlen = sizeof(addr);
    int result = getsockname(handle.value.crt_fd, &addr, &addrlen);
    return (result == 0) ? 1 : 0;
  }
  
  return 0;
}

int clogging_handle_is_pipe(clogging_handle_t handle) {
  if (handle.type == CLOGGING_HANDLE_TYPE_HANDLE) {
    HANDLE win_handle = (HANDLE)handle.value.handle;
    if (win_handle == NULL || win_handle == INVALID_HANDLE_VALUE) {
      return -1;
    }
    DWORD type = GetFileType(win_handle);
    return (type == FILE_TYPE_PIPE) ? 1 : 0;
  }
  
  if (handle.type == CLOGGING_HANDLE_TYPE_CRT) {
    /* Convert CRT fd to HANDLE and check */
    HANDLE win_handle = (HANDLE)_get_osfhandle(handle.value.crt_fd);
    if (win_handle == INVALID_HANDLE_VALUE) {
      return -1;
    }
    DWORD type = GetFileType(win_handle);
    return (type == FILE_TYPE_PIPE) ? 1 : 0;
  }
  
  return 0;
}

ssize_t clogging_handle_write(clogging_handle_t handle, const void *buf,
                             size_t count) {
  DWORD written = 0;
  int result = 0;
  
  switch (handle.type) {
    case CLOGGING_HANDLE_TYPE_CRT:
      result = _write(handle.value.crt_fd, buf, (unsigned int)count);
      return (ssize_t)result;
    
    case CLOGGING_HANDLE_TYPE_HANDLE: {
      HANDLE win_handle = (HANDLE)handle.value.handle;
      if (win_handle == NULL || win_handle == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
      }
      if (WriteFile(win_handle, buf, (DWORD)count, &written, NULL)) {
        return (ssize_t)written;
      }
      /* Convert Windows error to errno */
      DWORD error = GetLastError();
      if (error == ERROR_HANDLE_EOF) {
        errno = EIO;
      } else if (error == ERROR_NOT_ENOUGH_MEMORY) {
        errno = ENOMEM;
      } else {
        errno = EIO;
      }
      return -1;
    }
    
    case CLOGGING_HANDLE_TYPE_SOCKET: {
      SOCKET sock = (SOCKET)handle.value.socket;
      if (sock == INVALID_SOCKET) {
        errno = EBADF;
        return -1;
      }
      int sent = send(sock, (const char *)buf, (int)count, 0);
      if (sent == SOCKET_ERROR) {
        int wsa_error = WSAGetLastError();
        /* Map Windows socket errors to errno */
        switch (wsa_error) {
          case WSAEBADF:
          case WSAENOTSOCK:
            errno = EBADF;
            break;
          case WSAECONNRESET:
            errno = ECONNRESET;
            break;
          case WSAECONNABORTED:
            errno = ECONNABORTED;
            break;
          case WSAEFAULT:
            errno = EFAULT;
            break;
          default:
            errno = EIO;
            break;
        }
        return -1;
      }
      return (ssize_t)sent;
    }
    
    default:
      errno = EBADF;
      return -1;
  }
}

#else /* Unix/Linux */

clogging_handle_t clogging_create_handle_from_fd(int fd) {
  return fd;
}

int clogging_handle_is_valid(clogging_handle_t handle) {
  return handle >= 0;
}

int clogging_handle_is_socket(clogging_handle_t handle) {
  struct stat statbuf;
  if (fstat(handle, &statbuf) < 0) {
    return -1;
  }
  return S_ISSOCK(statbuf.st_mode) ? 1 : 0;
}

int clogging_handle_is_pipe(clogging_handle_t handle) {
  struct stat statbuf;
  if (fstat(handle, &statbuf) < 0) {
    return -1;
  }
  return S_ISFIFO(statbuf.st_mode) ? 1 : 0;
}

ssize_t clogging_handle_write(clogging_handle_t handle, const void *buf,
                             size_t count) {
  return write(handle, buf, count);
}

#endif /* _WIN32 */


#ifdef __cplusplus
}
#endif
