/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifdef _WIN32

#include "utf8_utils.h"
#include "logging.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

/* Test Windows UTF-16 to UTF-8 conversion */
void test_wide_to_utf8(void) {
  printf("Test: UTF-16 to UTF-8 conversion\n");

  /* Test 1: Simple ASCII conversion */
  wchar_t wide_ascii[] = L"Hello World";
  char utf8_buffer[256];
  int result = clogging_utf8_from_wide(wide_ascii, utf8_buffer, sizeof(utf8_buffer));
  assert(result > 0);
  assert(strcmp(utf8_buffer, "Hello World") == 0);
  printf("  ✓ ASCII conversion passed\n");

  /* Test 2: Latin characters with accents */
  wchar_t wide_latin[] = L"Caf\u00E9"; /* café */
  result = clogging_utf8_from_wide(wide_latin, utf8_buffer, sizeof(utf8_buffer));
  assert(result > 0);
  assert(utf8_buffer[0] == 'C');
  printf("  ✓ Latin character conversion passed\n");

  /* Test 3: Emoji (uses surrogate pairs in UTF-16) */
  wchar_t wide_emoji[] = L"\U0001F680"; /* 🚀 */
  result = clogging_utf8_from_wide(wide_emoji, utf8_buffer, sizeof(utf8_buffer));
  assert(result > 0);
  /* 🚀 in UTF-8 is F0 9F 9A 80 (4 bytes) */
  assert((unsigned char)utf8_buffer[0] == 0xF0);
  assert((unsigned char)utf8_buffer[1] == 0x9F);
  assert((unsigned char)utf8_buffer[2] == 0x9A);
  assert((unsigned char)utf8_buffer[3] == 0x80);
  printf("  ✓ Emoji conversion passed\n");

  /* Test 4: Chinese characters */
  wchar_t wide_chinese[] = L"\u4E2D\u6587"; /* 中文 */
  result = clogging_utf8_from_wide(wide_chinese, utf8_buffer, sizeof(utf8_buffer));
  assert(result > 0);
  printf("  ✓ Chinese character conversion passed\n");

  /* Test 5: Buffer too small */
  char small_buffer[5];
  result = clogging_utf8_from_wide(wide_ascii, small_buffer, sizeof(small_buffer));
  assert(result == -1); /* Should fail */
  printf("  ✓ Buffer overflow detection passed\n");

  /* Test 6: NULL pointer handling */
  result = clogging_utf8_from_wide(NULL, utf8_buffer, sizeof(utf8_buffer));
  assert(result == -1);
  result = clogging_utf8_from_wide(wide_ascii, NULL, sizeof(utf8_buffer));
  assert(result == -1);
  printf("  ✓ NULL pointer handling passed\n");
}

/* Test UTF-8 to UTF-16 conversion */
void test_utf8_to_wide(void) {
  printf("Test: UTF-8 to UTF-16 conversion\n");

  /* Test 1: ASCII conversion */
  const char utf8_ascii[] = "Hello World";
  wchar_t wide_buffer[256];
  int result = clogging_utf8_to_wide(utf8_ascii, wide_buffer, sizeof(wide_buffer) / sizeof(wchar_t));
  assert(result > 0);
  assert(wcscmp(wide_buffer, L"Hello World") == 0);
  printf("  ✓ ASCII conversion passed\n");

  /* Test 2: Latin with accents (UTF-8: C3 A9 for é) */
  const char utf8_latin[] = "Caf\xC3\xA9"; /* café */
  result = clogging_utf8_to_wide(utf8_latin, wide_buffer, sizeof(wide_buffer) / sizeof(wchar_t));
  assert(result > 0);
  assert(wide_buffer[0] == L'C');
  assert(wide_buffer[1] == L'a');
  assert(wide_buffer[2] == L'f');
  assert(wide_buffer[3] == L'\u00E9'); /* é */
  printf("  ✓ Latin character conversion passed\n");

  /* Test 3: Emoji (UTF-8: F0 9F 9A 80 for 🚀) */
  const char utf8_emoji[] = "\xF0\x9F\x9A\x80"; /* 🚀 */
  result = clogging_utf8_to_wide(utf8_emoji, wide_buffer, sizeof(wide_buffer) / sizeof(wchar_t));
  assert(result > 0);
  printf("  ✓ Emoji conversion passed\n");

  /* Test 4: Invalid UTF-8 should fail */
  const char invalid_utf8[] = "\xC3"; /* Incomplete UTF-8 sequence */
  result = clogging_utf8_to_wide(invalid_utf8, wide_buffer, sizeof(wide_buffer) / sizeof(wchar_t));
  assert(result == -1);
  printf("  ✓ Invalid UTF-8 detection passed\n");

  /* Test 5: Buffer too small */
  wchar_t small_buffer[3];
  result = clogging_utf8_to_wide(utf8_ascii, small_buffer, sizeof(small_buffer) / sizeof(wchar_t));
  assert(result == -1);
  printf("  ✓ Buffer overflow detection passed\n");
}

/* Test round-trip conversion */
void test_round_trip(void) {
  printf("Test: Round-trip conversion (UTF-16 -> UTF-8 -> UTF-16)\n");

  wchar_t original[] = L"Hello \u00E9 \U0001F680"; /* Hello é 🚀 */
  char utf8_buffer[256];
  wchar_t round_trip[256];

  /* Convert UTF-16 to UTF-8 */
  int result = clogging_utf8_from_wide(original, utf8_buffer, sizeof(utf8_buffer));
  assert(result > 0);

  /* Convert UTF-8 back to UTF-16 */
  result = clogging_utf8_to_wide(utf8_buffer, round_trip, sizeof(round_trip) / sizeof(wchar_t));
  assert(result > 0);

  /* Should match original */
  assert(wcscmp(original, round_trip) == 0);
  printf("  ✓ Round-trip conversion passed\n");
}

/* Test console initialization */
void test_console_init(void) {
  printf("Test: Console UTF-8 initialization\n");

  int result = clogging_init_utf8_console();
  assert(result == 0);
  printf("  ✓ Console UTF-8 initialization passed\n");
}

/* Test logging with Windows API */
void test_logging_with_windows_api(void) {
  printf("Test: Logging with Windows API strings\n");

  /* Initialize logging with UTF-8 support */
  clogging_basic_init("utf8_demo", 9 + 1, "", 0, LOG_LEVEL_DEBUG, NULL);
  clogging_init_utf8_console();

  /* Get Windows username (UTF-16 from Windows API) */
  wchar_t wide_username[256];
  DWORD size = 256;
  if (GetUserNameW(wide_username, &size)) {
    char utf8_username[768];
    int result = clogging_utf8_from_wide(wide_username, utf8_username, sizeof(utf8_username));
    if (result > 0) {
      LOG_INFO("Windows user: %s", utf8_username);
      assert(GET_NUM_DROPPED_MESSAGES() == 0);
      printf("  ✓ Windows username logging passed\n");
    }
  }

  /* Log emoji with UTF-8 */
  LOG_INFO("Windows logging with emoji: \xF0\x9F\x8E\x89"); /* 🎉 */
  assert(GET_NUM_DROPPED_MESSAGES() == 0);
  printf("  ✓ Emoji logging passed\n");
}

int main(void) {
  printf("Running UTF-8 Windows conversion tests...\n\n");

  test_wide_to_utf8();
  printf("\n");
  test_utf8_to_wide();
  printf("\n");
  test_round_trip();
  printf("\n");
  test_console_init();
  printf("\n");
  test_logging_with_windows_api();

  printf("\n✓ All UTF-8 Windows conversion tests passed!\n");
  return 0;
}

#else /* _WIN32 */

int main(void) {
  printf("This test is Windows-specific and requires -D_WIN32\n");
  return 0;
}

#endif /* _WIN32 */
