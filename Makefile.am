SUBDIRS = src def
EXTRA_DIST  = BUGS debian/* hotkeys.spec config/install-sh config/missing config/mkinstalldirs

debian-dist: dist
	fakeroot debian/rules clean
	mv @PACKAGE@-@VERSION@.tar.gz ../@PACKAGE@_@VERSION@.orig.tar.gz
