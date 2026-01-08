/*
 * UTF-8 Logging Example for Windows
 * 
 * This example demonstrates using clogging with UTF-8 strings on Windows,
 * including conversion from Windows API UTF-16 (wide character) strings.
 * 
 * Compile and run with: cmake -DCLOGGING_USE_UTF8_STRINGS=ON
 */

#ifdef _WIN32

#include "logging.h"
#include "utf8_utils.h"

#include <stdio.h>
#include <windows.h>

int main(void) {
  printf("UTF-8 Logging Example for Windows\n");
  printf("==================================\n\n");

  /* Initialize Windows console for UTF-8 output */
  clogging_init_utf8_console();

  /* Initialize logging with UTF-8 support enabled */
  INIT_LOGGING("utf8_win_demo", 13 + 1, "", 0, LOG_LEVEL_INFO);

  /* Get Windows user name (returns UTF-16) */
  printf("Getting Windows user name...\n");
  wchar_t wide_username[256];
  DWORD size = 256;
  if (GetUserNameW(wide_username, &size)) {
    /* Convert UTF-16 to UTF-8 */
    char utf8_username[768];
    if (clogging_utf8_from_wide(wide_username, utf8_username, sizeof(utf8_username)) > 0) {
      LOG_INFO("Windows user: %s", utf8_username);
    }
  }

  /* Get Windows computer name (returns UTF-16) */
  printf("Getting Windows computer name...\n");
  wchar_t wide_computername[256];
  DWORD namesize = 256;
  if (GetComputerNameExW(ComputerNameDnsHostname, wide_computername, &namesize)) {
    /* Convert UTF-16 to UTF-8 */
    char utf8_computername[768];
    if (clogging_utf8_from_wide(wide_computername, utf8_computername, sizeof(utf8_computername)) > 0) {
      LOG_INFO("Computer name: %s", utf8_computername);
    }
  }

  /* Log various Unicode content */
  LOG_INFO("Starting application \xF0\x9F\x9A\x80"); /* 🚀 */

  /* Log with international characters */
  LOG_INFO("Status: OK \xE2\x9C\x93"); /* ✓ */
  LOG_INFO("Warning: Check logs \xE2\x9A\xA0"); /* ⚠ */

  /* Log with emoji and symbols */
  LOG_INFO("Debug mode enabled: \xF0\x9F\x90\x9B"); /* 🐛 */
  LOG_INFO("Performance: \xE2\x9C\x93 Excellent"); /* ✓ */

  /* Demonstrate format strings */
  LOG_INFO("Initialization complete - %d modules loaded", 5);

  /* Change log level to DEBUG */
  SET_LOG_LEVEL(LOG_LEVEL_DEBUG);
  LOG_DEBUG("Detailed logging enabled with emoji support \xF0\x9F\x98\x80"); /* 😀 */

  /* More complex example with converted strings */
  printf("\nDemonstrating string conversion:\n");
  
  /* Example: Convert a literal wide string */
  wchar_t wide_str[] = L"Application startup complete \U0001F389"; /* 🎉 */
  char utf8_str[512];
  if (clogging_utf8_from_wide(wide_str, utf8_str, sizeof(utf8_str)) > 0) {
    LOG_INFO("%s", utf8_str);
  }

  printf("\n✓ All Windows UTF-8 examples logged successfully!\n");
  printf("Note: Windows console should now display Unicode characters correctly.\n");
  return 0;
}

#else /* _WIN32 */

#include <stdio.h>

int main(void) {
  printf("This example is Windows-specific.\n");
  printf("Please run on Windows or compile with -DCMAKE_SYSTEM_NAME=Windows\n");
  return 1;
}

#endif /* _WIN32 */
