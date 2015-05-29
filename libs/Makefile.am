
pconfigdir = $(libdir)/pkgconfig
pconfig_DATA = $(PACKAGE).pc

lib_LTLIBRARIES = libclogging.la

libclogging_la_SOURCES = dummy.h

# listed in same order as top-level Makefile.am's SUBDIRS
libclogging_la_LIBADD = \
 ../src/libsrc.la

libclogging_la_LDFLAGS = -release @VERSION@
