ACLOCAL_AMFLAGS = -I m4

GIT_DESCRIPTION := $(if $(wildcard .git),$(shell git describe --match='barnowl-*' HEAD 2>/dev/null))
GIT_FLAGS := $(if $(GIT_DESCRIPTION),-DGIT_VERSION=$(GIT_DESCRIPTION:barnowl-%=%))

bin_PROGRAMS = barnowl.bin
if ENABLE_ZCRYPT
bin_PROGRAMS += zcrypt
endif

zcrypt_SOURCES = zcrypt.c filterproc.c

check_PROGRAMS = tester.bin
dist_check_DATA = t
dist_check_SCRIPTS = runtests.sh

bin_SCRIPTS = barnowl
check_SCRIPTS = tester

barnowl tester: %: barnowl-wrapper.in %.bin
	cp $< $@
	chmod +x $@

barnowl_bin_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h \
     owl.c \
     $(GEN_C) $(GEN_H)

dist_man_MANS = doc/barnowl.1
dist_doc_DATA = doc/intro.txt doc/advanced.txt

barnowl_bin_LDADD = compat/libcompat.a libfaim/libfaim.a

tester_bin_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h \
     $(GEN_C) $(GEN_H) \
     tester.c

tester_bin_LDADD = compat/libcompat.a libfaim/libfaim.a

TESTS=runtests.sh

AM_CPPFLAGS = \
           -I$(top_srcdir)/libfaim/ \
           -DDATADIR='"$(pkgdatadir)"' \
           -DBINDIR='"$(bindir)"' \
           $(GIT_FLAGS)

CODELIST_SRCS=message.c mainwin.c popwin.c zephyr.c messagelist.c \
     commands.c global.c text.c fmtext.c editwin.c \
     util.c logging.c \
     perlconfig.c keys.c functions.c zwrite.c viewwin.c help.c filter.c \
     regex.c history.c view.c dict.c variable.c filterelement.c pair.c \
     keypress.c keymap.c keybinding.c cmd.c context.c \
     aim.c buddy.c buddylist.c style.c errqueue.c \
     zbuddylist.c popexec.c select.c wcwidth.c \
     mainpanel.c msgwin.c sepbar.c editcontext.c signal.c closures.c

NORMAL_SRCS = filterproc.c filterproc.h window.c window.h windowcb.c

BASE_SRCS = $(CODELIST_SRCS) $(NORMAL_SRCS)

GEN_C = varstubs.c perlglue.c gmarshal_funcs.c
GEN_H = owl_prototypes.h owl_prototypes.h.new gmarshal_funcs.h

BUILT_SOURCES = $(GEN_C) $(GEN_H)

# Only copy file into place if file.new is different
%: %.new
	@diff -U0 $@ $< || { \
	 test -f $@ && echo '$@ changed!'; \
	 echo cp -f $< $@; \
	      cp -f $< $@; }

proto: owl_prototypes.h

perlglue.c: perlglue.xs typemap
	$(AM_V_GEN)perl $(XSUBPPDIR)/xsubpp $(XSUBPPFLAGS) -prototypes $< > $@

varstubs.c: stubgen.pl variable.c
	$(AM_V_GEN)perl $< $(sort $(filter-out $<,$+)) > $@

owl_prototypes.h.new: codelist.pl varstubs.c $(CODELIST_SRCS)
	$(AM_V_GEN)perl $< $(sort $(filter-out $<,$+)) > $@

gmarshal_funcs.h: marshal_types
	glib-genmarshal --header $< > $@
gmarshal_funcs.c: marshal_types
	glib-genmarshal --body $< > $@

# For emacs flymake-mode
check-syntax: proto
	$(COMPILE) -Wall -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

do_transform = $(shell echo '$(1)' | sed '$(transform)')
install-exec-hook:
	mv -f $(DESTDIR)$(bindir)/$(call do_transform,barnowl.bin) \
	      $(DESTDIR)$(bindir)/$(call do_transform,barnowl)

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/$(call do_transform,barnowl)

CLEANFILES = $(BUILT_SOURCES) $(bin_SCRIPTS) $(check_SCRIPTS)
EXTRA_DIST = \
    autogen.sh \
    barnowl-wrapper.in \
    codelist.pl \
    doc \
    examples \
    marshal_types \
    perlglue.xs \
    scripts \
    stubgen.pl \
    typemap

SUBDIRS = compat libfaim perl
