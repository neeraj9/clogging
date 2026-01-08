# clogging

[![Software License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square)](LICENSE.txt)


A logging library for c programming language. It adhers to
[ISO C17](https://en.wikipedia.org/wiki/C17_(C_standard_revision)) C standard.

> It is built with -std=c17.
> It is built with dynamic TLS model in case you want to use the
> shared library. Alternatively, you can statically link with
> this library.

## Using Basic Logging on Console

Take a look at the examples for details, but basically you could do the
following to get started.

    /* save as test.c */
    #include <clogging/basic_logging.h>

    int main(int argc, char *argv[])
    {
        clogging_basic_init(argv[0], "", LOG_LEVEL_DEBUG, NULL);
        BASIC_LOG_DEBUG("A basic debug log looks like this");
        return 0;
    }

Now you can compile this test.c as follows:

    $ gcc -lclogging test.c


The above is a starting point to get things started.

## Write Code to Abstract Out Specific Logging Type

You can alternatively write a logging.h which makes generic use of logging
as follows:

    #ifndef LOGGING_H
    #define LOGGING_H

    #include "basic_logging.h"

    #define SET_LOG_LEVEL BASIC_SET_LOG_LEVEL
    #define GET_LOG_LEVEL BASIC_GET_LOG_LEVEL
    #define GET_NUM_DROPPED_MESSAGES BASIC_GET_NUM_DROPPED_MESSAGES

    #define LOG_ERROR BASIC_LOG_ERROR
    #define LOG_WARN BASIC_LOG_WARN
    #define LOG_INFO BASIC_LOG_INFO
    #define LOG_DEBUG BASIC_LOG_DEBUG

    #endif /* LOGGING_H */

The main application code will now look like as follows:

    #include "logging.h"

    int main(int argc, char *argv[])
    {
        /* the second argument is the total size of the buffer including null byte */
        clogging_basic_init(argv[0], "", LOG_LEVEL_DEBUG, NULL);
        LOG_DEBUG("A basic debug log looks like this");
        return 0;
    }

## Prerequisites

- CMake 3.10 or later
- C compiler (gcc, clang, MSVC, etc.)
- POSIX threads library (pthread)
- Optional: pkg-config (for development)

## Quick Start

### Linux/macOS

```bash
# Create build directory
mkdir build
cd build

# Configure the project
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build .

# Run tests
ctest

# Install (optional)
sudo cmake --install . --prefix /usr/local
```

### Windows (with Visual Studio)

> With multi-config generators, CMAKE_BUILD_TYPE is ignored—the configuration is selected
> at build time, not configure time.When you run cmake --build . without specifying a config,
> Visual Studio defaults to Debug. You need to explicitly specify the configuration.

```bash
# Create build directory
mkdir build
cd build

# Configure the project (Visual Studio 2019 or later)
cmake .. -G "Visual Studio 16 2019"

# Build
cmake --build . --config Debug

# Run tests
ctest

# Install
cmake --install . --prefix "C:\Program Files\clogging"
```

## Build Options

- `BUILD_TESTS` (ON/OFF, default: ON) - Build test executables
- `BUILD_SHARED_LIBS` (ON/OFF, default: ON) - Build shared libraries instead of static
- `CLOGGING_USE_UTF8_STRINGS` (ON/OFF, default: OFF) - Enable UTF-8 string validation and utilities

Example:
```bash
cmake .. -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DCLOGGING_USE_UTF8_STRINGS=ON
```

## UTF-8 String Support

clogging supports UTF-8 encoded strings when built with the `CLOGGING_USE_UTF8_STRINGS` option:

```bash
cmake -DCLOGGING_USE_UTF8_STRINGS=ON ..
```

### What UTF-8 Support Enables

- **UTF-8 Validation**: In debug builds, validate that all strings passed to logging functions are valid UTF-8
- **Windows UTF-16 Conversion**: Helper functions to convert Windows API strings (UTF-16) to UTF-8
- **Unicode Output**: Proper handling of international characters, emoji, and special symbols
- **Console Support**: Automatic Windows console configuration for UTF-8 output

### Using UTF-8 Strings

All string parameters to logging functions should be UTF-8 encoded:

```c
#include "logging.h"
#include "utf8_utils.h"

int main(void) {
    /* the second argument is the total size of the buffer including null byte */
    clogging_basic_init("myapp", "", LOG_LEVEL_INFO, NULL);
    
    // ASCII works everywhere
    LOG_INFO("Hello World!");
    
    // UTF-8 with international characters
    LOG_INFO("Café");  // Works on UTF-8 systems
    LOG_INFO("Hello 🚀");  // Emoji support
    LOG_INFO("你好世界");  // Chinese characters
    
    return 0;
}
```

### Windows-Specific Usage

On Windows, convert UTF-16 strings from the Windows API to UTF-8:

```c
#include "logging.h"
#include "utf8_utils.h"
#include <windows.h>

int main(void) {
    // Initialize UTF-8 console support
    clogging_init_utf8_console();
    
    /* the second argument is the total size of the buffer including null byte */
    clogging_basic_init("myapp", "", LOG_LEVEL_INFO, NULL);
    
    // Get Windows username (UTF-16)
    wchar_t wide_username[256];
    DWORD size = 256;
    if (GetUserNameW(wide_username, &size)) {
        // Convert UTF-16 to UTF-8
        char utf8_username[512];
        clogging_utf8_from_wide(wide_username, utf8_username, sizeof(utf8_username));
        
        // Use the UTF-8 string
        LOG_INFO("User: %s", utf8_username);
    }
    
    return 0;
}
```

### Building with UTF-8 Support

**Linux/macOS:**
```bash
cmake -DCLOGGING_USE_UTF8_STRINGS=ON ..
cmake --build .
ctest  # Runs UTF-8 tests
```

**Windows (MSVC):**
```bash
cmake -DCLOGGING_USE_UTF8_STRINGS=ON -G "Visual Studio 16 2019" ..
cmake --build . --config Debug
ctest
```

The UTF-8 flag automatically:
- Enables the `/utf-8` compiler flag on MSVC
- Ensures source files are treated as UTF-8 encoded
- Builds UTF-8 validation and conversion utilities
- Enables UTF-8 tests

### UTF-8 API Reference

When `CLOGGING_USE_UTF8_STRINGS` is enabled, these functions are available:

**Validation:**
- `int clogging_utf8_validate(const char *str, size_t len)` - Validate UTF-8 encoding
- `int clogging_utf8_strlen(const char *str)` - Get character count (not byte count)

**Windows Conversion (Windows only):**
- `int clogging_utf8_from_wide(const wchar_t *wide, char *utf8, size_t utf8_size)` - Convert UTF-16 to UTF-8
- `int clogging_utf8_to_wide(const char *utf8, wchar_t *wide, size_t wide_size)` - Convert UTF-8 to UTF-16
- `int clogging_init_utf8_console(void)` - Configure Windows console for UTF-8

**Utilities:**
- `int clogging_utf8_is_continuation(unsigned char byte)` - Check for UTF-8 continuation byte
- `int clogging_utf8_char_length(unsigned char first_byte)` - Get UTF-8 character length

See `utf8_utils.h` for detailed documentation on each function.

### Performance Considerations

- UTF-8 validation only occurs in **debug builds** (no overhead in production)
- Conversion utilities add minimal overhead (just before I/O)
- Default mode (without the flag) has **zero performance impact**
- Recommended: Use the flag during development, disable for production if not needed

### Common Issues

**On Windows, emoji and international characters don't display correctly:**
1. Make sure your source files are saved as UTF-8 (with or without BOM)
2. Use `/utf-8` compiler flag: `cmake .. -DCLOGGING_USE_UTF8_STRINGS=ON`
3. Call `clogging_init_utf8_console()` early in main()

**Validation errors in debug builds:**
- Ensure all strings passed to logging functions are valid UTF-8
- On Windows, convert from UTF-16 using `clogging_utf8_from_wide()`
- Check that file encoding is UTF-8 (not ANSI/CP-1252)

## Installation

### Standard Installation

```bash
cmake --install . --prefix /usr/local
```

This will install:
- Libraries to `${prefix}/lib`
- Headers to `${prefix}/include/clogging`
- CMake config files to `${prefix}/lib/cmake/clogging`
- pkg-config file to `${prefix}/lib/pkgconfig`

### Custom Prefix

```bash
cmake --install . --prefix /custom/prefix
```

## Using the Library

### With pkg-config

```bash
gcc -c myapp.c $(pkg-config --cflags clogging)
gcc -o myapp myapp.o $(pkg-config --libs clogging)
```

### With CMake (Recommended)

In your `CMakeLists.txt`:

```cmake
find_package(clogging REQUIRED)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE clogging::clogging)
```

Then:
```bash
cmake .. -Dclogging_DIR=/usr/local/lib/cmake/clogging
```

### Manual Linking

```bash
gcc -I/usr/local/include -c myapp.c
gcc -o myapp myapp.o -L/usr/local/lib -lclogging -lpthread
```

## Development Workflow

### In-source Build (for development)

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest --output-on-failure
```

### Out-of-source Build (recommended)

The above example already uses out-of-source build, keeping source tree clean.

## CMake Configuration Files

The project installs the following CMake files:
- `cloggingConfig.cmake` - Package configuration file
- `cloggingConfigVersion.cmake` - Version information
- `cloggingTargets.cmake` - Target definitions

These enable other CMake projects to find and use clogging with:
```cmake
find_package(clogging CONFIG REQUIRED)
```

## Running tests

Run tests in `Debug` configuration as follows:

```
mkdir build
cd build
cmake ..
cmake --build .
ctest -C Debug
```

### References

* [ISO/IEC 9899:2011](http://www.iso.org/iso/home/store/catalogue_ics/catalogue_detail_ics.htm?csnumber=57853)
  - The ISO C99 C standard.
