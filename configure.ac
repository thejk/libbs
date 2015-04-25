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

DEFINES="-D_GNU_SOURCE" # -D_BSD_SOURCE -D_ATFILE_SOURCE"
AX_CFLAGS_WARN_ALL([DEFINES])
AX_APPEND_COMPILE_FLAGS([-Wextra], [DEFINES])
AC_ARG_ENABLE([debug], AC_HELP_STRING([compile with debug options]),
              if test "x$enableval" = "xyes"; then
                DEFINES="$DEFINES -g -DDEBUG"
              else
                DEFINES="$DEFINES -DNDEBUG"
              fi)
AC_SUBST([DEFINES])

LIB_DEFINES="$DEFINES"
AX_APPEND_COMPILE_FLAGS([-fvisbility=hidden], [LIB_DEFINES])
AC_SUBST([LIB_DEFINES])

AC_CHECK_HEADER([stdint.h],,AC_MSG_ERROR([Need stdint.h]))
AC_CHECK_HEADER([stdbool.h],,AC_MSG_ERROR([Need stdbool.h]))
AC_CHECK_FUNCS([getopt_long])

AC_ARG_ENABLE([udev-rules],AS_HELP_STRING([--enable-udev-rules],[install udev rules (default is no)]),[install_udev_rules=$enableval],[install_udev_rules=no])

AM_CONDITIONAL([INSTALL_UDEV_RULES],[test "x$install_udev_rules" = xyes])

PKG_CHECK_MODULES([LIBUSB], [libusb >= 0.1])

AC_CONFIG_FILES([data/libbs.pc])
AC_CONFIG_HEADERS([src/config.h])
AC_OUTPUT([Makefile data/Makefile src/Makefile])
