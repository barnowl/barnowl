dnl $Id$
dnl Process this file with autoconf to produce a configure script.
AC_INIT(owl.c)

AC_CONFIG_HEADER(config.h)

AC_PROG_CC

dnl If we're using GCC, enable all warnings
if test "$GCC" = yes; then
     CFLAGS="$CFLAGS -Wall -g";
fi

m4_include(/usr/share/aclocal/pkg.m4)
dnl m4_include(pkg.m4)

dnl Check for Athena
AC_MSG_CHECKING(for /usr/athena/include)
if test -d /usr/athena/include; then
	CFLAGS=${CFLAGS}\ -I/usr/athena/include
	CPPFLAGS=${CPPFLAGS}\ -I/usr/athena/include
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
AC_MSG_CHECKING(for /usr/athena/lib)
if test -d /usr/athena/lib; then
	LDFLAGS=${LDFLAGS}\ -L/usr/athena/lib
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

dnl Check for kerberosIV include
AC_MSG_CHECKING(for kerberosIV)
if test -d /usr/include/kerberosIV; then
        CFLAGS=${CFLAGS}\ -I/usr/include/kerberosIV
        CPPFLAGS=${CPPFLAGS}\ -I/usr/include/kerberosIV
        AC_MSG_RESULT(yes)
elif test -d /usr/local/include/kerberosIV; then
        CFLAGS=${CFLAGS}\ -I/usr/local/include/kerberosIV
        CPPFLAGS=${CPPFLAGS}\ -I/usr/local/include/kerberosIV
        AC_MSG_RESULT(yes)
else
        AC_MSG_RESULT(no)
fi

AC_CHECK_LIB(ncursesw, initscr,, AC_MSG_ERROR(No curses library found.))
AC_CHECK_LIB(com_err, com_err)
AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(k5crypto, krb5_derive_key)
dnl AC_CHECK_LIB(des425, req_act_vno)
AC_CHECK_LIB(des425, des_cbc_encrypt)
AC_CHECK_LIB(resolv, res_search)
AC_CHECK_LIB(krb5, krb5_get_credentials)
AC_CHECK_LIB(krb4, krb_sendauth,,
   AC_CHECK_LIB(krb, krb_sendauth))
dnl AC_CHECK_LIB(zephyr, ZGetSender,, AC_MSG_ERROR(No zephyr library found.))
AC_CHECK_LIB(zephyr, ZGetSender)
AC_CHECK_LIB(zephyr, ZInitLocationInfo, AC_DEFINE([HAVE_LIBZEPHYR_ZINITLOCATIONINFO], [], [Have ZInitLocationInfo]),)

AC_ARG_ENABLE([athstatic], AS_HELP_STRING([--enable-athstatic],
                                          [Statically link libraries in /usr/athena/lib]),
                                          [ATHSTATIC=./athstatic],
                                          [ATHSTATIC=])

AC_CHECK_FUNCS(use_default_colors resizeterm des_string_to_key des_key_sched des_ecb_encrypt)

AC_MSG_CHECKING(for des_ecb_encrypt prototype)
AC_TRY_COMPILE([#include <des.h>
int des_ecb_encrypt(char foo[], char bar[], des_key_schedule baz, int qux);],
[int foo = des_ecb_encrypt(0,0,0,0);],
ac_cv_des_ecb_encrypt_proto=no,
ac_cv_des_ecb_encrypt_proto=yes)
AC_MSG_RESULT($ac_cv_des_ecb_encrypt_proto)
if test "$ac_cv_des_ecb_encrypt_proto" = yes; then
	AC_DEFINE([HAVE_DES_ECB_ENCRYPT_PROTO], [], [have proto for des_ecb_encrypt])
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(strings.h sys/ioctl.h sys/filio.h unistd.h com_err.h)

dnl Add CFLAGS for embeded perl
FOO=`perl -MExtUtils::Embed -e ccopts`
echo Adding perl CFLAGS ${FOO}
CFLAGS=${CFLAGS}\ ${FOO}

dnl Find the location of perl XSUBPP
AC_MSG_CHECKING(for the perl xsubpp precompiler)
XSUBPPDIR="`(perl -MExtUtils::MakeMaker -e 'print ExtUtils::MakeMaker->new({NAME => qw(owl)})->tool_xsubpp;') | grep \^XSUBPPDIR | sed -e 's/XSUBPPDIR = //g;'`"
if test -n "${XSUBPPDIR}"; then
   AC_MSG_RESULT(${XSUBPPDIR})
else
   AC_MSG_ERROR(not found)
fi

if test -f "${XSUBPPDIR}/typemap"; then
   XSUBPPFLAGS="-typemap ${XSUBPPDIR}/typemap"
else
   XSUBPPFLAGS=""
fi

dnl Add LDFLAGS for embedded perl
FOO=`perl -MExtUtils::Embed -e ldopts`
echo Adding perl LDFLAGS ${FOO}
LDFLAGS=${LDFLAGS}\ ${FOO}

dnl Add CFLAGS and LDFLAGS for glib-2.0
PKG_CHECK_MODULES(GLIB,glib-2.0)

echo Adding glib-2.0 CFLAGS ${GLIB_CFLAGS}
CFLAGS="${GLIB_CFLAGS} ${CFLAGS}"
echo Adding glib-2.0 LDFLAGS ${GLIB_LIBS}
LDFLAGS="${GLIB_LIBS} ${LDFLAGS}"


dnl Checks for typedefs, structures, and compiler characteristics.

AC_SUBST(XSUBPPDIR)
AC_SUBST(XSUBPPFLAGS)
AC_SUBST(ATHSTATIC)

AC_PROG_INSTALL

AC_CONFIG_SUBDIRS(libfaim)

AC_OUTPUT(Makefile)
