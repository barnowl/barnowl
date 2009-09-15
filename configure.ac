dnl Process this file with autoconf to produce a configure script.
AC_INIT([BarnOwl],[1.4pre1],[bug-barnowl@mit.edu])
AM_INIT_AUTOMAKE([-Wall foreign])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_HEADER([config.h])

AC_PROG_CC

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

AC_ARG_WITH([stack-protector],
  [AS_HELP_STRING([--with-stack-protector],
    [Enable gcc -fstack-protector])],
  [],
  [with_stack_protector=check])

AS_IF([test "x$with_stack_protector" != xno],
  [AX_C_CHECK_FLAG([-fstack-protector],[],[],
    [CFLAGS="$CFLAGS -fstack-protector"],
    [if test "x$with_stack_protector" != xcheck; then
       AC_MSG_FAILURE([--with-stack-protector selected, but gcc does support it.])
     fi
    ])])

AC_CHECK_LIB(ncursesw, initscr,, AC_MSG_ERROR(No libncursesw found.))
AC_CHECK_LIB(com_err, com_err)
AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(socket, socket)
dnl AC_CHECK_LIB(des425, req_act_vno)
AC_CHECK_LIB(des425, des_cbc_encrypt,,AC_CHECK_LIB(crypto,DES_cbc_encrypt))
AC_CHECK_LIB(resolv, res_search)

AC_ARG_WITH([zephyr],
  [AS_HELP_STRING([--with-libzephyr],
    [Enable Zephyr support])],
  [],
  [with_zephyr=check])

AS_IF([test "x$with_zephyr" != xno],
  [AC_CHECK_LIB([zephyr], [ZGetSender],
   [LIBS="$LIBS -lzephyr"
    AC_DEFINE([HAVE_LIBZEPHYR], [1],
                [Define if you have libzephyr])
    AC_CHECK_LIB([zephyr], [ZInitLocationInfo],
      AC_DEFINE([HAVE_LIBZEPHYR_ZINITLOCATIONINFO], [1],
                [Have ZInitLocationInfo]),)
   ],
   [if test "x$with_zephyr" != xcheck; then
      AC_MSG_FAILURE(
        [--with-libzephyr was given, but libzephyr does not seem to be available.])
    fi
   ])])

AC_CHECK_FUNCS(use_default_colors resizeterm des_string_to_key des_key_sched des_ecb_encrypt)
AC_CHECK_FUNCS(				   DES_string_to_key  DES_ecb_encrypt DES_key_sched)

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
AC_MSG_NOTICE([Adding perl LIBS ${FOO}])
LIBS=${LIBS}\ ${FOO}

dnl Add CFLAGS and LDFLAGS for glib-2.0
PKG_CHECK_MODULES(GLIB,glib-2.0)

AC_MSG_NOTICE([Adding glib-2.0 CFLAGS ${GLIB_CFLAGS}])
CFLAGS="${GLIB_CFLAGS} ${CFLAGS}"
AC_MSG_NOTICE([Adding glib-2.0 LDFLAGS ${GLIB_LIBS}])
LDFLAGS="${GLIB_LIBS} ${LDFLAGS}"

if test "x${prefix}" = "xNONE"; then
   prefix="${ac_default_prefix}"
fi

AC_DEFINE_UNQUOTED([DATADIR],["${prefix}/share/${PACKAGE}"],
                   [Package data directory])

dnl Checks for typedefs, structures, and compiler characteristics.

AX_CFLAGS_WARN_ALL

AX_C_CHECK_FLAG([-Wno-pointer-sign],[],[],
  [LIBFAIM_CFLAGS="$LIBFAIM_CFLAGS -Wno-pointer-sign"])

AC_SUBST([LIBFAIM_CFLAGS])

AC_SUBST(XSUBPPDIR)
AC_SUBST(XSUBPPFLAGS)

AC_PROG_INSTALL
AC_PROG_RANLIB

AC_CONFIG_FILES([Makefile libfaim/Makefile perl/modules/Makefile])
AC_OUTPUT
