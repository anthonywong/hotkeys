SUBDIRS = src def
EXTRA_DIST  = BUGS debian/* hotkeys.spec

debian-dist: dist
	fakeroot debian/rules clean
	mv @PACKAGE@-@VERSION@.tar.gz ../@PACKAGE@_@VERSION@.orig.tar.gz
	dch -v "@VERSION@-1" "New upstream version"
