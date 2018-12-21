# Copyright © 2005 Scott James Remnant <scott@netsplit.com>
# Copyright © 2008-2009,2015 Guillem Jover <guillem@debian.org>

# DPKG_FUNC_VA_COPY
# -----------------
# Define HAVE_VA_COPY if we have va_copy.
AC_DEFUN([DPKG_FUNC_VA_COPY], [
  AC_CACHE_CHECK([for va_copy], [dpkg_cv_va_copy], [
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[
#include <stdarg.h>
      ]], [[
va_list v1, v2;
va_copy(v1, v2);
      ]])
    ], [
      dpkg_cv_va_copy=yes
    ], [
      dpkg_cv_va_copy=no
    ])
  ])
  AS_IF([test "x$dpkg_cv_va_copy" = "xyes"], [
    AC_DEFINE([HAVE_VA_COPY], [1], [Define to 1 if the 'va_copy' macro exists])
  ])
])# DPKG_FUNC_VA_COPY

# DPKG_FUNC_C99_SNPRINTF
# -----------------------
# Define HAVE_C99_SNPRINTF if we have C99 snprintf family semantics
AC_DEFUN([DPKG_FUNC_C99_SNPRINTF], [
  AC_CACHE_CHECK([for C99 snprintf functions], [dpkg_cv_c99_snprintf], [
    AC_RUN_IFELSE([
      AC_LANG_SOURCE([[
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
int test_vsnprintf(const char *fmt, ...)
{
	int n;
	va_list args;

	va_start(args, fmt);
	n = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	return n;
}
int main()
{
	int n;

	n = snprintf(NULL, 0, "format %s %d", "string", 10);
	if (n != strlen("format string 10"))
		return 1;

	n = test_vsnprintf("format %s %d", "string", 10);
	if (n != strlen("format string 10"))
		return 1;

	return 0;
}
      ]])
    ], [
      dpkg_cv_c99_snprintf=yes
    ], [
      dpkg_cv_c99_snprintf=no
    ], [
      dpkg_cv_c99_snprintf=maybe
    ])

    AS_IF([test "x$dpkg_cv_c99_snprintf" = "xmaybe"], [
      AC_COMPILE_IFELSE([
        AC_LANG_SOURCE([[
#define _GNU_SOURCE 1
#include <unistd.h>
#if !defined(_XOPEN_VERSION) || _XOPEN_VERSION < 600
#error "snprintf() has conflicting semantics with C99 on SUSv2 and earlier"
#endif
        ]])
      ], [
        dpkg_cv_c99_snprintf=yes
      ], [
        dpkg_cv_c99_snprintf=no
      ])
    ])
  ])
  AS_IF([test "x$dpkg_cv_c99_snprintf" = "xyes"], [
    AC_DEFINE([HAVE_C99_SNPRINTF], [1],
      [Define to 1 if the 'snprintf' family is C99 conformant])
  ])
  AM_CONDITIONAL([HAVE_C99_SNPRINTF], [test "x$dpkg_cv_c99_snprintf" = "xyes"])
])# DPKG_FUNC_C99_SNPRINTF

# DPKG_USE_MMAP
# -------------
# Define USE_MMAP if mmap() is available and it was requested
AC_DEFUN([DPKG_USE_MMAP], [
  AC_ARG_ENABLE([mmap],
    [AS_HELP_STRING([--enable-mmap],
      [enable usage of unrealiable mmap if available])],
    [], [enable_mmap=no])

  AS_IF([test "x$enable_mmap" = "xyes"], [
    AC_CHECK_FUNCS([mmap])
    AC_DEFINE([USE_MMAP], [1], [Use unreliable mmap support])
  ])
])

# DPKG_USE_DISK_PREALLOCATE
# -------------------------
# Define USE_DISK_PREALLOCATE if disk size pre-allocation is available
# and it was requested.
AC_DEFUN([DPKG_USE_DISK_PREALLOCATE], [
  AC_ARG_ENABLE([disk-preallocate],
    [AS_HELP_STRING([--enable-disk-preallocate],
      [enable usage of disk size pre-allocation])],
    [], [enable_disk_preallocate=no])

  AS_IF([test "x$enable_disk_preallocate" = "xyes"], [
    AC_DEFINE([USE_DISK_PREALLOCATE], [1], [Use disk size pre-allocation])
  ])
])

# DPKG_CHECK_PROGNAME
# -------------------
# Check for system implementations of program name tracking.
AC_DEFUN([DPKG_CHECK_PROGNAME], [
  AC_MSG_CHECKING([for program_invocation_short_name])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM(
      [[#include <errno.h>]],
      [[const char *p = program_invocation_short_name;]])
  ], [
    AC_DEFINE([HAVE_PROGRAM_INVOCATION_SHORT_NAME], [1],
      [Define to 1 if you have program_invocation_short_name])
    AC_MSG_RESULT([yes])
  ], [
    AC_MSG_RESULT([no])
  ])

  AC_MSG_CHECKING([for __progname])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM(
      [[extern char *__progname;]],
      [[printf("%s", __progname);]])
  ], [
    AC_DEFINE([HAVE___PROGNAME], [1], [Define to 1 if you have __progname])
    AC_MSG_RESULT([yes])
  ], [
    AC_MSG_RESULT([no])
  ])
]) # DPKG_CHECK_PROGNAME

# DPKG_CHECK_COMPAT_FUNCS(LIST)
# -----------------------
# Check each function and define an automake conditional
AC_DEFUN([DPKG_CHECK_COMPAT_FUNCS], [
  AC_CHECK_FUNCS([$1])
  m4_foreach_w([ac_func], [$1], [
    AM_CONDITIONAL([HAVE_]AS_TR_CPP(ac_func),
      [test "x$ac_cv_func_[]AS_TR_SH(ac_func)" = "xyes"])
  ])
]) # DPKG_CHECK_COMPAT_FUNCS
