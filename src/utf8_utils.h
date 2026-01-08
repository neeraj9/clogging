/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifndef CLOGGING_UTF8_UTILS_H
#define CLOGGING_UTF8_UTILS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Validate that a string is valid UTF-8.
 * 
 * Arguments:
 *   str - pointer to string to validate
 *   len - length of string in bytes (can be 0 for auto-detect via strlen)
 * 
 * Returns:
 *   1 if valid UTF-8
 *   0 if invalid UTF-8
 *   -1 on error (NULL pointer)
 * 
 * If len is 0, the function uses strlen() to determine the length.
 */
int clogging_utf8_validate(const char *str, size_t len);

/* Get the number of UTF-8 characters (not bytes) in a string.
 * 
 * This counts the actual Unicode code points, not the byte count.
 * For example, the emoji 🚀 (U+1F680) is 4 bytes in UTF-8 but counts as 1 character.
 * 
 * Arguments:
 *   str - pointer to UTF-8 string
 * 
 * Returns:
 *   Number of UTF-8 characters (code points)
 *   -1 if string is not valid UTF-8 or is NULL
 */
int clogging_utf8_strlen(const char *str);

/* Check if a byte is a UTF-8 continuation byte.
 * 
 * UTF-8 continuation bytes have the bit pattern 10xxxxxx (0x80-0xBF).
 * 
 * Arguments:
 *   byte - byte to check
 * 
 * Returns:
 *   1 if byte is a continuation byte
 *   0 otherwise
 */
int clogging_utf8_is_continuation(unsigned char byte);

/* Get the length of the next UTF-8 character in bytes.
 * 
 * Given the first byte of a UTF-8 sequence, returns how many bytes
 * make up that character.
 * 
 * Arguments:
 *   first_byte - first byte of UTF-8 sequence
 * 
 * Returns:
 *   1, 2, 3, or 4 for valid UTF-8 start bytes
 *   -1 for invalid start byte or NULL
 */
int clogging_utf8_char_length(unsigned char first_byte);

#ifdef _WIN32
/* Convert Windows UTF-16 (wide char) to UTF-8.
 * 
 * Arguments:
 *   wide - pointer to null-terminated UTF-16 string (from Windows API)
 *   utf8 - pointer to output buffer for UTF-8 string
 *   utf8_size - size of output buffer in bytes
 * 
 * Returns:
 *   Number of bytes written to utf8 buffer (excluding null terminator)
 *   -1 on error (invalid input, buffer too small, NULL pointers)
 * 
 * Note: The output buffer will be null-terminated if conversion succeeds.
 *       A safe output size is (wide string length * 3 + 1) for worst-case UTF-16 to UTF-8.
 */
int clogging_utf8_from_wide(const wchar_t *wide, char *utf8, size_t utf8_size);

/* Convert UTF-8 to Windows UTF-16 (wide char).
 * 
 * Arguments:
 *   utf8 - pointer to null-terminated UTF-8 string
 *   wide - pointer to output buffer for UTF-16 string
 *   wide_size - size of output buffer in wide characters (not bytes)
 * 
 * Returns:
 *   Number of wide characters written (excluding null terminator)
 *   -1 on error (invalid input, buffer too small, NULL pointers)
 * 
 * Note: The output buffer will be null-terminated if conversion succeeds.
 */
int clogging_utf8_to_wide(const char *utf8, wchar_t *wide, size_t wide_size);

/* Set Windows console output code page to UTF-8.
 * 
 * This should be called once during application initialization to enable
 * proper UTF-8 output on Windows console. Without this, Unicode characters
 * may not display correctly.
 * 
 * Note: This is a no-op on non-Windows platforms.
 * 
 * Returns:
 *   0 on success
 *   -1 on error
 */
int clogging_init_utf8_console(void);

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* CLOGGING_UTF8_UTILS_H */
