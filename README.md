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
        BASIC_INIT_LOGGING(argv[0], 255, "", 0, LOG_LEVEL_DEBUG);
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

    #define INIT_LOGGING BASIC_INIT_LOGGING
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
        INIT_LOGGING(argv[0], "", LOG_LEVEL_DEBUG);
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

Example:
```bash
cmake .. -DBUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
```

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
