bin_PROGRAMS = barnowl.bin
check_PROGRAMS = tester perl_tester

barnowl_bin_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h \
     owl.c \
     libzcrypt.a \
     $(GEN_C) $(GEN_H)

barnowl_bin_LDADD = libfaim/libfaim.a libzcrypt.a

tester_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h \
     libzcrypt.a \
     $(GEN_C) $(GEN_H) \
     tester.c

tester_LDADD = libfaim/libfaim.a libzcrypt.a

perl_tester_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h \
     libzcrypt.a \
     $(GEN_C) $(GEN_H) \
     perl_tester.c

perl_tester_LDADD = libfaim/libfaim.a libzcrypt.a

TESTS=runtests.sh

noinst_LIBRARIES = libzcrypt.a

libzcrypt_a_SOURCES = zcrypt.c
libzcrypt_a_CFLAGS  = -w

CPPFLAGS = -I$(top_srcdir)/ \
           -I$(top_srcdir)/libfaim/

BASE_SRCS=list.c message.c mainwin.c popwin.c zephyr.c messagelist.c \
     commands.c global.c text.c fmtext.c editwin.c util.c logging.c \
     perlconfig.c keys.c functions.c zwrite.c viewwin.c help.c filter.c \
     regex.c history.c view.c dict.c variable.c filterelement.c pair.c \
     keypress.c keymap.c keybinding.c cmd.c context.c \
     aim.c buddy.c buddylist.c style.c errqueue.c \
     zbuddylist.c popexec.c obarray.c select.c wcwidth.c \
     glib_compat.c

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

perlglue.c: perlglue.xs
	$(AM_V_GEN)perl $(XSUBPPDIR)/xsubpp $(XSUBPPFLAGS) -prototypes perlglue.xs > perlglue.c

varstubs.c: stubgen.pl variable.c
	$(AM_V_GEN)perl $< $(filter-out $<,$+) > $@

owl_prototypes.h.new: codelist.pl varstubs.c $(BASE_SRCS) zcrypt.c
	$(AM_V_GEN)perl $< $(filter-out $<,$+) > $@

# For emacs flymake-mode
check-syntax: proto
	$(CC) $(CFLAGS) -Wall -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

install-data-local:
	$(mkinstalldirs) ${DESTDIR}${pkgdatadir}/lib
	(cd perl/lib && tar -cf - . ) | (cd ${DESTDIR}${pkgdatadir}/lib && tar -xf - )

do_transform = $(shell echo '$(1)' | sed '$(transform)')
install-exec-hook:
	mv -f $(DESTDIR)$(bindir)/$(call do_transform,barnowl.bin) \
	      $(DESTDIR)$(bindir)/$(call do_transform,barnowl)

SUBDIRS = libfaim/ perl/modules/
