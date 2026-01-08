/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#include "utf8_utils.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Test valid ASCII strings (single byte UTF-8) */
void test_valid_ascii(void) {
  const char *ascii = "Hello World";
  assert(clogging_utf8_validate(ascii, 0) == 1);
  assert(clogging_utf8_strlen(ascii) == 11);
  printf("✓ Valid ASCII test passed\n");
}

/* Test valid 2-byte UTF-8 (e.g., é, ñ) */
void test_valid_2byte(void) {
  /* é = U+00E9 = C3 A9 in UTF-8 */
  const char *utf8_2byte = "caf\xC3\xA9"; /* café */
  assert(clogging_utf8_validate(utf8_2byte, 0) == 1);
  assert(clogging_utf8_strlen(utf8_2byte) == 4);
  printf("✓ Valid 2-byte UTF-8 test passed\n");
}

/* Test valid 3-byte UTF-8 (e.g., Chinese characters) */
void test_valid_3byte(void) {
  /* 你 = U+4F60 = E4 BD A0 in UTF-8 */
  const char *utf8_3byte = "\xE4\xBD\xA0"; /* 你 */
  assert(clogging_utf8_validate(utf8_3byte, 0) == 1);
  assert(clogging_utf8_strlen(utf8_3byte) == 1);
  printf("✓ Valid 3-byte UTF-8 test passed\n");
}

/* Test valid 4-byte UTF-8 (e.g., emoji) */
void test_valid_4byte(void) {
  /* 🚀 = U+1F680 = F0 9F 9A 80 in UTF-8 */
  const char *utf8_4byte = "\xF0\x9F\x9A\x80"; /* 🚀 */
  assert(clogging_utf8_validate(utf8_4byte, 0) == 1);
  assert(clogging_utf8_strlen(utf8_4byte) == 1);
  printf("✓ Valid 4-byte UTF-8 test passed\n");
}

/* Test mixed ASCII and multi-byte */
void test_mixed_utf8(void) {
  /* "Hello 🌍" = H e l l o space 🌍 = 6 ASCII + 1 emoji = 7 chars */
  const char *mixed = "Hello \xF0\x9F\x8C\x8D"; /* Hello 🌍 */
  assert(clogging_utf8_validate(mixed, 0) == 1);
  assert(clogging_utf8_strlen(mixed) == 7);
  printf("✓ Mixed ASCII and UTF-8 test passed\n");
}

/* Test empty string */
void test_empty_string(void) {
  const char *empty = "";
  assert(clogging_utf8_validate(empty, 0) == 1);
  assert(clogging_utf8_strlen(empty) == 0);
  printf("✓ Empty string test passed\n");
}

/* Test NULL pointer handling */
void test_null_pointer(void) {
  assert(clogging_utf8_validate(NULL, 0) == -1);
  assert(clogging_utf8_strlen(NULL) == -1);
  printf("✓ NULL pointer handling test passed\n");
}

/* Test invalid continuation byte */
void test_invalid_continuation(void) {
  /* 0xC3 (start of 2-byte) followed by 0x00 (not continuation) */
  const char invalid[] = {0xC3, 0x00, 0x00};
  assert(clogging_utf8_validate(invalid, 2) == 0);
  printf("✓ Invalid continuation byte test passed\n");
}

/* Test incomplete sequence */
void test_incomplete_sequence(void) {
  /* Start of 3-byte UTF-8 (E4) but only 2 bytes total */
  const char incomplete[] = {0xE4, 0xBD, 0x00};
  assert(clogging_utf8_validate(incomplete, 2) == 0);
  printf("✓ Incomplete sequence test passed\n");
}

/* Test overlong encoding (security issue) */
void test_overlong_encoding(void) {
  /* 'A' (U+0041) normally encoded as 0x41
   * but overlong as 0xC0 0x81 (invalid) */
  const char overlong[] = {0xC0, 0x81, 0x00};
  assert(clogging_utf8_validate(overlong, 2) == 0);
  printf("✓ Overlong encoding test passed\n");
}

/* Test UTF-16 surrogates (invalid in UTF-8) */
void test_utf16_surrogate(void) {
  /* UTF-16 surrogate pair range: ED A0 80 - ED BF BF
   * This is invalid in UTF-8 */
  const char surrogate[] = {0xED, 0xA0, 0x80, 0x00};
  assert(clogging_utf8_validate(surrogate, 3) == 0);
  printf("✓ UTF-16 surrogate rejection test passed\n");
}

/* Test continuation byte checking */
void test_continuation_byte(void) {
  assert(clogging_utf8_is_continuation(0x80) == 1); /* 10000000 */
  assert(clogging_utf8_is_continuation(0xBF) == 1); /* 10111111 */
  assert(clogging_utf8_is_continuation(0x7F) == 0); /* 01111111 */
  assert(clogging_utf8_is_continuation(0xC0) == 0); /* 11000000 */
  printf("✓ Continuation byte detection test passed\n");
}

/* Test character length detection */
void test_char_length(void) {
  assert(clogging_utf8_char_length(0x41) == 1);   /* 'A' - ASCII */
  assert(clogging_utf8_char_length(0xC3) == 2);   /* Start of 2-byte */
  assert(clogging_utf8_char_length(0xE4) == 3);   /* Start of 3-byte */
  assert(clogging_utf8_char_length(0xF0) == 4);   /* Start of 4-byte */
  assert(clogging_utf8_char_length(0x80) == -1);  /* Continuation (invalid start) */
  assert(clogging_utf8_char_length(0xFF) == -1);  /* Invalid */
  printf("✓ Character length detection test passed\n");
}

int main(void) {
  printf("Running UTF-8 validation tests...\n\n");

  test_valid_ascii();
  test_valid_2byte();
  test_valid_3byte();
  test_valid_4byte();
  test_mixed_utf8();
  test_empty_string();
  test_null_pointer();
  test_invalid_continuation();
  test_incomplete_sequence();
  test_overlong_encoding();
  test_utf16_surrogate();
  test_continuation_byte();
  test_char_length();

  printf("\n✓ All UTF-8 validation tests passed!\n");
  return 0;
}
