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
#include <db.h>
#include <errno.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif /* HAVE_GETOPT_LONG */
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
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
#include "conf.h"

#include <X11/Xmu/Error.h>

#define	lowbit(x)	((x) & (-(x)))
#define	M(m)	fprintf(stderr,(m))
#define	M1(m,a)	fprintf(stderr,(m),(a))

/***====================================================================***/

char *      dpyName           = NULL;
Display *   dpy               = NULL;
char *      cfgFileName       = NULL;
int     	xkbOpcode         = 0;
int     	xkbEventCode      = 0;
XkbDescPtr  xkb               = NULL;
Bool        dummyErrFlag      = False;

Bool            synch         = False;
int	            verbose       = 0;
int	            loglevel      = 0;
Bool            background    = False;

FILE *          errorFile     = NULL;

char *          progname      = NULL;

char *          cdromDevice   = CDROM_DEV;

keyboard        kbd;                    /* the keyboard the user is using */

#ifdef HAVE_LIBXOSD
xosd *          osd           = (xosd*)1;
#endif

int             volUpAdj      = 2;
int             volDownAdj    = -2;

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
    printf("    -t, --type=TYPE          Specify the keyboard type (refer to -l)\n");
    printf("    -l, --kbd-list           Show all supported keyboards\n");
    printf("    -d, --cdrom-dev=DEVICE   Specify the CDROM/DVDROM device, or 'none'\n");
#ifdef HAVE_LIBXOSD
    printf("    -o, --osd=STATE          Turn off/on on-screen display\n");
#endif
    printf("    -L, --loglevel=LEVEL     Set the log level in syslog [0-7]\n");
    printf("    -b, --background         Run in background\n");
    printf("    -F, --fix-vmware=TIME    Use this option if vmware is used concurrently\n");
    printf("    -h, --help               Print this message\n");
/*
    M("-cfg <file>          Specify a config file\n");
    M("-d[isplay] <dpy>     Specify the display to watch\n");
    M("-v                   Print verbose messages\n");
*/
#else
    printf("    -t TYPE       Specify the keyboard type (refer to -l)\n");
    printf("    -l            Show all supported keyboards\n");
    printf("    -d DEVICE     Specify the CDROM/DVDROM device, 'none' for no device\n");
    printf("    -o STATE      Turn off/on on-screen display\n");
    printf("    -L LEVEL      Set the log level in syslog [0-7]\n");
    printf("    -b            Run in background\n");
    printf("    -F TIME       Use this option if vmware is used concurrently\n");
    printf("    -h            Print this message\n");
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
                        printf( "    %s\t- %s\n", ent->d_name, kbd.longName );
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
                    printf( "    %s\t- %s\n", ent->d_name, kbd.longName );
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

    if ( testReadable(defname) )     /* if the file exists... */
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
            if ( testReadable(defname) )     /* if the file exists... */
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
        uInfo("Unable to open `%s', fall back to %s\n", optarg, CDROM_DEV);
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
    setlogmask( LOG_UPTO(loglevel) );
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

    const char *flags = "hbt:d:lz:vL:F:"
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
        {"fix-vmware",      2, 0, 'F'},
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
              setConfig( "Kbd", optarg, 0 );
              break;
          case 'd':
              setConfig( "CDROM", optarg, 0 );
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
          case 'F':
              fixVMware(optarg);
              break;
          case 'z':
              break;
          case '?':
              break;

        }
    }

    if ( getConfig("Kbd")[0] )
    {
        setKbdType( argv[0], getConfig("Kbd") );
    }
    else
    {
        uInfo("You must set the keyboard type, use %s -t <type> to set it.\n", argv[0]);
        exit(1);
    }

    if ( getConfig("CDROM")[0] )
        setCDROMDevice( getConfig("CDROM") );

    /* check for a single additional argument */
    if ((argc - optind) > 1) {
        fprintf( stderr, "%s: too many arguments\n", argv[0] );
        bailout();
    }

    return True;
}

/* Copied from xkbevd */
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


/*
 * adj is a percentage, can be +ve or -ve for louder and softer resp.
 */
static int
adjustVol(int adj)
{
    int         mixer_fd = -1, cdrom_fd = -1;
    int         master_vol, cd_vol;
    struct cdrom_volctrl cdrom_vol;
    int         left, right;

    int ret = 0;

    /* open the mixer device */
    if ( (mixer_fd = open( MIXER_DEV, O_RDWR|O_NONBLOCK )) == -1 )
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
                int     t;
                float   myAdj;
                myAdj = 0xFF / 100.0 * adj;
                t = cdrom_vol.channel0 + myAdj;
                cdrom_vol.channel0 = (t < 0 ? 0 : (t > 0xFF ? 0xFF : t));
                t = cdrom_vol.channel1 + myAdj;
                cdrom_vol.channel1 = (t < 0 ? 0 : (t > 0xFF ? 0xFF : t));
#if 0
                t = cdrom_vol.channel2 + myAdj;
                cdrom_vol.channel2 = (t < 0 ? 0 : (t > 0xFF ? 0xFF : t));
                t = cdrom_vol.channel3 + myAdj;
                cdrom_vol.channel3 = (t < 0 ? 0 : (t > 0xFF ? 0xFF : t));
#endif
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
//                    xosd_display(osd, 1, XOSD_percentage, (((left+right)/2)*100/MAXLEVEL));
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
                        xosd_set_timeout(osd, -1);
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
#ifdef HAVE_LIBXOSD
        xosd_display(osd, 0, XOSD_string, "Eject");
        xosd_display(osd, 1, XOSD_string, "");
#endif
        if ( ioctl(fd, CDROMEJECT) == -1 )
        {
            uError("CD-ROM device %s eject failed", cdromDevice);
            close(fd);
            return -1;
        }

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

#ifdef HAVE_LIBXOSD
    xosd_display(osd, 0, XOSD_string, "Close tray");
    xosd_display(osd, 1, XOSD_string, "");
#endif

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

    close(fd);
    return 0;
}


static int
launchApp(int keycode)
{
    int     pid;
    char*   type = NULL;
    int     i;

    /* Given the keycode, we find the string corresponding to it */
    for ( i = 0; i < NUM_PREDEF_HOTKEYS; i++ )
    {
        if ( keycode == (kbd.defCmds)[i].key )
        {
            type = defStr[i].name;
            break;
        }
    }
    if ( type == NULL )
        return -1;  /* this keycode is not associated with any app */

    if ( (pid=fork2()) == -1 )
    {
        uInfo("Cannot launch the %s\n", type);
    }
    else if ( pid == 0 )
    {
        /* Construct the argument arrays */
        char**  arg_array;
        char*   c = getConfig(type);
        char*   cc;
        int     noOfArgs = 1;   /* including the NULL element */
        int     i = 0;
        do {
            c = strchr( c+1, ' ' );
            noOfArgs++;
        } while ( c != NULL );
        arg_array = XMALLOC( char*, noOfArgs );
        /* dup needed since strtok modifies the string */
        c = (char*) xstrdup( getConfig(type) );
        cc = c;         /* for free() */
        arg_array[0] = strtok( c, " " );
        while ( arg_array[i] != NULL )
        {
            i++;
            arg_array[i] = strtok( NULL, " " );
        }

        if ( execvp(arg_array[0], arg_array) == -1 )
        {
            uError("Cannot launch %s", type);
            XFREE(cc);
            XFREE(arg_array);
            _exit(-1);
        }
    }
    else
    {
#ifdef HAVE_LIBXOSD
        xosd_display(osd, 0, XOSD_string, "Launching:");
        xosd_display(osd, 1, XOSD_string, getConfig(type));
#endif
    }
    return 0;
}


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

            if ( (pid=fork2()) == -1 )
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
                    _exit(-1);
                }
            }
            else
            {
#ifdef HAVE_LIBXOSD
                xosd_display(osd, 0, XOSD_string, "Launching:");
                xosd_display(osd, 1, XOSD_string, kbd.customCmds[i].desc);
#endif
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
int
testReadable(const char* filename)
{
    int fd;
 
    if ( (fd = open(filename, O_RDONLY)) == -1 )
    {
        return 0;
    }
    else
    {
        close(fd);
        return 1;
    }
}

/***====================================================================***/
static int dummy() { /* grin */ }

static int
dummyHandler(Display* d, XErrorEvent* ev) 
{
    if ( d == dpy && ev->error_code == BadValue &&
         ev->request_code == 149 /* XKEYBOARD */ &&
         ev->minor_code == 9 /* XkbSetMap */ )
    {
        dummyErrFlag = True;
        SYSLOG( LOG_INFO, "X BadValue Error" );
    }
    else
    {
        XmuPrintDefaultErrorMessage(d, ev, stderr);
    }
    return 0;
}

static void
commitXKBChanges(int tcode)
{
    XkbMapChangesRec    mapChangeRec;

    /* Commit the change back to the server */
    bzero(&mapChangeRec, sizeof(mapChangeRec));
    mapChangeRec.changed = XkbKeySymsMask | XkbKeyTypesMask;
    mapChangeRec.first_key_sym = tcode;
    mapChangeRec.num_key_syms = 1;
    //        mapChangeRec.first_key_act = tcode;
    //        mapChangeRec.num_key_acts = 1;
    mapChangeRec.first_type = 0;
    mapChangeRec.num_types = xkb->map->num_types;
    if ( XkbChangeMap(dpy, xkb, &mapChangeRec) )
    {
#ifdef DEBUG
        printf("map changed done: %d\n",tcode);
#endif
    }
    else
    {
        uError("map changed failed\n"); bailout();
    }
}

void
initializeX(char* prg)
{
    KeySym              newKS;
    XkbMessageAction    xma;
    XkbMapChangesRec    mapChangeRec;
    int                 types[1];
    int                 i;
    int                 tcode;

    dpy = GetDisplay(prg, dpyName, &xkbOpcode, &xkbEventCode);
    if (!dpy)
        bailout();
    /* This function is NECESSARY to prevent the X BadValue error
     * when running in Synchronize mode */
    XSetAfterFunction(dpy, dummy);
    XSetErrorHandler(dummyHandler);

    /* Construct the Message Action struct */
    xma.type = XkbSA_ActionMessage;
    xma.flags = XkbSA_MessageOnPress;
    strcpy(xma.message," ");

#if 0
#ifdef DEBUG
    SYSLOG( LOG_DEBUG, "num_acts:%d size_acts:%d",
            xkb->server->num_acts, xkb->server->size_acts );
#endif
#endif

    /* Add KeySym to the key codes, as they don't have any KeySyms before */
    for ( i = 0; i < NUM_PREDEF_HOTKEYS + kbd.noOfCustomCmds; i++ )
    {
        if ( i < NUM_PREDEF_HOTKEYS )
        {
            tcode = (kbd.defCmds)[i].key;
            newKS = (kbd.defCmds)[i].keysym;
            if ( tcode == 0 )
                continue;
        }
        else
        {
            tcode = kbd.customCmds[i-NUM_PREDEF_HOTKEYS].keycode;
            newKS = kbd.customCmds[i-NUM_PREDEF_HOTKEYS].keysym;
        }
#if 0
        xkb= XkbGetKeyboard(dpy,XkbGBN_AllComponentsMask,XkbUseCoreKbd);
        xkb= XkbGetKeyboard(dpy,XkbAllComponentsMask,XkbUseCoreKbd);
#endif
        xkb = XkbGetMap(dpy, XkbAllMapComponentsMask, XkbUseCoreKbd);
        if (!xkb)
        {
            uError("XkbGetMap failed\n"); bailout();
        }

        /* Check the keycode range */
        if ( ! XkbKeycodeInRange(xkb, tcode) )
        {
            uInfo("The keycode %d cannot be used, as it's not between the min(%d) and max(%d) keycode of your keyboard.\n"
                  "Please increase the 'maximum' value in /usr/X11R6/lib/X11/xkb/keycodes/xfree86, then restart X.",
                  tcode, xkb-> min_key_code, xkb->max_key_code);
            continue;
        }

        /* Assign a group to the key code */
        types[0] = XkbOneLevelIndex;
        if ( XkbChangeTypesOfKey(xkb, tcode, 1, XkbGroup1Mask, types, NULL)
                != Success )
        {
            uError("XkbChangeTypesOfKey failed"); bailout();
        }
/*
        types[XkbGroup1Index] = XkbKeyTypeIndex( xkb, code, XkbGroup1Index );
*/

        /* Change their Keysyms */
        if ( XkbResizeKeySyms( xkb, tcode, 1 ) == NULL )
        {
            uInfo("resize keysym failed\n"); bailout();
        }
        /* Assign a new keysym to the key code.  According to
         * XF86keysym.h, the vendor specific XFree86 keysym range is
         * 0x1008FF01 to 0x1008FFFF. So I just assign keysyms to the
         * internet keys starting from 0x1008FF01.  I think it doesn't
         * really matter to which one I use for the moment. For the
         * exact keysyms, please refer to XKeysymDB, */
//        newKS = 0x1008FF01 + tcode;       
        *XkbKeySymsPtr(xkb,tcode) = newKS;

#if 0
printf("keycode %d owns keysym %x\n", tcode,newKS);
        XChangeKeyboardMapping(dpy, tcode, 1, &newKS, 1);
#endif

        /* Add one key action to it */
        if ( XkbResizeKeyActions( xkb, tcode, 1 ) == NULL )
        {
            uInfo("resize key action failed\n"); bailout();
        }

        commitXKBChanges(tcode);
        commitXKBChanges(tcode);    /* YES, we need to call it twice! */

        /* Assign the Message Action to the key code */
        (&(xkb->server->acts[ xkb->server->key_acts[tcode] ]))[0] = (XkbAction) xma;

        /* Commit the change back to the server. Yeah we need to do it
         * here instead of in commit XKBChanges(). Strange, eh?  But
         * you just can't, I wonder what the fsck X is doing.  I get
         * this just by lots of trial-and-error and many nights of no
         * sleeping to trace X with gdb. */
        bzero(&mapChangeRec, sizeof(mapChangeRec));
        mapChangeRec.changed = XkbKeyActionsMask;
        mapChangeRec.first_key_act = tcode;
        mapChangeRec.num_key_acts = 1;
        if ( XkbChangeMap(dpy, xkb, &mapChangeRec) )
        {
#ifdef DEBUG
            printf("map changed done: %d\n",tcode);
#endif
        }
        else
        {
            uError("map changed failed\n"); bailout();
        }
#if 0
#ifdef DEBUG
        SYSLOG( LOG_DEBUG, "idx:%d has action:%d no.:%d noOfGrps:%d\n",
                xkb->server->key_acts[tcode], XkbKeyHasActions(xkb,tcode),
                XkbKeyNumActions(xkb,tcode), XkbKeyNumGroups(xkb,tcode) );
        SYSLOG( LOG_DEBUG, "keycode %d\nbefore: %d",
                tcode, XkbKeyActionsPtr(xkb,tcode)[0].type);
#endif

//    xkb = XkbGetMap(dpy, XkbAllMapComponentsMask, XkbUseCoreKbd);
#ifdef DEBUG
        SYSLOG( LOG_DEBUG, "after: %d",XkbKeyActionsPtr(xkb,tcode)[0].type);
#endif
#endif

        if ( dummyErrFlag ) /* dummyHandler() have set it */
        {
            i--;            /* need need to redo this round, as the action message
                               was not assigned to this keysym */
            dummyErrFlag == False;
        }
    }

    /* Select the ActionMessage event in any circumstances */
    if ( !XkbSelectEvents( dpy, XkbUseCoreKbd, XkbActionMessageMask,
                           XkbActionMessageMask ))
    {
        uInfo("Couldn't select desired XKB events\n");
        bailout();
    }
}

/* Initialize XOSD */
void
initXOSD(void)
{
#ifdef HAVE_LIBXOSD
    if (osd)
    {
        osd = xosd_init(getConfig("osd_font"),
                        /* I dunno why, but you must call strdup here... */
                        xstrdup(getConfig("osd_color")),
                        atoi(getConfig("osd_timeout")),
                        strncmp(getConfig("osd_position"),"top",3)?XOSD_bottom:XOSD_top,
                        atoi(getConfig("osd_offset")) );
    }
#endif
}


/* fork2() -- like fork, but the new process is immediately orphaned
 *            (won't leave a zombie when it exits)                 
 * Returns 1 to the parent, not any meaningful pid.               
 * The parent cannot wait() for the new process (it's unrelated).
 */

/* This version assumes that you *haven't* caught or ignored SIGCHLD. */
/* If you have, then you should just be using fork() instead anyway.  */

/* fork2() is from the Unix Programming FAQ */
int
fork2(void)
{
    pid_t pid;
    int rc;
    int status;

    if (!(pid = fork()))
    {
        switch (fork())
        {
            case 0:  return 0;
            case -1: _exit(errno);    /* assumes all errnos are <256 */
            default: _exit(0);
        }
    }

    if (pid < 0 || waitpid(pid,&status,0) < 0)
        return -1;

    if (WIFEXITED(status))
        if (WEXITSTATUS(status) == 0)
            return 1;
        else
            errno = WEXITSTATUS(status);
    else
        errno = EINTR;  /* well, sort of :-) */

    return -1;
}

int
main(int argc, char *argv[])
{
    XkbEvent    ev;
    int         i, k;

    errorFile = stderr;
    openlog( PACKAGE, LOG_CONS | LOG_PID, LOG_USER );

    readConfigFile();

    /* initialize the kbd variable */
    kbd.noOfCustomCmds = 0;
    kbd.defCmds = XCALLOC( defEntry, NUM_PREDEF_HOTKEYS );

    if ( !parseArgs(argc,argv) )
        bailout();

    if (background)
    {

        if ( fork() !=0 )
        {
            SYSLOG( LOG_NOTICE, "Running in the background");
            _exit(0);
        }
        else
        {
            chdir("/");
        }
    }

    initializeX(argv);
    initXOSD();

    printf( "%s started successfully.\n", progname );

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

#ifdef HAVE_LIBXOSD
            if (osd)
                xosd_set_timeout(osd, atoi(getConfig("osd_timeout")));
#endif
            if ( keytypes[ev.message.keycode] == 1 )
            {
                /* Apps stuffs */
                launchApp(ev.message.keycode);
            } else
            /* Sound stuffs */
            if ( ev.message.keycode == (kbd.defCmds)[ejectKey].key )
            {
                /* Use thread to improve the responsiveness */
                pthread_t       tp;
                pthread_attr_t  attr;

                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
                pthread_create (&tp, &attr, ejectDisc, NULL);
            } else 
            if ( ev.message.keycode == (kbd.defCmds)[volUpKey].key ) {
                adjustVol(volUpAdj);
            } else 
            if ( ev.message.keycode == (kbd.defCmds)[volDownKey].key ) {
                adjustVol(volDownAdj);
            } else 
            if ( ev.message.keycode == (kbd.defCmds)[muteKey].key ) {
                doMute();
            } else
            /* APM stuffs */
            if ( ev.message.keycode == (kbd.defCmds)[sleepKey].key ||
                 ev.message.keycode == (kbd.defCmds)[wakeupKey].key ) {
                sleepState(STANDBY);
            } else
            if ( ev.message.keycode == (kbd.defCmds)[powerDownKey].key ) {
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
