#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([clogging], [0.1], [neeraj.sharma@alumni.iitg.ernet.in])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/basic_logging.c])
AC_CONFIG_HEADERS([src/config.h])

# safe logging for multi-threaded environment
AC_DEFINE([LOGGING_OPTIONAL_TLS], [__thread], [Use thread local storage for MT-safety])

# Checks for programs.
AM_PROG_AR([ar])
AC_PROG_CC([gcc cc])
AC_PROG_LIBTOOL
AC_PROG_CC_STDC
AC_PROG_INSTALL

AC_CONFIG_MACRO_DIR([m4])

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create,,
  [AC_MSG_ERROR([required library pthread missing])])

AC_SEARCH_LIBS([gmtime_r], [rt])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(pthread.h,, [AC_MSG_ERROR([pthread.h required])])
AC_CHECK_HEADERS([string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([gethostname])

AC_CONFIG_FILES([
								 Makefile
								 src/Makefile
								 libs/clogging.pc
								 libs/Makefile
								 examples/Makefile
								 ])

AC_OUTPUT
