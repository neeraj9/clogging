# clogging

A logging library for c programming language

## Building

    sh bootstrap.sh
    ./configure --prefix=/usr/local
    make
    make install

## Using

Take a look at the examples for details, but basically you could do the
following to get started.

    #include <clogging/logging.h>

    int main(int argc, char *argv[])
    {
        init_logging(argv[0], "", LOG_LEVEL_DEBUG);
        LOG_DEBUG("A basic debug log looks like this");
        return 0;
    }

The above is a starting point to get things started.
