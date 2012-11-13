MODULES = Jabber IRC WordWrap Twitter Facebook Kerberos

EXTRA_DIST = $(MODULES:=/Makefile.PL) $(MODULES:=/lib)
EXTRA_DIST += \
    Facebook/README \
    Twitter/README \
    Twitter/twitter.example

all-local: $(MODULES)
$(MODULES): %: %/Makefile
	(cd $* && $(MAKE) $*.par)

BARNOWL_PERL := $(shell which perl) -I$(abs_srcdir)/../lib

MODULES_MAKEFILE = $(MODULES:=/Makefile)
$(MODULES_MAKEFILE): %/Makefile: %/Makefile.PL ../lib/Module/Install/BarnOwl.pm
	$(AM_V_GEN)test -d $* || mkdir $*
	$(AM_V_at)test $(srcdir) = . || $(LN_S) $(abs_srcdir)/$*/Makefile.PL $*/Makefile.PL
	$(AM_V_at)(cd $* && BARNOWL_SRCDIR='$(abs_srcdir)/$*' && export BARNOWL_SRCDIR && $(BARNOWL_PERL) Makefile.PL 'PERL=$(BARNOWL_PERL)')

MODULES_CLEAN = $(MODULES:%=clean_%)
clean-local: $(MODULES_CLEAN)
$(MODULES_CLEAN): clean_%:
	(cd $* && { test ! -f Makefile || $(MAKE) realclean; })
	rm -f $*/$*.par
	rm -rf $*/inc
	test $(srcdir) = . || rm -f $*/Makefile.PL

MODULES_INSTALL = $(MODULES:%=module_install_%)
install-data-local: $(MODULES_INSTALL)
installdirs-local:
	$(mkinstalldirs) ${DESTDIR}${pkgdatadir}/modules/
$(MODULES_INSTALL): module_install_%: % installdirs-local
	${INSTALL_DATA} $*/$*.par ${DESTDIR}${pkgdatadir}/modules/$*.par

uninstall-local:
	rm -f $(MODULES:%=${DESTDIR}${pkgdatadir}/modules/%.par)

.PHONY: $(MODULES) $(MODULES_CLEAN) $(MODULES_INSTALL)
