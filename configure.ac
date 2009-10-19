dnl Process this file with autoconf to produce a configure script.
AC_INIT([BarnOwl],[1.4],[bug-barnowl@mit.edu])
AM_INIT_AUTOMAKE([-Wall foreign])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

AC_PROG_CC

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
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([res_search], [resolv])

AC_ARG_WITH([zephyr],
  [AS_HELP_STRING([--with-libzephyr],
    [Enable Zephyr support])],
  [],
  [with_zephyr=check])

AS_IF([test "x$with_zephyr" != xno],
  [AC_MSG_CHECKING([for Kerberos IV])
   AS_IF([krb5-config krb4 --libs >/dev/null 2>&1],
     [AC_MSG_RESULT([yes])
      AC_DEFINE([HAVE_KERBEROS_IV], [1], [Define if you have kerberos IV])
      CFLAGS="${CFLAGS} `krb5-config krb4 --cflags`"
      LIBS="${LIBS} `krb5-config krb4 --libs`"
     ],
     [AC_MSG_RESULT([no])
      PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto])
      CFLAGS="${CFLAGS} ${LIBCRYPTO_CFLAGS}"
      LIBS="${LIBS} ${LIBCRYPTO_LIBS}"
     ])
   AC_CHECK_LIB([zephyr], [ZGetSender],
   [LIBS="$LIBS -lzephyr"
    AC_DEFINE([HAVE_LIBZEPHYR], [1],
                [Define if you have libzephyr])
    AC_CHECK_LIB([zephyr], [ZInitLocationInfo],
      AC_DEFINE([HAVE_LIBZEPHYR_ZINITLOCATIONINFO], [1],
                [Have ZInitLocationInfo]),)
    AC_CHECK_LIB([com_err], [com_err])
    AC_CHECK_HEADERS([com_err.h])
   ],
   [if test "x$with_zephyr" != xcheck; then
      AC_MSG_FAILURE(
        [--with-libzephyr was given, but libzephyr does not seem to be available.])
    fi
   ])])

AC_CHECK_FUNCS([use_default_colors resizeterm])
AC_CHECK_FUNCS([des_string_to_key des_key_sched des_ecb_encrypt])
AC_CHECK_FUNCS([DES_string_to_key DES_ecb_encrypt DES_key_sched])

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(strings.h sys/ioctl.h sys/filio.h unistd.h)

dnl Add CFLAGS for embeded perl
PERL_CFLAGS=`perl -MExtUtils::Embed -e ccopts`
AC_MSG_NOTICE([Adding perl CFLAGS ${PERL_CFLAGS}])
CFLAGS="${CFLAGS} ${PERL_CFLAGS}"

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
AX_CFLAGS_STRICT_PROTOTYPES

AX_C_CHECK_FLAG([-Wno-pointer-sign],[],[],
  [LIBFAIM_CFLAGS="$LIBFAIM_CFLAGS -Wno-pointer-sign"])

AC_SUBST([LIBFAIM_CFLAGS])

AC_SUBST(XSUBPPDIR)
AC_SUBST(XSUBPPFLAGS)

AC_PROG_INSTALL
AC_PROG_RANLIB

AC_CONFIG_FILES([Makefile libfaim/Makefile perl/modules/Makefile])
AC_OUTPUT
