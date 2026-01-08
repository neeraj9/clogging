/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifndef CLOGGING_LOGGING_COMMON_H
#define CLOGGING_LOGGING_COMMON_H

#include <time.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
/* Windows doesn't have ssize_t, define it */
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif /* _WIN32 */

/* Maximum size of message in bytes which can be logged. Note that this
 * do not include the prefix size where timestamp and other details
 * might be present.
 */
#define MAX_LOG_MSG_LEN 256

/* DONT change the values because there is a lookup
 * implemented in specific logging implementation
 * based on these values. See logging_common.c for
 * a sample usage in the get_log_level_as_cstring() implementation.
 *
 * Additionally note that the sever the error the lower the value
 * should be. This is used in specific implementation of logging.
 * See basic_logging.c logmsg() as an implementation sample.
 */
enum LogLevel {
  LOG_LEVEL_ERROR = 0,
  LOG_LEVEL_WARN = 1,
  LOG_LEVEL_INFO = 2,
  LOG_LEVEL_DEBUG = 3
};

/* The default log level is INFO and must be considered so in
 * all implementations of specific logging. See basic_logging.c for
 * usage of this.
 */
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO

/* Platform-agnostic file descriptor/handle type for cross-platform I/O.
 *
 * On Unix/Linux: Simple int file descriptor
 * On Windows: Can hold CRT file descriptor, HANDLE, or SOCKET
 *
 * This abstraction allows the logging library to work with files,
 * sockets, pipes, and FIFOs across all platforms.
 */
#ifdef _WIN32

/* Windows handle types */
typedef enum {
  CLOGGING_HANDLE_TYPE_INVALID = 0,
  CLOGGING_HANDLE_TYPE_CRT,      /* CRT file descriptor (_open, _socket) */
  CLOGGING_HANDLE_TYPE_HANDLE,   /* Win32 HANDLE (CreateFile, pipes) */
  CLOGGING_HANDLE_TYPE_SOCKET    /* Winsock SOCKET */
} clogging_handle_type_t;

typedef struct {
  clogging_handle_type_t type;
  union {
    int crt_fd;
    void *handle;        /* HANDLE */
    uint64_t socket;     /* SOCKET (void* on 64-bit, UINT on 32-bit) */
  } value;
} clogging_handle_t;

/* Special constants for common file descriptors */
#define CLOGGING_INVALID_HANDLE ((clogging_handle_t){CLOGGING_HANDLE_TYPE_INVALID, {.crt_fd = -1}})
#define CLOGGING_STDOUT_HANDLE clogging_create_handle_from_fd(1)
#define CLOGGING_STDERR_HANDLE clogging_create_handle_from_fd(2)

#else /* Unix/Linux */

/* Unix/Linux: simple int file descriptor */
typedef int clogging_handle_t;

/* Special constants for common file descriptors */
#define CLOGGING_INVALID_HANDLE (-1)
#define CLOGGING_STDOUT_HANDLE (1)
#define CLOGGING_STDERR_HANDLE (2)

#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

/* Get the string representation of logging level.
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is defined then this is a
 * MT safe function in all respects.
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is NOT defined then
 * the init_logging() should call this method during initialization
 * as a workaround to make it MT safe initialization. See the implementation
 * of this function for more details.
 */
const char *get_log_level_as_cstring(enum LogLevel level);

/*
 * Get the current time in c string with '\0' termination
 * and returns the length of the string.
 * The current implementation follows the ISO 8601 date and time
 * combined format with a resolution of seconds in the UTC timezone.
 * In case of error -1 is retured.
 */
int time_to_cstr(time_t *t, char *timestr, int maxlen);

/*
  * A safer version of strncpy which guarantees null termination
  * of the destination string with truncation if required.
  *
  * The function returns the pointer to destination string.
*/
char *clogging_strtcpy(char *dest, const char *src, size_t dsize);

/* Cross-platform handle management functions.
 *
 * These functions provide a uniform interface for working with
 * file descriptors, handles, and sockets across Unix/Linux and Windows.
 */

/* Create a handle from a C runtime file descriptor (portable).
 * On Unix: returns the fd as-is
 * On Windows: wraps the fd in a clogging_handle_t structure
 */
clogging_handle_t clogging_create_handle_from_fd(int fd);

/* Create a handle from a native Windows handle (Windows only, no-op on Unix).
 * For use with CreateFile(), CreateNamedPipe(), etc.
 */
#ifdef _WIN32
clogging_handle_t clogging_create_handle_from_native(void *handle);

/* Create a handle from a Windows socket (Windows only, no-op on Unix).
 * For use with socket(), WSASocket(), etc.
 */
clogging_handle_t clogging_create_handle_from_socket(uint64_t socket);
#endif /* _WIN32 */

/* Validate if a handle is valid (not INVALID_HANDLE or -1).
 */
int clogging_handle_is_valid(clogging_handle_t handle);

/* Check if handle represents a socket.
 * Returns 1 if socket, 0 if not, -1 on error.
 */
int clogging_handle_is_socket(clogging_handle_t handle);

/* Check if handle represents a pipe or FIFO.
 * Returns 1 if pipe/FIFO, 0 if not, -1 on error.
 */
int clogging_handle_is_pipe(clogging_handle_t handle);

/* Platform-agnostic write operation.
 * Returns number of bytes written, or -1 on error.
 * Sets errno on error.
 */
ssize_t clogging_handle_write(clogging_handle_t handle, const void *buf,
                             size_t count);

#ifdef __cplusplus
}
#endif

#endif /* CLOGGING_LOGGING_COMMON_H */
