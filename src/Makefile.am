bin_PROGRAMS    = hotkeys
hotkeys_SOURCES = hotkeys.c kbddef.c conf.c apmlib.c xmalloc.c

CFLAGS      = @CFLAGS@ @XML_CFLAGS@ @X_CFLAGS@ -I.. -DDEBUG 
LIBS        = @LIBS@ @XML_LIBS@ @X_LIBS@
DEFS        = @DEFS@ -DSHAREDIR=\"@datadir@/@PACKAGE@\" \
              -DCONFDIR=\"@sysconfdir@\" -DCONFIG_NAME=\"@PACKAGE@.conf\"

##  uncomment the following if hotkeys requires the math library
##hotkeys_LDADD=-lm

##EXTRA_DIST=hotkeys.lsm.in hotkeys.spec.in hotkeys.texinfo
EXTRA_DIST  = $(man_MANS) 

##  if you write a self-test script named `chk', uncomment the
##  following and add `chk' to the EXTRA_DIST list
##TESTS=chk

##  build and install the .info pages
##info_TEXINFOS = hotkeys.texinfo
##hotkeys_TEXINFOS = gpl.texinfo

##  install the man pages
man_MANS       = hotkeys.1
