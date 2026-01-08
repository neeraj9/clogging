#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([clogging], [0.1], [neerajsharma.9@outlook.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/basic_logging.c])
AC_CONFIG_HEADERS([src/config.h])

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

# enable perf to show correct function names by uncommenting the
# following line
#CFLAGS+=" -fno-omit-frame-pointer"

# find the full filename of the compiled file
CFLAGS+=" "
#CFLAGS+=-D__FILENAME__='"$(subst $(SOURCE_PREFIX)/,,$(readlink -f $<))"'
#CFLAGS+=-D__FILENAME__='"$(readlink -f $<)"'
#CFLAGS+="-D__FILENAME__='\$(top_srcdir)/$<'"
CFLAGS+=-D__FILENAME__='"\"`readlink -f $< | sed -e \"s|$(SOURCE_PREFIX)/|$(PACKAGE)/|\"`@$(VERSION)\""'
#CFLAGS+=-D__FILENAME__='"`readlink -f $< | sed -e \"s|$(SOURCE_PREFIX)/|$(PACKAGE)/|\"`"'
CFLAGS+=" "

SOURCE_PREFIX='$(shell readlink -f $(top_srcdir))'
# substitute custom variables
AC_SUBST(SOURCE_PREFIX)


AC_CONFIG_FILES([
	Makefile
	src/Makefile
	libs/clogging.pc
	libs/Makefile
	tests/Makefile
	])

AC_OUTPUT
