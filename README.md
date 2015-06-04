[![Build Status](https://travis-ci.org/neeraj9/clogging.svg?branch=master)](https://travis-ci.org/neeraj9/clogging)
[![Software License](https://img.shields.io/badge/license-LGPLv2.1-blue.svg?style=flat-square)](LICENSE.txt)

# clogging

A logging library for c programming language. It adhers to
[ISO C99](http://en.wikipedia.org/wiki/C99) C standard.

> It is built with -std=gnu99.
> It is built with dynamic TLS model in case you want to use the
> shared library. Alternatively, you can statically link with
> this library.

## Building

    sh bootstrap.sh
    ./configure --prefix=/usr/local
    make
    make install

## Using Basic Logging on Console

Take a look at the examples for details, but basically you could do the
following to get started.

    /* save as test.c */
    #include <clogging/basic_logging.h>

    int main(int argc, char *argv[])
    {
        BASIC_INIT_LOGGING(argv[0], "", LOG_LEVEL_DEBUG);
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


### References

* [ISO/IEC 9899:2011](http://www.iso.org/iso/home/store/catalogue_ics/catalogue_detail_ics.htm?csnumber=57853)
  - The ISO C99 C standard.
