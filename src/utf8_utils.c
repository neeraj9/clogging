/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#include "utf8_utils.h"

#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Check if a byte is a valid UTF-8 start byte and return expected sequence length.
 * UTF-8 sequence lengths:
 * 0xxxxxxx = 1 byte (ASCII)
 * 110xxxxx = 2 bytes
 * 1110xxxx = 3 bytes
 * 11110xxx = 4 bytes
 * 10xxxxxx = continuation byte (invalid as start byte)
 */
int clogging_utf8_char_length(unsigned char first_byte) {
  if ((first_byte & 0x80) == 0) {
    /* 0xxxxxxx - single byte (ASCII) */
    return 1;
  } else if ((first_byte & 0xE0) == 0xC0) {
    /* 110xxxxx - 2 bytes */
    return 2;
  } else if ((first_byte & 0xF0) == 0xE0) {
    /* 1110xxxx - 3 bytes */
    return 3;
  } else if ((first_byte & 0xF8) == 0xF0) {
    /* 11110xxx - 4 bytes */
    return 4;
  } else {
    /* Invalid start byte (including continuation bytes 10xxxxxx) */
    return -1;
  }
}

int clogging_utf8_is_continuation(unsigned char byte) {
  /* Continuation bytes have the pattern 10xxxxxx (0x80-0xBF) */
  return (byte & 0xC0) == 0x80 ? 1 : 0;
}

int clogging_utf8_validate(const char *str, size_t len) {
  if (str == NULL) {
    return -1;
  }

  /* If len is 0, determine length automatically */
  if (len == 0) {
    len = strlen(str);
  }

  size_t i = 0;
  while (i < len) {
    unsigned char byte = (unsigned char)str[i];
    
    /* Check for embedded null byte (invalid in UTF-8 when length is explicit) */
    if (byte == 0x00) {
      /* If we have an explicit length and haven't reached it, null byte in middle is invalid */
      return 0;
    }
    
    int char_len = clogging_utf8_char_length(byte);

    if (char_len < 0) {
      /* Invalid start byte */
      return 0;
    }

    if (char_len == 1) {
      /* Single byte (ASCII), just advance */
      i += 1;
      continue;
    }

    /* Multi-byte sequence - check continuation bytes */
    if (i + char_len > len) {
      /* Sequence extends past end of string */
      return 0;
    }

    for (int j = 1; j < char_len; j++) {
      unsigned char cont_byte = (unsigned char)str[i + j];
      if (!clogging_utf8_is_continuation(cont_byte)) {
        /* Not a valid continuation byte */
        return 0;
      }
    }

    /* Check for overlong encodings.
     * These are multi-byte sequences that could have been encoded with fewer bytes.
     * Overlong encodings are a security issue and must be rejected.
     */
    if (char_len == 2) {
      /* 2-byte sequence: first byte should be in range 0xC2-0xDF */
      if (byte < 0xC2) {
        return 0; /* Overlong encoding */
      }
    } else if (char_len == 3) {
      /* 3-byte sequence: 
       * If first byte is 0xE0, second byte must be >= 0xA0 (to avoid overlong)
       */
      if (byte == 0xE0) {
        unsigned char second = (unsigned char)str[i + 1];
        if (second < 0xA0) {
          return 0; /* Overlong encoding */
        }
      }
      /* Also reject UTF-16 surrogates (0xED 0xA0-0xBF 0x80-0xBF) */
      if (byte == 0xED) {
        unsigned char second = (unsigned char)str[i + 1];
        if ((second & 0xF0) >= 0xA0) {
          return 0; /* UTF-16 surrogate - invalid in UTF-8 */
        }
      }
    } else if (char_len == 4) {
      /* 4-byte sequence:
       * If first byte is 0xF0, second byte must be >= 0x90 (to avoid overlong)
       */
      if (byte == 0xF0) {
        unsigned char second = (unsigned char)str[i + 1];
        if (second < 0x90) {
          return 0; /* Overlong encoding */
        }
      }
      /* Also reject sequences > U+10FFFF (max valid Unicode) */
      if (byte >= 0xF5) {
        return 0; /* Out of range */
      }
    }

    i += char_len;
  }

  return 1; /* Valid UTF-8 */
}

int clogging_utf8_strlen(const char *str) {
  if (str == NULL) {
    return -1;
  }

  int char_count = 0;
  size_t i = 0;

  while (str[i] != '\0') {
    unsigned char byte = (unsigned char)str[i];
    int char_len = clogging_utf8_char_length(byte);

    if (char_len < 0) {
      /* Invalid UTF-8 */
      return -1;
    }

    /* Verify continuation bytes exist and are valid */
    for (int j = 1; j < char_len; j++) {
      if (str[i + j] == '\0') {
        /* Premature end of string */
        return -1;
      }
      if (!clogging_utf8_is_continuation((unsigned char)str[i + j])) {
        /* Invalid continuation byte */
        return -1;
      }
    }

    i += char_len;
    char_count++;
  }

  return char_count;
}

#ifdef _WIN32

int clogging_utf8_from_wide(const wchar_t *wide, char *utf8, size_t utf8_size) {
  if (wide == NULL || utf8 == NULL || utf8_size == 0) {
    return -1;
  }

  /* Use WideCharToMultiByte to convert UTF-16 to UTF-8 */
  int required_size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
  if (required_size <= 0) {
    return -1;
  }

  /* Check if output buffer is large enough (required_size includes null terminator) */
  if ((size_t)required_size > utf8_size) {
    return -1; /* Buffer too small */
  }

  /* Perform the actual conversion */
  int bytes_written = WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, (int)utf8_size, NULL, NULL);
  if (bytes_written <= 0) {
    return -1;
  }

  /* Return length without null terminator */
  return bytes_written - 1;
}

int clogging_utf8_to_wide(const char *utf8, wchar_t *wide, size_t wide_size) {
  if (utf8 == NULL || wide == NULL || wide_size == 0) {
    return -1;
  }

  /* First validate that input is valid UTF-8 */
  if (clogging_utf8_validate(utf8, 0) != 1) {
    return -1;
  }

  /* Use MultiByteToWideChar to convert UTF-8 to UTF-16 */
  int required_size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
  if (required_size <= 0) {
    return -1;
  }

  /* Check if output buffer is large enough (required_size includes null terminator) */
  if ((size_t)required_size > wide_size) {
    return -1; /* Buffer too small */
  }

  /* Perform the actual conversion */
  int wide_chars_written = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, (int)wide_size);
  if (wide_chars_written <= 0) {
    return -1;
  }

  /* Return length without null terminator */
  return wide_chars_written - 1;
}

int clogging_init_utf8_console(void) {
  /* Set console output code page to UTF-8 */
  if (!SetConsoleOutputCP(CP_UTF8)) {
    return -1;
  }
  /* Also set input code page to UTF-8 */
  if (!SetConsoleCP(CP_UTF8)) {
    return -1;
  }
  return 0;
}

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif
