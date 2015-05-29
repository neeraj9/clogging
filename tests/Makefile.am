
LDADD = $(LOCAL_LIBS)
LOCAL_LIBS = \
 ../libs/libclogging.la

TEST_EXE = \
 test_basic_logging \
 test_fd_logging

TEST_SH =

test_basic_logging_SOURCES = test_basic_logging.c
test_fd_logging_SOURCES = test_fd_logging.c

check_PROGRAMS = $(TEST_EXE)
TESTS = $(TEST_EXE) $(TEST_SH)
