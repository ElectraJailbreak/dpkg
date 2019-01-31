# Copyright © 2005 Scott James Remnant <scott@netsplit.com>
# Copyright © 2007 Frank Lichtenheld <djpig@debian.org>
# Copyright © 2007, 2009, 2011 Guillem Jover <guillem@debian.org>

# DPKG_PROG_PERL
# --------------
# Locate perl interpreter in the path
AC_DEFUN([DPKG_PROG_PERL], [
  AC_ARG_VAR([PERL], [Perl interpreter])dnl
  m4_define([_PERL_MIN_VERSION], [5.20.2])
  AC_SUBST([PERL_MIN_VERSION], [_PERL_MIN_VERSION])
  AC_CACHE_CHECK([for perl >= _PERL_MIN_VERSION], [ac_cv_path_PERL], [
    AC_PATH_PROGS_FEATURE_CHECK([PERL], [perl], [
      perlcheck=$(test -x $ac_path_PERL && \
                  $ac_path_PERL -MConfig -Mversion -e \
                  'my $r = qv("v$Config{version}") >= qv("_PERL_MIN_VERSION");
                   print "yes" if $r')
      AS_IF([test "x$perlcheck" = "xyes"], [
        ac_cv_path_PERL=$ac_path_PERL ac_path_PERL_found=:
      ])
    ], [
      AC_MSG_ERROR([cannot find perl >= _PERL_MIN_VERSION])
    ])
  ])
  AC_SUBST([PERL], [$ac_cv_path_PERL])
  AC_ARG_VAR([PERL_LIBDIR], [Perl library directory])dnl
  # Let the user override the variable.
  AS_IF([test -z "$PERL_LIBDIR"], [
    PERL_LIBDIR=$($PERL -MConfig -e \
                        'my $r = $Config{vendorlibexp};
                         $r =~ s/$Config{vendorprefixexp}/\$(prefix)/;
                         print $r')
  ])
])# DPKG_PROG_PERL

# DPKG_PROG_PO4A
# --------------
AC_DEFUN([DPKG_PROG_PO4A], [
  AC_REQUIRE([AM_NLS])
  AC_ARG_VAR([PO4A], [po4a program])
  AC_CHECK_PROGS([PO4A], [po4a])
  AS_IF([test "$USE_NLS" = "yes" && test -n "$PO4A"], [
    USE_PO4A=yes
  ], [
    USE_PO4A=no
  ])
  AC_SUBST([USE_PO4A])
])# DPKG_PROG_PO4A

# DPKG_PROG_POD2MAN
# --------------
AC_DEFUN([DPKG_PROG_POD2MAN], [
  AC_ARG_VAR([POD2MAN], [pod2man program])
  AC_CHECK_PROGS([POD2MAN], [pod2man])
  AM_CONDITIONAL([BUILD_POD_DOC], [test "x$POD2MAN" != "x"])
])# DPKG_PROG_POD2MAN

# DPKG_DEB_PROG_TAR
# -----------------
# Specify GNU tar program name to use by dpkg-deb. On GNU systems this is
# usually simply tar, on BSD systems this is usually gnutar or gtar.
AC_DEFUN([DPKG_DEB_PROG_TAR], [
  AC_ARG_VAR([TAR], [GNU tar program])
  AC_CHECK_PROGS([TAR], [gnutar gtar tar], [tar])
  AS_IF([! $TAR --version 2>/dev/null | grep -q '^tar (GNU tar)'], [
    AC_MSG_ERROR([cannot find a GNU tar program])
  ])
  AC_DEFINE_UNQUOTED([TAR], ["$TAR"], [GNU tar program])
])# DPKG_DEB_PROG_TAR

# DPKG_PROG_PATCH
# ---------------
# Specify GNU patch program name to use by dpkg-source. On GNU systems this
# is usually simply patch, on BSD systems this is usually gpatch.
# Even though most invocations would work with other patch implementations,
# currently only GNU patch is directory traversal resistant.
AC_DEFUN([DPKG_PROG_PATCH], [
  AC_ARG_VAR([PATCH], [GNU patch program])
  AC_CHECK_PROGS([PATCH], [gpatch patch], [patch])
  AS_IF([! $PATCH --version 2>/dev/null | grep -q '^GNU patch'], [
    AC_MSG_ERROR([cannot find a GNU patch program])
  ])
  AC_DEFINE_UNQUOTED([PATCH], ["$PATCH"], [GNU patch program])
])# DPKG_PROG_PATCH
