/*
    HOTKEYS - use keys on your multimedia keyboard to control your computer
    Copyright (C) 2000,2001  Anthony Y P Wong <ypwong@ypwong.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    $Id$

    Reference:
    http://www.win.tue.nl/math/dw/personalpages/aeb/linux/kbd/scancodes.html
*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif
#include "common.h"

#include <X11/Xosdefs.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern char *getenv();
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif /* HAVE_GETOPT_LONG */
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Mixer related */
#include <fcntl.h>
#include <sys/ioctl.h>
#if defined (__FreeBSD__)
#include <machine/soundcard.h>
#else
#       if defined (__NetBSD__) || defined (__OpenBSD__)
#       include <soundcard.h>          /* OSS emulation */
#       undef ioctl
#       else
/* BSDI, Linux, Solaris */
#       include <sys/soundcard.h>
#       endif                          /* __NetBSD__ or __OpenBSD__ */
#endif                          /* __FreeBSD__ */

/* CDROM related */
#include <linux/cdrom.h>        /* FIXME: linux specific! */
/* APM (suspend/standby) support */
#include "apm.h"

#include "hotkeys.h"


#define	lowbit(x)	((x) & (-(x)))
#define	M(m)	fprintf(stderr,(m))
#define	M1(m,a)	fprintf(stderr,(m),(a))

/***====================================================================***/

/* Corresponding human readable strings to enum _application,
 * all strings should be user configurable (TODO) */
char* app_strings[NUM_APPS] = {
    "Web browser",
    "Email reader",
    "Calculator",
    "Xterm",
    "File manager",
    "app1", "app2", "app3", "app4", "app5"
};

char* applications[NUM_APPS] = {
    BROWSER, MAILER, CALCULATOR, XTERM, FILEMANAGER,
    NULL, NULL, NULL, NULL, NULL
};

char* application_args[NUM_APPS] = {
    BROWSER_ARGS, MAILER_ARGS, CALCULATOR_ARGS, XTERM_ARGS, FILEMANAGER_ARGS,
    NULL, NULL, NULL, NULL, NULL
};

/***====================================================================***/

char *      dpyName           = NULL;
Display *   dpy               = NULL;
char *      cfgFileName       = NULL;
int     	xkbOpcode         = 0;
int     	xkbEventCode      = 0;
XkbDescPtr  xkb               = NULL;

unsigned long   eventMask     = 0;

Bool            synch         = False;
int	            verbose       = 0;
int	            loglevel      = 0;
Bool            background    = False;

FILE *          errorFile     = NULL;

char *          progname      = NULL;

char *          cdromDevice   = CDROM_DEV;

keyboard        kbd;                    /* the keyboard the user is using */

#ifdef HAVE_LIBXOSD
#define FONT    "-*-lucidatypewriter-bold-r-normal-*-*-250-*-*-*-*-*-*" 
#define COLOR   "LawnGreen"
#define TIMEOUT 3
xosd *          osd           = (xosd*)1;
#endif

/***====================================================================***/

void
usage(int argc, char *argv[])
{
    char cmplStr[256] = {'\0'};

#ifdef HAVE_LIBXOSD
    strcat(cmplStr,"XOSD");
#endif

    printf("HOTKEYS v" VERSION " -- use the hotkeys on your Internet/multimedia keyboard to control your computer");
    if (strlen(cmplStr))
        printf(" (compile options: %s)", cmplStr);
    printf("\n");
    printf("Usage: %s [options...]\n", argv[0]);
    printf("Legal options:\n");
#ifdef HAVE_GETOPT_LONG
    printf("\t-t, --type=TYPE          Specify the keyboard type (refer to -l)\n");
    printf("\t-l, --kbd-list           Show all supported keyboards\n");
    printf("\t-d, --cdrom-dev=DEVICE   Specify the CDROM/DVDROM device, or 'none'\n");
#ifdef HAVE_LIBXOSD
    printf("\t-o, --osd=STATE          Turn off/on on-screen display\n");
#endif
    printf("\t-L, --loglevel=LEVEL     Set the log level in syslog [0-7]\n");
    printf("\t-b, --background         Run in background\n");
    printf("\t-h, --help               Print this message\n");
/*
    M("-cfg <file>          Specify a config file\n");
    M("-d[isplay] <dpy>     Specify the display to watch\n");
    M("-v                   Print verbose messages\n");
*/
#else
    printf("\t-t TYPE       Specify the keyboard type (refer to -l)\n");
    printf("\t-l            Show all supported keyboards\n");
    printf("\t-d DEVICE     Specify the CDROM/DVDROM device, 'none' for no device\n");
    printf("\t-o STATE      Turn off/on on-screen display\n");
    printf("\t-L LEVEL      Set the log level in syslog [0-7]\n");
    printf("\t-b            Run in background\n");
    printf("\t-h            Print this message\n");
#endif /* HAVE_GETOPT_LONG */
}


void
showKbdList(int argc, char *argv[])
{
    DIR*            dir;
    struct dirent*  ent;
    char*           homedir;
    char*           h;
    int             flag = 0;
    int             len;

#ifdef HAVE_GETOPT_LONG
    printf("Supported keyboards: (with corresponding options to --kbd-list or -l)\n");
#else
    printf("Supported keyboards: (with corresponding options to -l)\n");
#endif

    /* Read the definition files in $HOME/.hotkeys */
    if ( (h = getenv("HOME")) != NULL )
    {
        homedir = XMALLOC( char, strlen(h) + strlen("/.hotkeys") + 1 );
        strcpy( homedir, h );
        strcat( homedir, "/.hotkeys" );
        if ( ( dir = opendir(homedir) ) != NULL )
        {
            while ( ent = readdir(dir) )
            {
                len = NLENGTH(ent);
                /* Show all files that ends with ".def" */
                if ( len > 4 &&
                     strncmp( &(ent->d_name[len-4]), ".def", 4 ) == 0 )
                {
                    ent->d_name[ len-4 ] = '\0';
                    if ( setKbdType(NULL, ent->d_name) == True )
                    {
                        printf( "\t%s -- \"%s\"\n", kbd.longName, ent->d_name );
                        flag = 1;
                    }
                }
            }
        }
        XFREE(homedir);
    }
    else
    {
        homedir = NULL;
    }

    /* Read the default location: SHAREDIR */
    if ( ( dir = opendir(SHAREDIR) ) != NULL )
    {
        while ( ent = readdir(dir) )
        {
            len = NLENGTH(ent);
            /* Show all files that ends with ".def" in SHAREDIR */
            if ( len > 4 &&
                 strncmp( &(ent->d_name[len-4]), ".def", 4 ) == 0 )
            {
                ent->d_name[ len-4 ] = '\0';
                if ( setKbdType(NULL, ent->d_name) == True )
                {
                    printf( "\t%s -- \"%s\"\n", kbd.longName, ent->d_name );
                    flag = 1;
                }
            }
        }
    }

    if ( flag == 0 )
    {
        printf( "NONE FOUND. Probably due to an unsuccessful installation.\n");
    }

    closedir(dir);
}


static Bool
setKbdType(const char* prog, const char* type)
{
    char*       defname;
    char*       h;
    Bool        ret = True;

    /* Make up the complete filename, try the default SHAREDIR
     * first */
    defname = XMALLOC( char, strlen(SHAREDIR)+strlen(type)+6 );
    strcpy( defname, SHAREDIR );
    strcat( defname, "/" );
    strcat( defname, type );
    strcat( defname, ".def" );

    if ( testReadable(defname) == True )     /* if the file exists... */
    {
        ret = readDefFile( defname );
    }
    else
    {
        /* Now try if it exists in $HOME/.hotkeys or not */
        if ( (h = getenv("HOME")) != NULL )
        {
            XFREE( defname );
            /* Make up the complete filename */
            defname = XMALLOC( char, strlen(h) +
                                     strlen("/.hotkeys/") +
                                     strlen(type) + 5 );
            strcpy( defname, h );
            strcat( defname, "/.hotkeys/" );
            strcat( defname, type );
            strcat( defname, ".def" );
            if ( testReadable(defname) == True )     /* if the file exists... */
            {
                ret = readDefFile( defname );
            }
            else
            {
                /* No matching keyboard type found even in the user's
                 * local directory */
                if ( prog != NULL )
                {
                    uInfo("Keyboard type `%s' is not supported.\n"
                            "Use %s --kbd-list to list all supported keyboard\n",
                            type, prog);
                    exit(0);
                }
                else
                {
                    ret = False;
                }
            }
        }
        else
        {
            /* No matching keyboard type */
            if ( prog != NULL )
            {
                uInfo("Keyboard type `%s' is not supported.\n"
                        "Use %s --kbd-list to list all supported keyboard\n",
                        type, prog);
                exit(0);
            }
            else
            {
                ret = False;
            }
        }
    }
    XFREE( defname );
    return ret;
}

/* Option is --cdrom-dev or -d */
void
setCDROMDevice(char* optarg)
{
    int fd;

    if ( strncasecmp( optarg, "none", 4 ) == 0 )
    {
        cdromDevice = NULL;
        return;
    }
    if ( (fd = open( optarg, O_RDONLY|O_NONBLOCK )) == -1)
    {
        uInfo("Unable to open `%s', fall back to %s\n", cdromDevice, CDROM_DEV);
    }
    else
    {
        if ( ( cdromDevice = (char*) xstrdup(optarg) ) == NULL )
        {
            uError("Insufficient memory");
            bailout();
        }
    }
    close (fd);
}


static void
setLoglevel(int level)
{
    /* Map the supplied level to the one defined in syslog.h */
    switch (level)
    {
        case 0:     loglevel = LOG_EMERG;       break;
        case 1:     loglevel = LOG_ALERT;       break;
        case 2:     loglevel = LOG_CRIT;        break;
        case 3:     loglevel = LOG_ERR;         break;
        case 4:     loglevel = LOG_WARNING;     break;
        case 5:     loglevel = LOG_NOTICE;      break;
        case 6:     loglevel = LOG_INFO;        break;
        case 7:     loglevel = LOG_DEBUG;       break;
        default:    loglevel = LOG_ERR;         break;
    }
}

#ifdef HAVE_LIBXOSD
static void
toggleOSD(char* optarg)
{
    int arg = 0;

    if ( strncasecmp( optarg, "on",  2 ) == 0 ||
         strncasecmp( optarg, "1",   1 ) == 0 ||
         strncasecmp( optarg, "yes", 3 ) == 0 )
    {
        arg = 1;
    }
    else
    if ( strncasecmp( optarg, "off", 3 ) != 0 &&
         strncasecmp( optarg, "0",   1 ) != 0 &&
         strncasecmp( optarg, "no",  2 ) != 0 )
    {
        uInfo("Unknown argument: %s, assuming on\n", optarg);
        arg = 1;
    }

    if ( !arg )
        osd = NULL;
}
#endif /* HAVE_LIBXOSD */

/***====================================================================***/

static Bool
parseArgs(int argc, char *argv[])
{
    int     c, i;
    int     digit_optind = 0;
    Bool    kbdSet = False;

    const char *flags = "hbt:d:lz:vL:"
#ifdef HAVE_LIBXOSD
        "o:"
#endif
    ;
#ifdef HAVE_GETOPT_LONG
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] =
    {
        {"help",            0, 0, 'h'},
        {"background",      0, 0, 'b'},
        {"type",            1, 0, 't'},
        {"cdrom-dev",       1, 0, 'd'},
        {"kbd-list",        0, 0, 'l'},
        {"verbose",         0, 0, 'v'},
        {"loglevel",        1, 0, 'L'},
#ifdef HAVE_LIBXOSD
        {"osd",             1, 0, 'o'},
#endif
        {0, 0, 0, 0}
    };
#endif /* HAVE_GETOPT_LONG */

    if ( strrchr( argv[0], '/' ) ) {
        /* strip the directories */
        progname = (char*) strrchr( argv[0], '/' ) + 1;
    } else {
        progname = argv[0];
    }

#ifdef HAVE_GETOPT_LONG
    while ((c = getopt_long(argc, argv, flags, long_options, &option_index)) != -1) {
#else
    while ((c = getopt(argc, argv, flags)) != -1) {
#endif /* HAVE_GETOPT_LONG */

        switch (c)
        {
#if 0
#ifdef HAVE_GETOPT_LONG
          case 0:

              if ( strncmp( long_options[option_index].name, "type", 4 ) == 0 )
              {
                  setKbdType(argv[0], optarg);
              } else
              if ( strncmp( long_options[option_index].name, "cdrom-dev", 9 ) == 0 )
              {
                  setCDROMDevice(optarg);
              }
              break;
#endif /* HAVE_GETOPT_LONG */
#endif

          case 't':
              setKbdType(argv[0], optarg);
              kbdSet = True;
              break;
          case 'd':
              setCDROMDevice(optarg);
              break;
          case 'h':
              usage(argc, argv);
              exit(0);
              break;
          case 'b':
              background = True;
              break;
          case 'l':
              showKbdList(argc, argv);
              exit(0);
              break;
          case 'L':
              setLoglevel(atoi(optarg));
              break;
#ifdef HAVE_LIBXOSD
          case 'o':
              toggleOSD(optarg);
              break;
#endif
          case 'z':
              break;
          case '?':
              break;

        }
    }

    if ( kbdSet == False )
    {
        uInfo("You must set the keyboard type, use %s -t <type> to set it.\n", argv[0]);
        exit(1);
    }

    /* check for a single additional argument */
    if ((argc - optind) > 1) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        bailout();
    }

    return True;
}

static Display *
GetDisplay(char* program, char* dpyName, int* opcodeRtrn, int* evBaseRtrn)
{
    int	mjr,mnr,error;
    Display	*dpy;

    mjr = XkbMajorVersion;
    mnr = XkbMinorVersion;
    dpy = XkbOpenDisplay(dpyName,evBaseRtrn,NULL,&mjr,&mnr,&error);
    if (dpy == NULL)
    {
        switch (error)
        {
            case XkbOD_BadLibraryVersion:
                uInfo("%s was compiled with XKB version %d.%02d\n",
                        program,XkbMajorVersion,XkbMinorVersion);
                uInfo("X library supports incompatible version %d.%02d\n",
                        mjr,mnr);
                break;
            case XkbOD_ConnectionRefused:
                uInfo("Cannot open display \"%s\"\n",dpyName);
                break;
            case XkbOD_NonXkbServer:
                uInfo("XKB extension not present on %s\n",dpyName);
                break;
            case XkbOD_BadServerVersion:
                uInfo("%s was compiled with XKB version %d.%02d\n",
                        program,XkbMajorVersion,XkbMinorVersion);
                uInfo("Server %s uses incompatible version %d.%02d\n",
                        dpyName,mjr,mnr);
                break;
            default:
                uInternalError("Unknown error %d from XkbOpenDisplay\n",error);
        }
    }
    else
    {
        if (synch)
            XSynchronize(dpy,True);
        if (opcodeRtrn)
            XkbQueryExtension(dpy,opcodeRtrn,evBaseRtrn,NULL,&mjr,&mnr);
    }
    return dpy;
}

void
bailout(void)
{
    if ( dpy != NULL )
	XCloseDisplay(dpy);
    uInfo("Bailing out...\n");
    exit(1);
}

static int
adjust_vol(int adj)
{
    int         mixer_fd = -1, cdrom_fd = -1;
    int         master_vol, cd_vol;
    struct cdrom_volctrl cdrom_vol;
    int         left, right;

    int ret = 0;

    /* open the mixer device */
    if ( (mixer_fd = open( MIXER_DEV, O_RDWR )) == -1 )
    {
        uError("Unable to open `%s'", MIXER_DEV);
    }
    else
    {
        if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_VOLUME, &master_vol) == -1)
        {
            uError("Unable to read the volume of `%s'", MIXER_DEV);
            ret = -1;
        }
        else
        {
            /* Set the master volume */
            left = (master_vol & 0xFF) + adj;
            right = ((master_vol >> 8) & 0xFF) + adj;
            left = (left>MAXLEVEL) ? MAXLEVEL : ((left<0) ? 0 : left);
            right = (right>MAXLEVEL) ? MAXLEVEL : ((right<0) ? 0 : right);
            master_vol = left + (right << 8);

            if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &master_vol) == -1)
            {
                uError("Unable to set the master volume");
                ret = -1;
            }
#ifdef HAVE_LIBXOSD
            else if (osd)
            {
                xosd_display(osd, 0, XOSD_string, "Volume");
                xosd_display(osd, 1, XOSD_percentage, (((left+right)/2)*100/MAXLEVEL));
            }
#endif
        }
#if 0
        if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_CD, &cd_vol) == -1)
        {
            uError("Unable to read the CD volume of `%s'", MIXER_DEV);
            ret = -1; goto LEAVE; 
        }
        else
        {
            /* Set the CD volume */
            left = (cd_vol & 0xFF) + adj;
            right = ((cd_vol >> 8) & 0xFF) + adj;
            left = (left>MAXLEVEL) ? MAXLEVEL : ((left<0) ? 0 : left);
            right = (right>MAXLEVEL) ? MAXLEVEL : ((right<0) ? 0 : right);
            cd_vol = left + (right << 8);

            if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_CD, &cd_vol) == -1)
            {
                uError("Unable to set the CD volume");
                ret = -1; goto LEAVE;
            }
        }
#endif
    }

    /* open the cdrom/dvdrom drive device */
    if ( cdromDevice != NULL )
    {
        if ( (cdrom_fd = open( cdromDevice, O_RDONLY|O_NONBLOCK )) == -1 )
        {
            uError("Unable to open `%s'", cdromDevice);
        }
        else
        {
            /* read the cdrom volume */
            if ( ioctl(cdrom_fd, CDROMVOLREAD, &cdrom_vol) == -1 )
            {
                uError("Unable to read the CDROM volume of `%s'", cdromDevice);
                ret = -1;
            }
            else
            {
                /* Set the CDROM volume */
                cdrom_vol.channel0 += adj; cdrom_vol.channel1 += adj;
                cdrom_vol.channel2 += adj; cdrom_vol.channel3 += adj;
                if ( ioctl(cdrom_fd, CDROMVOLCTRL, &cdrom_vol) == -1 )
                {
                    uError("Unable to set the volume of %s", cdromDevice);
                    ret = -1;
                }
            }
        }
    }

    if (mixer_fd != -1)     close(mixer_fd);
    if (cdrom_fd != -1)     close(cdrom_fd);

    return ret;
}


/*
 *  Mute or un-mute the /dev/mixer master volume and CDROM drive's volume
 */
static int
doMute(void)
{
    static Bool             muted = False;
    static int              last_mixer_vol, last_cd_vol;
    static struct cdrom_volctrl last_cdrom_vol;

    int                     vol, cd_vol;
    struct cdrom_volctrl    cdrom_vol;
    int                     mixer_fd = -1, cdrom_fd = -1;

    short ret = 0;      /* return value */

    /* open the mixer device */
    if ( (mixer_fd = open( MIXER_DEV, O_RDWR|O_NONBLOCK )) == -1 )
    {
        uError("Unable to open `%s'", MIXER_DEV);
    }
    /* open the cdrom/dvdrom drive device */
    if ( cdromDevice != NULL )
    {
        if ( (cdrom_fd = open( cdromDevice, O_RDONLY|O_NONBLOCK )) == -1 )
        {
            uError("Unable to open `%s'", cdromDevice);
        }
    }

    if ( muted )
    {
        /* Un-mute them */
        if (mixer_fd != -1)
        {
            if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &last_mixer_vol) == -1)
            {
                uError("Unable to un-mute the mixer");
                ret = -1;
            }
            else
            {
                muted = False;
#ifdef HAVE_LIBXOSD
                if (osd)
                {
                    int left = last_mixer_vol & 0xFF,
                        right = (last_mixer_vol >> 8) & 0xFF;
                    xosd_display(osd, 0, XOSD_string, "Unmute");
                    xosd_display(osd, 1, XOSD_percentage, (((left+right)/2)*100/MAXLEVEL));
                }
#endif
            }
        }
#if 0
        if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_CD, &last_cd_vol) == -1)
        {
            uError("Unable to un-mute the CD volume");
            ret = -1;
        } else
            muted = False;
#endif
        if (cdrom_fd != -1)
        {
            if ( ioctl(cdrom_fd, CDROMVOLCTRL, &last_cdrom_vol) == -1 )
            {
                uError("Unable to un-mute `%s'", cdromDevice);
                ret = -1;
            } else
                muted = False;
        }
    }
    else    /* ! muted */
    {

        /* Read and store the mixer volume, do not try to mute them
         * if we cannot read any of their values. */

        if (mixer_fd != -1)
        {
            if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_VOLUME, &last_mixer_vol) == -1)
            {
                uError("Unable to read the mixer volume of `%s'", MIXER_DEV);
                ret = -1;
            }
            else
            {
                /* Mute it! */
                vol = 0;
                if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &vol) == -1)
                {
                    uError("Unable to mute mixer volume of `%s'", MIXER_DEV);
                    ret = -1;
                }
                else
                {
                    muted = True;
#ifdef HAVE_LIBXOSD
                    if (osd)
                    {
                        xosd_display(osd, 0, XOSD_string, "Mute");
                        xosd_display(osd, 1, XOSD_string, "");
                    }
#endif
                }
            }
        }
#if 0
        if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_CD, &last_cd_vol) == -1)
        {
            uError("Unable to read the CD volume of `%s'", MIXER_DEV);
            ret = -1; goto LEAVE2;
        }
        else
        {
            if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_CD, &vol) == -1)
            {
                uError("Unable to mute CD volume of `%s'", MIXER_DEV);
                ret = -1;
            } else
                muted = True;
        }
#endif
        /* read and store the cdrom volume */
        if (cdrom_fd != -1)
        {
            if ( ioctl(cdrom_fd, CDROMVOLREAD, &last_cdrom_vol) == -1 )
            {
                uError("Unable to read the CDROM volume of `%s'", cdromDevice);
                ret = -1;
            }
            else
            {
                /* Set the volume to 0. FIXME: is this linux specific? Do
                 * other platforms also have 4 channels? */
                cdrom_vol.channel0 = cdrom_vol.channel1 = cdrom_vol.channel2 =
                    cdrom_vol.channel3 = 0;
                if ( ioctl(cdrom_fd, CDROMVOLCTRL, &cdrom_vol) == -1 )
                {
                    uError("Unable to mute `%s'", cdromDevice);
                    ret = -1;
                } else
                    muted = True;
            }
        }
    }

    if (mixer_fd != -1)   close(mixer_fd);
    if (cdrom_fd != -1)   close(cdrom_fd);

    return ret;
}

static int
ejectDisc(void)
{
    static Bool ejected = False;

    if ( cdromDevice == NULL )
        return 0;

    if ( ejected )
    {
        if ( closeTray() == 0 )
        {
            ejected = False;
            return 0;
        }
    }
    else
    {
        int fd;

        if ( (fd = open(cdromDevice, O_RDONLY|O_NONBLOCK)) == -1)
        {
            uError("Unable to open `%s'", cdromDevice);
            return -1;
        }
        if ( ioctl(fd, CDROMEJECT) == -1 )
        {
            uError("CD-ROM device %s eject failed", cdromDevice);
            close(fd);
            return -1;
        }

#ifdef HAVE_LIBXOSD
        xosd_display(osd, 0, XOSD_string, "Eject");
        xosd_display(osd, 1, XOSD_string, "");
#endif
        ejected = True;
        close(fd);
        return 0;
    }
}

static int
closeTray(void)
{
    int fd;

    if ( cdromDevice == NULL )
        return 0;

    if ( (fd = open(cdromDevice, O_RDONLY|O_NONBLOCK)) == -1)
    {
        uInfo("unable to open `%s'\n", cdromDevice);
        return -1;
    }

    /* Close it in case it's already opened */
#ifdef CDROMCLOSETRAY
    if ( ioctl(fd, CDROMCLOSETRAY) == -1 )
    {
        uInfo("CD-ROM tray of device %s close command failed",
                cdromDevice);
        close(fd);
        return -1;
    }
#endif /* CDROMCLOSETRAY */

#ifdef HAVE_LIBXOSD
    xosd_display(osd, 0, XOSD_string, "Close tray");
    xosd_display(osd, 1, XOSD_string, "");
#endif
    close(fd);
    return 0;
}

static int
playDisc(void)
{
    return closeTray();
}


static int
launchApp(int type)
{
    int pid = fork();
    if ( pid == -1 )
    {
        uInfo("Cannot launch the %s\n", app_strings[type]);
    }
    else if ( pid == 0 )
    {
        /* Construct the argument arrays */
        char**  arg_array;
        char*   c = application_args[type];
        char*   cc;
        int     noOfArgs = 1;   /* including the NULL element */
        int     i = 0;
        do {
            c = strchr( c+1, ' ' );
            noOfArgs++;
        } while ( c != NULL );
        arg_array = XMALLOC( char*, noOfArgs );
        /* dup needed since strtok modifies the string */
        c = (char*) xstrdup( application_args[type] );
        cc = c;         /* for free() */
        arg_array[0] = strtok( c, " " );
        while ( arg_array[i] != NULL )
        {
            i++;
            arg_array[i] = strtok( NULL, " " );
        }

        if ( execvp(applications[type], arg_array) == -1 )
        {
            uError("Cannot launch the %s", app_strings[type]);
            XFREE(cc);
            XFREE(arg_array);
            exit(-1);
        }
    }
    else
    {
        xosd_display(osd, 0, XOSD_string, "Launching:");
        xosd_display(osd, 1, XOSD_string, app_strings[type]);
    }
}


/*
int
launchMailer(void)
{
    int pid = fork();
    if ( pid == -1 )
    {
        uError("Cannot launch the Email client: %s\n", strerror(errno));
    }
    else if ( pid == 0 )
    {
        char* args[] = MAILER_ARGS;
        if ( execvp(MAILER, args) == -1 ) {
            uError("Cannot launch the Email client\n");
        }
    }
}
*/


int
sleepState(int mode)
{
#ifdef USE_APMD
    switch (mode)
    {
      case SUSPEND:
        error = system("apm -s");
        break;
      case STANDBY:
        error = system("apm -S");
        break;
      default:
        error = 0;
        break;
    }
#else
    int fd;
    int error;

    if ( (fd = apm_open()) < 0 )
    {
        uError("unable to open APM device");
        exit(1);
    }
    switch (mode)
    {
      case SUSPEND:
        error = apm_suspend(fd);
        break;
      case STANDBY:
        error = apm_standby(fd);
        break;
      default:
        error = 0;
        break;
    }
    apm_close(fd);
#endif /* USE_APMD */
}


static void
lookupUserCmd(const int keycode)
{
    int     i;

    for ( i = 0; i < kbd.noOfCustomCmds; i++ )
    {
        if ( kbd.customCmds[i].keycode == keycode )
        {
            int pid;

            if ( (pid=fork()) == -1 )
            {
                uInfo("Cannot launch \"%s\"\n", kbd.customCmds[i].desc);
            }
            else if ( pid == 0 )
            {
                /* Construct the argument arrays */
                char**  arg_array;
                char*   c = kbd.customCmds[i].command;
                char*   cc;
                int     noOfArgs = 1;   /* including the NULL element */
                int     j = 0;
                do {
                    c = strchr( c+1, ' ' );
                    noOfArgs++;
                } while ( c != NULL );
                arg_array = XMALLOC( char*, noOfArgs );
                /* dup needed since strtok modifies the string */
                c = (char*) xstrdup( kbd.customCmds[i].command ); 
                cc = c;         /* cc is for free() later */
                arg_array[0] = strtok( c, " " );
                while ( arg_array[j] != NULL )
                {
                    j++;
                    arg_array[j] = strtok( NULL, " " );
                }

                if ( execvp(arg_array[0], arg_array) == -1 )
                {
                    uError("Cannot launch \"%s\"", kbd.customCmds[i].desc);
                    XFREE(cc);
                    XFREE(arg_array);
                    exit(-1);
                }
            }
            else
            {
                xosd_display(osd, 0, XOSD_string, "Launching:");
                xosd_display(osd, 1, XOSD_string, kbd.customCmds[i].desc);
                break;  /* break the for loop */
            }
        }
    }
}

/***====================================================================***/

#ifdef DEBUG
static void
printXkbActionMessage(FILE* file,XkbEvent* xkbev)
{
    XkbActionMessageEvent *msg= &xkbev->message;
    SYSLOG( LOG_DEBUG, "message: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                                        msg->message[0],msg->message[1],
                                        msg->message[2],msg->message[3],
                                        msg->message[4],msg->message[5]);
    SYSLOG( LOG_DEBUG, "key %d, event: %s,  follows: %s\n",msg->keycode,
                                     (msg->press?"press":"release"),
                                     (msg->key_event_follows?"yes":"no"));
    return;
}
#endif


void
uError(char* s,...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile,"%s: ", progname);
    vfprintf(errorFile,s,ap);
    fprintf(errorFile,": %s\n", strerror(errno));
    fflush(errorFile);
    va_end(ap);
    return;
}

void
uInfo(char* s,...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile,"%s: ", progname);
    vfprintf(errorFile,s,ap);
    fflush(errorFile);
    va_end(ap);
    return;
}

void
uInternalError(char* s,...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile,"%s: internal error: ", progname);
    vfprintf(errorFile,s,ap);
    fflush(errorFile);
    va_end(ap);
    return;
}


/*
 * Test whether filename is readable
 */
Bool
testReadable(const char* filename)
{
    int fd;
 
    if ( (fd = open(filename, O_RDONLY)) == -1 )
    {
        return False;
    }
    else
    {
        close(fd);
        return True;
    }
}

/***====================================================================***/
static int dummy() {}

static void
initializeX(char* argv[])
{
    KeySym              newKS;
    XkbMessageAction    xma;
    XkbMapChangesRec    mapChangeRec;
    int                 types[1];
    int                 i;
    int                 tcode;

    dpy = GetDisplay(argv[0], dpyName, &xkbOpcode, &xkbEventCode);
    if (!dpy)
        bailout();

    /* Select the ActionMessage event */
    if ( !XkbSelectEvents( dpy, XkbUseCoreKbd, XkbActionMessageMask,
                           XkbActionMessageMask ))
    {
        uInfo("Couldn't select desired XKB events\n");
        bailout();
    }
#if 0
    xkb= XkbGetKeyboard(dpy,XkbGBN_AllComponentsMask,XkbUseCoreKbd);
    xkb= XkbGetKeyboard(dpy,XkbAllComponentsMask,XkbUseCoreKbd);
#endif
    xkb = XkbGetMap(dpy, XkbAllMapComponentsMask, XkbUseCoreKbd);
    if (!xkb)
    {
        uInfo("XkbGetMap failed\n"); bailout();
    }
    XSetAfterFunction(dpy,dummy);
    /* Construct the Message Action struct */
    xma.type = XkbSA_ActionMessage;
    xma.flags = XkbSA_MessageOnPress;
    strcpy(xma.message," ");

#ifdef DEBUG
    SYSLOG( LOG_DEBUG, "num_acts:%d size_acts:%d",
            xkb->server->num_acts, xkb->server->size_acts );
#endif

    /* Add KeySym to the key codes, as they don't have any KeySyms before */
    for ( i = 0; i < NUM_PREDEF_HOTKEYS + kbd.noOfCustomCmds; i++ )
    {
        if ( i < NUM_PREDEF_HOTKEYS )
        {
            tcode = (kbd.keycodes)[i];
            if ( tcode == 0 )
                continue;
        }
        else
        {
            tcode = kbd.customCmds[i-NUM_PREDEF_HOTKEYS].keycode;
        }

        /* Assign a group to the key code */
/*
        types[XkbGroup1Index] = XkbKeyTypeIndex( xkb, code, XkbGroup1Index );
*/
        types[0] = XkbOneLevelIndex;
        if ( XkbChangeTypesOfKey(xkb, tcode, 1, XkbGroup1Mask, types, NULL)
                != Success )
        {
            uError("damn it!"); bailout();
        }

        /* Change their Keysyms */
        if ( XkbResizeKeySyms( xkb, tcode, 1 ) == NULL )
        {
            uInfo("resize keysym failed\n"); bailout();
        }
        /* Add one key action to it */
        if ( XkbResizeKeyActions( xkb, tcode, 1 ) == NULL )
        {
            uInfo("resize key action failed\n"); bailout();
        }

        /* Assign a new keysym to the key code */
        newKS = 0x2200+i;       /* FIXME: I just choose a keysym that's not yet assigned */
/*
        newKS = 0xFFE0;
*/
        *XkbKeySymsPtr(xkb,tcode) = newKS;

        /* Send the change back to the server */
        /* XkbKeyActionsMask must be here, just XkbKeySymsMask|XkbKeyTypesMask
         * is not sufficient */
        if ( XkbSetMap(dpy, XkbKeySymsMask|XkbKeyActionsMask|XkbKeyTypesMask, xkb) )
        {
#ifdef DEBUG
            SYSLOG( LOG_DEBUG, "Map set done");
#endif
        }
        else
        {
            uInfo("map set failed\n"); bailout();
        }

#ifdef DEBUG
        SYSLOG( LOG_DEBUG, "idx:%d has action:%d no.:%d noOfGrps:%d\n",
                xkb->server->key_acts[tcode], XkbKeyHasActions(xkb,tcode),
                XkbKeyNumActions(xkb,tcode), XkbKeyNumGroups(xkb,tcode) );
        SYSLOG( LOG_DEBUG, "keycode %d\nbefore: %d",
                tcode, XkbKeyActionsPtr(xkb,tcode)[0].type);
#endif
        /* Assign the Message Action to the key code */
        (&(xkb->server->acts[ xkb->server->key_acts[tcode] ]))[0] = (XkbAction) xma;

#ifdef DEBUG
        SYSLOG( LOG_DEBUG, "after: %d",XkbKeyActionsPtr(xkb,tcode)[0].type);
#endif
    }

    /***************************************************************/

    /* Commit the change back to the server. This is necessary! */
    if ( XkbSetMap(dpy, XkbKeyActionsMask, xkb) )
    {
#ifdef DEBUG
        SYSLOG( LOG_DEBUG, "map set done");
#endif
    }
    else
    {
        uInfo("map set failed\n"); bailout();
    }

    /* Initialize XOSD */
#ifdef HAVE_LIBXOSD
    if (osd)
    {
        osd = xosd_init(FONT, COLOR, TIMEOUT, XOSD_bottom, 25);
    }
#endif
}

static void
removeCorpse(int s)
{
    int     status;
    pid_t   pid;

    pid = wait(&status);
#ifdef DEBUG
    SYSLOG( LOG_DEBUG, "Child %d exited\n", pid);
#endif
}


static void
installSigHandler(void)
{
    struct sigaction s;

    bzero(&s, sizeof(s));
    s.sa_handler = removeCorpse;
    s.sa_flags = SA_NOCLDSTOP;

    sigaction( SIGCHLD, &s, NULL);
}


int
main(int argc, char *argv[])
{
    XkbEvent    ev;
    int         i, k;

    errorFile = stderr;
    openlog( PACKAGE, LOG_CONS | LOG_PID, LOG_USER );

    /* initialize the kbd variable */
    kbd.noOfCustomCmds = 0;
    kbd.keycodes = XCALLOC( hotkey, NUM_PREDEF_HOTKEYS );

    if ( !parseArgs(argc,argv) )
        bailout();

    if (background)
    {
        if ( fork() !=0 )
        {
            SYSLOG( LOG_NOTICE, "Running in the background");
            exit(0);
        }
    }

    initializeX(argv);
    installSigHandler();

    /* Process the events in a forever loop */
    while (1)
    {
        XNextEvent( dpy, &ev.core );
#ifdef DEBUG
        printXkbActionMessage( stdout, &ev );
#endif
        if ( ev.type == xkbEventCode+XkbEventCode &&
             ev.any.xkb_type == XkbActionMessage )
        {
            SYSLOG( LOG_INFO, "Keycode %d pressed\n", ev.message.keycode );

            /* Sound stuffs */
            if ( ev.message.keycode == (kbd.keycodes)[playKey] ) {
                playDisc();
            } else 
            if ( ev.message.keycode == (kbd.keycodes)[ejectKey] ) {
                ejectDisc();
            } else 
            if ( ev.message.keycode == (kbd.keycodes)[volUpKey] ) {
                adjust_vol(2);
            } else 
            if ( ev.message.keycode == (kbd.keycodes)[volDownKey] ) {
                adjust_vol(-2);
            } else 
            if ( ev.message.keycode == (kbd.keycodes)[muteKey] ) {
                doMute();
            } else
            /* Apps stuffs */
            if ( ev.message.keycode == (kbd.keycodes)[browserKey] ) {
                launchApp(app_browser);
            } else
            if ( ev.message.keycode == (kbd.keycodes)[emailKey] ) {
                launchApp(app_mailer);
            } else
            if ( ev.message.keycode == (kbd.keycodes)[calculatorKey] ) {
                launchApp(app_calculator);
            } else
            if ( ev.message.keycode == (kbd.keycodes)[myComputerKey] ) {
                launchApp(app_filemanager);
            } else
            /* APM stuffs */
            if ( ev.message.keycode == (kbd.keycodes)[sleepKey] ||
                 ev.message.keycode == (kbd.keycodes)[wakeupKey] ) {
                sleepState(STANDBY);
            } else
            if ( ev.message.keycode == (kbd.keycodes)[powerDownKey] ) {
                sleepState(SUSPEND);
            }
            else
            {
                lookupUserCmd(ev.message.keycode);  /* User-defined stuffs */
            }
        }
    }

#ifdef HAVE_LIBXOSD
    if (osd)
        xosd_uninit(osd);
#endif
    XCloseDisplay(dpy);
    closelog();
    return 0;
}

