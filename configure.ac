AC_INIT([libbs], [0.1], [the_jk@yahoo.com])
AC_CONFIG_MACRO_DIR([m4])

AC_ISC_POSIX
AC_HEADER_STDC
AC_C_CONST
AC_C_INLINE
AC_C___ATTRIBUTE__
LT_INIT

AM_INIT_AUTOMAKE([dist-bzip2 foreign color-tests])
AM_SILENT_RULES([yes])
AC_PROG_CC
AM_PROG_CC_C_O

DEFINES="" #-D_GNU_SOURCE -D_BSD_SOURCE -D_ATFILE_SOURCE"
AX_CFLAGS_WARN_ALL([DEFINES])
AC_ARG_ENABLE([debug], AC_HELP_STRING([compile with debug options]),
              if test "x$enableval" = "xyes"; then
                DEFINES="$DEFINES -g -DDEBUG"
              else
                DEFINES="$DEFINES -DNDEBUG"
              fi)
AC_SUBST([DEFINES])

AC_CHECK_HEADER([stdint.h],,AC_MSG_ERROR([Need stdint.h]))
AC_CHECK_HEADER([stdbool.h],,AC_MSG_ERROR([Need stdbool.h]))

PKG_CHECK_MODULES([LIBUSB], [libusb])

AC_CONFIG_HEADERS([src/config.h])
AC_OUTPUT([Makefile src/Makefile])
