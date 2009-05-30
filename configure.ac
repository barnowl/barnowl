dnl $Id$
dnl Process this file with autoconf to produce a configure script.
AC_INIT(owl.c)

AC_CONFIG_HEADER(config.h)

AC_PROG_CC

dnl If we're using GCC, enable all warnings
if test "$GCC" = yes; then
     CFLAGS="$CFLAGS -Wall -g";
fi

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
elif test -d /usr/include/openssl; then
        CFLAGS=${CFLAGS}\ -I/usr/include/openssl
        CPPFLAGS=${CPPFLAGS}\ -I/usr/include/openssl
        AC_MSG_RESULT(OpenSSL DES found instead)
else
        AC_MSG_RESULT(no)
fi
PROTECT_CFLAGS=${PROTECT_CFLAGS-"-fstack-protector"}
SAVE_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $PROTECT_CFLAGS"
AC_MSG_CHECKING(whether protection cflags work)
AC_COMPILE_IFELSE(int i;,
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
    CFLAGS=$SAVE_CFLAGS])


AC_CHECK_LIB(ssp, __stack_chk_guard)
AC_CHECK_LIB(ncursesw, initscr,, AC_MSG_ERROR(No libncursesw found.))
AC_CHECK_LIB(com_err, com_err)
AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(socket, socket)
dnl AC_CHECK_LIB(des425, req_act_vno)
AC_CHECK_LIB(des425, des_cbc_encrypt,,AC_CHECK_LIB(crypto,DES_cbc_encrypt))
AC_CHECK_LIB(resolv, res_search)
dnl AC_CHECK_LIB(zephyr, ZGetSender,, AC_MSG_ERROR(No zephyr library found.))
AC_CHECK_LIB(zephyr, ZGetSender)
AC_CHECK_LIB(zephyr, ZInitLocationInfo, AC_DEFINE([HAVE_LIBZEPHYR_ZINITLOCATIONINFO], [], [Have ZInitLocationInfo]),)

AC_ARG_ENABLE([athstatic], AS_HELP_STRING([--enable-athstatic],
                                          [Statically link libraries in /usr/athena/lib]),
                                          [ATHSTATIC=./athstatic],
                                          [ATHSTATIC=])

AC_CHECK_FUNCS(use_default_colors resizeterm des_string_to_key des_key_sched des_ecb_encrypt)
AC_CHECK_FUNCS(				   DES_string_to_key  DES_ecb_encrypt DES_key_sched)

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
AC_MSG_NOTICE([Adding perl CFLAGS ${FOO}])
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
AC_MSG_NOTICE([Adding perl LDFLAGS ${FOO}])
LDFLAGS=${LDFLAGS}\ ${FOO}

dnl Add CFLAGS and LDFLAGS for glib-2.0
PKG_CHECK_MODULES(GLIB,glib-2.0)

AC_MSG_NOTICE([Adding glib-2.0 CFLAGS ${GLIB_CFLAGS}])
CFLAGS="${GLIB_CFLAGS} ${CFLAGS}"
AC_MSG_NOTICE([Adding glib-2.0 LDFLAGS ${GLIB_LIBS}])
LDFLAGS="${GLIB_LIBS} ${LDFLAGS}"

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Support --program-{prefix,suffix,transform}
AC_ARG_PROGRAM

AC_SUBST(XSUBPPDIR)
AC_SUBST(XSUBPPFLAGS)
AC_SUBST(ATHSTATIC)

AC_PROG_INSTALL

AC_CONFIG_SUBDIRS(libfaim)

AC_OUTPUT(Makefile)
