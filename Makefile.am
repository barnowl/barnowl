ACLOCAL_AMFLAGS = -I m4

GIT_DESCRIPTION := $(if $(wildcard .git),$(shell git describe --match='barnowl-*' HEAD 2>/dev/null))
GIT_FLAGS := $(if $(GIT_DESCRIPTION),-DGIT_VERSION=$(GIT_DESCRIPTION:barnowl-%=%))

bin_PROGRAMS = barnowl.bin
if ENABLE_ZCRYPT
bin_PROGRAMS += zcrypt
endif

zcrypt_SOURCES = zcrypt.c filterproc.c

check_PROGRAMS = tester.bin

barnowl_bin_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h \
     owl.c \
     $(GEN_C) $(GEN_H)

man_MANS = doc/barnowl.1
doc_DATA = doc/intro.txt doc/advanced.txt

barnowl_bin_LDADD = compat/libcompat.a libfaim/libfaim.a

tester_bin_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h \
     $(GEN_C) $(GEN_H) \
     tester.c

tester_bin_LDADD = compat/libcompat.a libfaim/libfaim.a

TESTS=runtests.sh

AM_CPPFLAGS = -I$(top_srcdir)/ \
           -I$(top_srcdir)/libfaim/ \
           -DDATADIR='"$(pkgdatadir)"' \
           -DBINDIR='"$(bindir)"' \
           $(GIT_FLAGS)

CODELIST_SRCS=list.c message.c mainwin.c popwin.c zephyr.c messagelist.c \
     commands.c global.c text.c fmtext.c editwin.c util.c logging.c \
     perlconfig.c keys.c functions.c zwrite.c viewwin.c help.c filter.c \
     regex.c history.c view.c dict.c variable.c filterelement.c pair.c \
     keypress.c keymap.c keybinding.c cmd.c context.c \
     aim.c buddy.c buddylist.c style.c errqueue.c \
     zbuddylist.c popexec.c select.c wcwidth.c \
     glib_compat.c mainpanel.c msgwin.c sepbar.c editcontext.c signal.c

NORMAL_SRCS = filterproc.c window.c windowcb.c

BASE_SRCS = $(CODELIST_SRCS) $(NORMAL_SRCS)

GEN_C = varstubs.c perlglue.c
GEN_H = owl_prototypes.h

BUILT_SOURCES = $(GEN_C) $(GEN_H)

# Only copy file into place if file.new is different
%: %.new
	@diff -U0 $@ $< || { \
	 test -f $@ && echo '$@ changed!'; \
	 echo cp -f $< $@; \
	      cp -f $< $@; }

proto: owl_prototypes.h

perlglue.c: perlglue.xs $(TYPEMAP)
	$(AM_V_GEN)perl $(XSUBPPDIR)/xsubpp $(XSUBPPFLAGS) -prototypes perlglue.xs > perlglue.c

varstubs.c: stubgen.pl variable.c
	$(AM_V_GEN)perl $< $(sort $(filter-out $<,$+)) > $@

owl_prototypes.h.new: codelist.pl varstubs.c $(CODELIST_SRCS)
	$(AM_V_GEN)perl $< $(sort $(filter-out $<,$+)) > $@

# For emacs flymake-mode
check-syntax: proto
	$(COMPILE) -Wall -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

install-data-local:
	$(mkinstalldirs) ${DESTDIR}${pkgdatadir}/lib
	(cd perl/lib && tar -cf - . ) | (cd ${DESTDIR}${pkgdatadir}/lib && tar -xf - )

do_transform = $(shell echo '$(1)' | sed '$(transform)')
install-exec-hook:
	mv -f $(DESTDIR)$(bindir)/$(call do_transform,barnowl.bin) \
	      $(DESTDIR)$(bindir)/$(call do_transform,barnowl)

SUBDIRS = compat libfaim perl
