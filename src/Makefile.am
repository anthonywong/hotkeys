bin_PROGRAMS    = hotkeys
hotkeys_SOURCES = hotkeys.c kbddef.c conf.c fixVMware.c apmlib.c xmalloc.c \
                  splash.c \
                  XF86keysym.h apm.h common.h conf.h hotkeys.h kbddef.h \
                  xmalloc.h splash.h
sysconf_DATA    = hotkeys.conf
man_MANS        = hotkeys.1

CFLAGS      = @CFLAGS@ @X_CFLAGS@ @XML_CFLAGS@ @LIBXOSD_CFLAGS@ @GTK_CFLAGS@ -I.. # -DDEBUG 
LIBS        = @LIBS@ @X_LIBS@ @XML_LIBS@ @LIBXOSD_LIBS@ @GTK_LIBS@
DEFS        = @DEFS@ -DSHAREDIR=\"@datadir@/@PACKAGE@\" \
              -DCONFDIR=\"@sysconfdir@\" -DCONFIG_NAME=\"@PACKAGE@.conf\"

pkgdata_DATA = splash.png
$(pkgdata_DATA):

##  uncomment the following if hotkeys requires the math library
##hotkeys_LDADD=-lm

##EXTRA_DIST=hotkeys.lsm.in hotkeys.spec.in hotkeys.texinfo
EXTRA_DIST  = $(man_MANS) $(sysconf_DATA) $(pkgdata_DATA)

##  if you write a self-test script named `chk', uncomment the
##  following and add `chk' to the EXTRA_DIST list
##TESTS=chk

##  build and install the .info pages
##info_TEXINFOS = hotkeys.texinfo
##hotkeys_TEXINFOS = gpl.texinfo
