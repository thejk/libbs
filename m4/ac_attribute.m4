
AC_DEFUN([AC_C___ATTRIBUTE__],[
  AC_CACHE_CHECK([for __attribute__],
                 [ac_cv___attribute__],
                 [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#include <stdlib.h>
int func(int x); int foo(int x __attribute__ ((unused))) { exit(1); }
])], [ac_cv___attribute__=yes], [ac_cv___attribute__=no])])
  if test "x$ac_cv___attribute__" = xyes; then
    AC_DEFINE([HAVE___ATTRIBUTE__], [1], [Define to 1 if your compiler has __attribute__])
  fi
])

