#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([zoodis], [0.1], [jinoos@gmail.com])
AM_INIT_AUTOMAKE(hello, 0.1)
AC_CONFIG_SRCDIR([src/nalloc.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

# Checks for programs.
#AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

DEFAULT_ZOOKEEPER_PATH=/usr
AC_ARG_WITH([msgpack],AS_HELP_STRING([--with-msgpack=PATH], [Set msgpack library with PATH]))

AC_ARG_WITH([zookeeper],
    [AS_HELP_STRING([--with-zookeeper=PATH],
        [set specific zookeeper installation path.])],
    [],
    [with_zookeeper=default])

AC_ARG_WITH([zookeeper-static],
    [AS_HELP_STRING([--with-zookeeper-static=LIBRARY_ARCHIVE_PATH],
        [path of zookeeper library archive file as libzookeeper_mt.a ])],
    [AC_CHECK_FILES([${with_zookeeper_static}],
        [AC_MSG_NOTICE([found zookeeper library archive ${with_zookeeper_static}])],
        [AC_MSG_ERROR([Cannot find library archive file ${with_zookeeper_static}])])],
    [with_zookeeper_static=default])

if test "x$with_zookeeper" == "xdefault" || test "x$with_zookeeper" == "x"; then
    AC_CHECK_LIB([zookeeper_mt],[getenv],[],[AC_MSG_ERROR([Please install zookeeper library (multithread devel)])])
    AC_CHECK_HEADERS([zookeeper/zookeeper.h],[],[AC_MSG_ERROR([Please install zookeeper library (multithread devel)])])
else
    DEFAULT_ZOOKEEPER_PATH=$with_zookeeper

    AC_CHECK_FILES([${DEFAULT_ZOOKEEPER_PATH}/include/zookeeper/zookeeper.h ${DEFAULT_ZOOKEEPER_PATH}/lib/libzookeeper_mt.so],,
                    [AC_MSG_ERROR([Cannot find zookeeper header and library under ${DEFAULT_ZOOKEEPER_PATH}])])

    CFLAGS="$CFLAGS -I${DEFAULT_ZOOKEEPER_PATH}/include"
    LDFLAGS="$LDFLAGS -L${DEFAULT_ZOOKEEPER_PATH}/lib"
fi

if test "x$with_zookeeper_static" == "xdefault"; then
    LIBS="$LIBS -lzookeeper_mt"
else
    LDFLAGS="$with_zookeeper_static $LDFLAGS"
    LIBS="$LIBS -lm -lpthread"
fi


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
