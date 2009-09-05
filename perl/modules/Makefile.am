MODULES = Jabber IRC WordWrap

EXTRA_DIST = $(MODULES:=/Makefile.PL) $(MODULES:=/inc) $(MODULES:=/lib)

all-local: $(MODULES)
$(MODULES): %: %/Makefile
	(cd $* && $(MAKE) $*.par)

MODULES_MAKEFILE = $(MODULES:=/Makefile)
$(MODULES_MAKEFILE): %/Makefile: %/Makefile.PL
	$(AM_V_GEN)(cd $* && perl -I../../lib Makefile.PL)

MODULES_CLEAN = $(MODULES:%=clean_%)
clean-local: $(MODULES_CLEAN)
$(MODULES_CLEAN): clean_%:
	(cd $* && (test ! -f Makefile || $(MAKE) clean))

MODULES_INSTALL = $(MODULES:%=module_install_%)
install-data-local: $(MODULES_INSTALL)
installdirs-local:
	$(mkinstalldirs) ${DESTDIR}${pkgdatadir}/modules/
$(MODULES_INSTALL): module_install_%: % installdirs-local
	${INSTALL_DATA} $*/$*.par ${DESTDIR}${pkgdatadir}/modules/$*.par

.PHONY: $(MODULES) $(MODULES_CLEAN) $(MODULES_INSTALL)
