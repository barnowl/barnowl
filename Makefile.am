bin_PROGRAMS = barnowl.bin

barnowl_bin_SOURCES = $(BASE_SRCS) \
     owl.h owl_perl.h config.h test.h \
     owl.c \
     libzcrypt.a \
     $(GEN_C) $(GEN_H)

barnowl_bin_LDADD = libfaim/libfaim.a libzcrypt.a

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

proto: owl_prototypes.h

perlglue.c: perlglue.xs
	perl $(XSUBPPDIR)/xsubpp $(XSUBPPFLAGS) -prototypes perlglue.xs > perlglue.c

varstubs.c: variable.c stubgen.pl
	perl stubgen.pl > varstubs.c

# Only move owl_prototypes.h into place if the new one is different
owl_prototypes.h: codelist.pl varstubs.c $(BASE_SRCS)
	perl codelist.pl > owl_prototypes.h.new
	@cmp -s owl_prototypes.h.new $@ || { \
	 test -f $@ && echo 'Interfaces changed!'; \
	 echo mv -f owl_prototypes.h.new $@; \
	      mv -f owl_prototypes.h.new $@; }

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
