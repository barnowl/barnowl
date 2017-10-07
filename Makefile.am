ACLOCAL_AMFLAGS = -I m4

GIT_DESCRIPTION := $(if $(wildcard $(srcdir)/.git),$(shell cd $(srcdir) && git describe --match='barnowl-*' HEAD 2>/dev/null))
VERSION = $(if $(GIT_DESCRIPTION),$(GIT_DESCRIPTION:barnowl-%=%),$(DIST_VERSION))
DIST_VERSION = @VERSION@
-include $(srcdir)/DIST_VERSION.mk BUILD_VERSION.mk

FORCE:
BUILD_VERSION.mk: $(if $(filter-out $(BUILD_VERSION),$(VERSION)),FORCE)
	echo 'BUILD_VERSION = $(VERSION)' > $@
dist-hook:: $(if $(filter-out @VERSION@,$(VERSION)),dist-hook-version)
dist-hook-version:
	echo 'DIST_VERSION = $(VERSION)' > $(distdir)/DIST_VERSION.mk

bin_PROGRAMS = bin/barnowl
if ENABLE_ZCRYPT
bin_PROGRAMS += zcrypt
endif

zcrypt_SOURCES = zcrypt.c filterproc.c
nodist_zcrypt_SOURCES = version.c

check_PROGRAMS = bin/tester
dist_check_DATA = t
dist_check_SCRIPTS = runtests.sh

noinst_SCRIPTS = barnowl
check_SCRIPTS = tester

barnowl tester: %: barnowl-wrapper.in bin/% Makefile
	sed \
	    -e 's,[@]abs_srcdir[@],$(abs_srcdir),g' \
	    -e 's,[@]abs_builddir[@],$(abs_builddir),g' \
	    $< > $@
	chmod +x $@

bin_barnowl_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h \
     owl.c
nodist_bin_barnowl_SOURCES = $(GEN_C) $(GEN_H)

dist_man_MANS = doc/barnowl.1
dist_doc_DATA = doc/intro.txt doc/advanced.txt

bin_barnowl_LDADD = compat/libcompat.a

bin_tester_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h \
     tester.c
nodist_bin_tester_SOURCES = $(GEN_C) $(GEN_H)

bin_tester_LDADD = compat/libcompat.a

TESTS=runtests.sh

AM_CPPFLAGS = \
           -DDATADIR='"$(pkgdatadir)"' \
           -DBINDIR='"$(bindir)"'

CODELIST_SRCS=message.c mainwin.c popwin.c zephyr.c messagelist.c \
     commands.c global.c text.c fmtext.c editwin.c \
     util.c logging.c \
     perlconfig.c keys.c functions.c zwrite.c viewwin.c help.c filter.c \
     regex.c history.c view.c dict.c variable.c filterelement.c pair.c \
     keypress.c keymap.c keybinding.c cmd.c context.c \
     style.c errqueue.c \
     zbuddylist.c popexec.c select.c wcwidth.c \
     mainpanel.c msgwin.c sepbar.c editcontext.c signal.c closures.c

NORMAL_SRCS = filterproc.c filterproc.h window.c window.h windowcb.c

BASE_SRCS = $(CODELIST_SRCS) $(NORMAL_SRCS)

GEN_C = varstubs.c perlglue.c gmarshal_funcs.c version.c version.c.new
GEN_H = owl_prototypes.h owl_prototypes.h.new gmarshal_funcs.h

BUILT_SOURCES = $(GEN_H)

# Only copy file into place if file.new is different
owl_prototypes.h version.c: %: %.new
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

version.c.new: Makefile BUILD_VERSION.mk
	$(AM_V_GEN)echo 'const char *version = "$(VERSION)";' > $@

gmarshal_funcs.h: marshal_types
	glib-genmarshal --header $< > $@
gmarshal_funcs.c: marshal_types
	glib-genmarshal --body $< > $@

# For emacs flymake-mode
check-syntax: proto
	$(COMPILE) -Wall -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

CLEANFILES = $(BUILT_SOURCES) $(GEN_C) $(noinst_SCRIPTS) $(check_SCRIPTS) BUILD_VERSION.mk
MAINTAINERCLEANFILES = DIST_VERSION.mk
EXTRA_DIST = \
    autogen.sh \
    barnowl-wrapper.in \
    codelist.pl \
    doc/code.txt \
    doc/owl-window.txt \
    doc/releasing-barnowl.txt \
    examples \
    marshal_types \
    perlglue.xs \
    scripts \
    stubgen.pl \
    typemap \
    cpanfile

SUBDIRS = compat perl
