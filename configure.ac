dnl Process this file with autoconf to produce a configure script.
AC_INIT([BarnOwl],[1.11dev],[bug-barnowl@mit.edu])
AM_INIT_AUTOMAKE([1.7.0 foreign std-options -Wall -Wno-portability])
AM_MAINTAINER_MODE([enable])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_LN_S

AC_ARG_WITH([zephyr-default-format],
    [AS_HELP_STRING([--with-zephyr-default-format],
                    [value for the default format zephyrgram field])],
    [
        case $withval in
            yes) withval='Config error: see http://mit.edu/df';;
            no) withval='';;
        esac
        zephyr_default_format=$withval
    ],
    [zephyr_default_format='Config error: see http://mit.edu/df'])
AC_DEFINE_UNQUOTED(
    [ZEPHYR_DEFAULT_FORMAT], ["$zephyr_default_format"],
    [Value for the default format zephyrgram field]
)

AC_ARG_WITH([stack-protector],
  [AS_HELP_STRING([--with-stack-protector],
    [Enable gcc -fstack-protector])],
  [],
  [with_stack_protector=check])

AS_IF([test "x$with_stack_protector" != xno],
  [AX_CHECK_COMPILE_FLAG([-fstack-protector],
    [AM_CFLAGS="$AM_CFLAGS -fstack-protector"],
    [if test "x$with_stack_protector" != xcheck; then
       AC_MSG_FAILURE([--with-stack-protector selected, but gcc does support it.])
     fi
    ])])

AC_CHECK_LIB(ncursesw, initscr,, AC_MSG_ERROR(No libncursesw found.))
AC_CHECK_LIB(panelw, update_panels,, AC_MSG_ERROR(No libpanelw found.))
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([res_search], [resolv])

AC_ARG_WITH([zephyr],
  [AS_HELP_STRING([--with-zephyr],
    [Enable Zephyr support])],
  [],
  [with_zephyr=check])

AC_ARG_WITH([krb4],
  AS_HELP_STRING([--with-krb4],
                 [Build with kerberos IV]))

AS_IF([test "x$with_zephyr" != xno],
  [have_krb4=no

   AS_IF([test "x$with_krb4" != "xno"],
   [AC_MSG_CHECKING([for Kerberos IV])
    AS_IF([krb5-config krb4 --libs >/dev/null 2>&1],
      [AC_MSG_RESULT([yes])
       have_krb4=yes
       AC_DEFINE([HAVE_KERBEROS_IV], [1], [Define if you have kerberos IV])
       AM_CFLAGS="${AM_CFLAGS} `krb5-config krb4 --cflags`"
       LIBS="${LIBS} `krb5-config krb4 --libs`"
      ],
      [AC_MSG_RESULT([no])
       AS_IF([test "x$with_krb4" = "xyes"],
             [AC_MSG_ERROR([Kerberos IV requested but not found])])])])

   AS_IF([test "x$have_krb4" != xyes],
     [PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto],
        [AM_CFLAGS="${AM_CFLAGS} ${LIBCRYPTO_CFLAGS}"
         LIBS="${LIBS} ${LIBCRYPTO_LIBS}"
        ],
        [PKG_CHECK_MODULES([OPENSSL], [openssl],
           [AM_CFLAGS="${AM_CFLAGS} ${OPENSSL_CFLAGS}"
            LIBS="${LIBS} ${OPENSSL_LIBS}"
           ])])])

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
        [--with-zephyr was given, but libzephyr does not seem to be available.])
    fi
   ])])

AC_CHECK_FUNCS([use_default_colors])
AC_CHECK_FUNCS([resizeterm], [], [AC_MSG_ERROR([No resizeterm found])])
AC_CHECK_FUNCS([des_string_to_key DES_string_to_key], [HAVE_DES_STRING_TO_KEY=1])
AC_CHECK_FUNCS([des_ecb_encrypt DES_ecb_encrypt], [HAVE_DES_ECB_ENCRYPT=1])
AC_CHECK_FUNCS([des_key_sched DES_key_sched], [HAVE_DES_KEY_SCHED=1])

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(stdbool.h strings.h sys/ioctl.h sys/filio.h unistd.h)

dnl Add CFLAGS for embeded perl
PERL_CFLAGS=`perl -MExtUtils::Embed -e ccopts`
AC_MSG_NOTICE([Adding perl CFLAGS ${PERL_CFLAGS}])
AM_CFLAGS="${AM_CFLAGS} ${PERL_CFLAGS}"

dnl Find the location of perl XSUBPP
AC_MSG_CHECKING(for the perl xsubpp precompiler)
XSUBPPDIR="`cd "$srcdir" && perl -MExtUtils::MakeMaker -e 'print ExtUtils::MakeMaker->new({NAME => qw(owl)})->tool_xsubpp;' | grep \^XSUBPPDIR | sed -e 's/XSUBPPDIR = //g;'`"
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

dnl Add LIBS for embedded perl
FOO=`perl -MExtUtils::Embed -e ldopts`
AC_MSG_NOTICE([Adding perl LIBS ${FOO}])
LIBS=${LIBS}\ ${FOO}
AC_CHECK_LIB([perl], [perl_alloc],, AC_MSG_ERROR([No libperl found]))

AX_PROG_PERL_MODULES([AnyEvent],,
                     [AC_MSG_ERROR([cannot find perl module AnyEvent.])])
AX_PROG_PERL_MODULES([Class::Accessor::Fast],,
                     [AC_MSG_ERROR([cannot find perl module Class::Accessor::Fast.])])
AX_PROG_PERL_MODULES([ExtUtils::Depends],,
                     [AC_MSG_ERROR([cannot find perl module ExtUtils::Depends])])
AX_PROG_PERL_MODULES([Glib],,
                     [AC_MSG_ERROR([cannot find perl module Glib.])])
AX_PROG_PERL_MODULES([Module::Install::Base],,
                     [AC_MSG_ERROR([cannot find perl module Module::Install::Base])])
AX_PROG_PERL_MODULES([PAR],,
                     [AC_MSG_WARN([PAR.pm not found. Loadable modules will be disabled.])])

dnl Add CFLAGS and LIBS for glib-2.0
PKG_CHECK_MODULES(GLIB,[glib-2.0 >= 2.16 gobject-2.0 gthread-2.0])

AC_MSG_NOTICE([Adding glib-2.0 CFLAGS ${GLIB_CFLAGS}])
AM_CFLAGS="${GLIB_CFLAGS} ${AM_CFLAGS}"
AC_MSG_NOTICE([Adding glib-2.0 LIBS ${GLIB_LIBS}])
LIBS="${GLIB_LIBS} ${LIBS}"

if test "x${prefix}" = "xNONE"; then
   prefix="${ac_default_prefix}"
fi

dnl Add CFLAGS for glib-perl
GLIB_PERL_CFLAGS=`perl -MExtUtils::Depends -e 'my $e = ExtUtils::Depends->new("BarnOwl","Glib"); my %h = $e->get_makefile_vars; print $h{"INC"}'`
AC_MSG_NOTICE([Adding glib-perl CFLAGS ${GLIB_PERL_CFLAGS}])
AM_CFLAGS="${GLIB_PERL_CFLAGS} ${AM_CFLAGS}"

dnl Checks for typedefs, structures, and compiler characteristics.

AX_CFLAGS_WARN_ALL([AM_CFLAGS])
AX_APPEND_COMPILE_FLAGS([-Wstrict-prototypes -Wwrite-strings],[AM_CFLAGS])

dnl Shut gcc up about zero-length format strings; the warning's apparently for
dnl efficiency reasons, which is bogus for custom functions.
AX_APPEND_COMPILE_FLAGS([-Wno-format-zero-length],[AM_CFLAGS])

AM_CONDITIONAL([ENABLE_ZCRYPT], [test "$HAVE_DES_STRING_TO_KEY" && dnl
                                 test "$HAVE_DES_KEY_SCHED" && dnl
                                 test "$HAVE_DES_ECB_ENCRYPT"])

AM_CFLAGS="$AM_CFLAGS -D_XOPEN_SOURCE=600"
dnl Define _BSD_SOURCE because zephyr needs caddr_t.
AM_CFLAGS="$AM_CFLAGS -D_BSD_SOURCE"
dnl Define __EXTENSIONS__ for strcasecmp on Solaris.
AM_CFLAGS="$AM_CFLAGS -D__EXTENSIONS__"
dnl Define _XOPEN_SOURCE_EXTENDED for some features in ncurses,
dnl including cchar_t.  This should not be necessary with
dnl _XOPEN_SOURCE=600, but some versions of ncurses
dnl apparently still need it.
AM_CFLAGS="$AM_CFLAGS -D_XOPEN_SOURCE_EXTENDED"

AC_SUBST([AM_CFLAGS])

AC_SUBST(XSUBPPDIR)
AC_SUBST(XSUBPPFLAGS)

AC_PROG_INSTALL
AC_PROG_RANLIB
AC_CHECK_PROG([HAVE_ZIP], [zip], [yes], [no])
if test "x${HAVE_ZIP}" = "xno"; then
   AC_MSG_ERROR([cannot find a 'zip' binary.])
fi

AC_CONFIG_LIBOBJ_DIR([compat])
AC_CHECK_DECLS([memrchr])
AC_REPLACE_FUNCS([memrchr])

AC_SUBST([abs_builddir])
AC_SUBST([abs_srcdir])

AC_CONFIG_FILES([Makefile compat/Makefile perl/Makefile perl/modules/Makefile])
AC_OUTPUT
