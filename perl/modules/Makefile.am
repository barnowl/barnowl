MODULES = Jabber IRC WordWrap Twitter Facebook

EXTRA_DIST = $(MODULES:=/Makefile.PL) $(MODULES:=/inc) $(MODULES:=/lib)
EXTRA_DIST += \
    Facebook/README \
    Twitter/README \
    Twitter/twitter.example

all-local: $(MODULES)
$(MODULES): %: %/Makefile
	(cd $* && $(MAKE) $*.par)

MODULES_MAKEFILE = $(MODULES:=/Makefile)
$(MODULES_MAKEFILE): %/Makefile: %/Makefile.PL
	[ $(srcdir) = . ] || cp -srf --no-preserve=mode -- $(abs_srcdir)/$*/. $*
	$(AM_V_GEN)(cd $* && perl -I../../lib Makefile.PL)

MODULES_CLEAN = $(MODULES:%=clean_%)
clean-local: $(MODULES_CLEAN)
$(MODULES_CLEAN): clean_%:
	if [ $(srcdir) = . ]; then \
	    (cd $* && { test ! -f Makefile || $(MAKE) realclean; }) && \
	    rm -f $*/$*.par; \
	else \
	    rm -rf $*; \
	fi

MODULES_INSTALL = $(MODULES:%=module_install_%)
install-data-local: $(MODULES_INSTALL)
installdirs-local:
	$(mkinstalldirs) ${DESTDIR}${pkgdatadir}/modules/
$(MODULES_INSTALL): module_install_%: % installdirs-local
	${INSTALL_DATA} $*/$*.par ${DESTDIR}${pkgdatadir}/modules/$*.par

uninstall-local:
	rm -f $(MODULES:%=${DESTDIR}${pkgdatadir}/modules/%.par)

.PHONY: $(MODULES) $(MODULES_CLEAN) $(MODULES_INSTALL)
