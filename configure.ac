#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([zoodis], [0.1], [jinoos@gmail.com])
AM_INIT_AUTOMAKE(hello, 0.1)
AC_CONFIG_SRCDIR([src/nalloc.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

AC_CHECK_LIB([zookeeper_mt],[getenv],[],[AC_MSG_ERROR([Please install zookeeper library (multithread devel)])])
AC_CHECK_HEADERS([zookeeper/zookeeper.h],, AC_MSG_ERROR([zookeeper library headers not found. Please install zookeeper-mt-dev library (multithread devel)]))

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset select socket strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
