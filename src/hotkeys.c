/*
    HOTKEYS - use keys on your multimedia keyboard to control your computer
    Copyright (C) 2000  Anthony Y P Wong <ypwong@ypwong.org>

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

#include <sys/types.h>
#include <sys/stat.h>

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
    "web browser",
    "email reader",
    "calculator",
    "Xterm",
    "file manager",
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

unsigned long   eventMask     = 0;

Bool            synch         = False;
int	            verbose       = 0;
Bool            background    = False;

XkbDescPtr      xkb           = NULL;

FILE *          errorFile     = NULL;

char *          progname      = NULL;

char *          cdromDevice   = CDROM_DEV;

keyboard        kbd;                    /* the keyboard the user is using */

/***====================================================================***/

void
usage(int argc, char *argv[])
{
    printf("HOTKEYS v%s -- use the hotkeys on your Internet/multimedia keyboard to control your computer\n", VERSION);
    printf("Usage: %s [options...]\n", argv[0]);
    printf("Legal options:\n");
#ifdef HAVE_GETOPT_LONG
    printf("\t-t, --type               Specify the keyboard type (refer to -l)\n");
    printf("\t-d, --cdrom-dev          Specify the CDROM/DVDROM device\n");
    printf("\t-b, --background         Run in background\n");
    printf("\t-l, --kbd-list           Show all supported keyboards\n");
    printf("\t-h, --help               Print this message\n");
//    M("-cfg <file>          Specify a config file\n");
//    M("-d[isplay] <dpy>     Specify the display to watch\n");
//    M("-v                   Print verbose messages\n");
#else
    printf("\t-t   Specify the keyboard type (refer to -l)\n");
    printf("\t-d   Specify the CDROM/DVDROM device\n");
    printf("\t-l   Show all supported keyboards\n");
    printf("\t-b   Run in background\n");
    printf("\t-h   Print this message\n");
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

#ifdef HAVE_GETOPT_LONG
    printf("Supported keyboards: (with corresponding options to --kbd-list or -l)\n");
#else
    printf("Supported keyboards: (with corresponding options to -l)\n");
#endif

    /* Read the definition files in $HOME/.hotkeys */
    h = getenv("HOME");
    if ( h != NULL )
    {
        homedir = XMALLOC( char, strlen(h) + strlen("/.hotkeys") + 1 );
        strcpy( homedir, h );
        strcat( homedir, "/.hotkeys" );
        if ( ( dir = opendir(homedir) ) != NULL )
        {
            while ( ent = readdir(dir) )
            {
                /* Show all files that ends with ".def" */
                if ( NLENGTH( ent ) > 4 &&
                     strstr( ent->d_name, ".def" ) != NULL &&
                     strlen(strstr( ent->d_name, ".def" )) == 4 ) /* XXX Not portable, but how to fix?? */
                {
                    ent->d_name[ strlen(ent->d_name)-4 ] = '\0';
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
            /* Show all files that ends with ".def" in SHAREDIR */
            if ( NLENGTH( ent ) > 4 &&
                 strstr( ent->d_name, ".def" ) != NULL &&
                 strlen(strstr( ent->d_name, ".def" )) == 4 ) /* XXX Not portable, but how to fix?? */
            {
                ent->d_name[ strlen(ent->d_name)-4 ] = '\0';
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
    struct stat t;
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

    if ( stat( defname, &t ) == 0 )     /* if the file exists... */
    {
        readDefFile( defname );
    }
    else
    {
        /* Now try if it exists in $HOME/.hotkeys or not */
        h = getenv("HOME");
        if ( h != NULL )
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
            if ( stat( defname, &t ) == 0 )     /* if the file exists... */
            {
                readDefFile( defname );
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

void
setCDROMDevice(char* optarg)
{
    /* Option is --cdrom-dev or -d */
    int fd = open( optarg, O_RDONLY|O_NONBLOCK );
    if (fd == -1) {
        uInfo("Unable to open `%s', fall back to %s\n", cdromDevice, CDROM_DEV);
    } else {
        if ( ( cdromDevice = (char*) xstrdup(optarg) ) == NULL ) {
            uError("Insufficient memory");
            bailout();
        }
    }
    close (fd);
}

/***====================================================================***/

static Bool
parseArgs(int argc, char *argv[])
{
    int     c, i;
    int     digit_optind = 0;
    Bool    kbdSet = False;

    const char *flags = "hbt:d:lz:";
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

    mjr= XkbMajorVersion;
    mnr= XkbMinorVersion;
    dpy= XkbOpenDisplay(dpyName,evBaseRtrn,NULL,&mjr,&mnr,&error);
    if (dpy==NULL) {
	switch (error) {
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
//    uAction("Exiting\n");
    if ( dpy != NULL )
	XCloseDisplay(dpy);
    uInfo("Bailing out...\n");
    exit(1);
}

static int
adjust_vol(int adj)
{
    int mixer_fd, cdrom_fd;
    int master_vol, cd_vol;
    struct cdrom_volctrl cdrom_vol;
    int left, right;

    int ret = 0;

    /* open the mixer device */
    if ( (mixer_fd = open( MIXER_DEV, O_RDWR )) == -1 )
    {
        uError("Unable to open `%s'", MIXER_DEV);
        return -1;
    }
    /* open the cdrom/dvdrom drive device */
    if ( (cdrom_fd = open( cdromDevice, O_RDONLY|O_NONBLOCK )) == -1 )
    {
        uError("Unable to open `%s'", cdromDevice);
        return -1;
    }

    if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_VOLUME, &master_vol) == -1)
    {
        uError("Unable to read the volume of `%s'", MIXER_DEV);
        ret = -1; goto LEAVE; 
    }
    if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_CD, &cd_vol) == -1)
    {
        uError("Unable to read the CD volume of `%s'", MIXER_DEV);
        ret = -1; goto LEAVE; 
    }

    /* read the cdrom volume */
    if ( ioctl(cdrom_fd, CDROMVOLREAD, &cdrom_vol) == -1 )
    {
        uError("Unable to read the CDROM volume of `%s'", cdromDevice);
        ret = -1; goto LEAVE; 
    }

    /* Set the master volume */
    left = (master_vol & 0xFF) + adj;
    right = ((master_vol >> 8) & 0xFF) + adj;
    left = (left>MAXLEVEL) ? MAXLEVEL : ((left<0) ? 0 : left);
    right = (right>MAXLEVEL) ? MAXLEVEL : ((right<0) ? 0 : right);
    master_vol = left + (right << 8);

    if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &master_vol) == -1)
    {
        uError("Unable to set the master volume");
        ret = -1; goto LEAVE;
    }

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

    /* Set the CDROM volume */
    cdrom_vol.channel0 += adj; cdrom_vol.channel1 += adj;
    cdrom_vol.channel2 += adj; cdrom_vol.channel3 += adj;
    if ( ioctl(cdrom_fd, CDROMVOLCTRL, &cdrom_vol) == -1 )
    {
        uError("Unable to set the volume of %s", cdromDevice);
        ret = -1;
    }

LEAVE:
    close(mixer_fd);
    close(cdrom_fd);

    return ret;
}


/*
 *  Mute or un-mute the /dev/mixer master volume and CDROM drive's
 *  volume
 */
static int
doMute(void)
{
    static Bool muted = False;
    static int last_mixer_vol, last_cd_vol;
    static struct cdrom_volctrl last_cdrom_vol;

    int vol, cd_vol;
    struct cdrom_volctrl cdrom_vol;
    int mixer_fd, cdrom_fd;

    short ret = 0;      /* return value */

    /* open the mixer device */
    if ( (mixer_fd = open( MIXER_DEV, O_RDWR|O_NONBLOCK )) == -1 )
    {
        uError("Unable to open `%s'", MIXER_DEV);
        return -1;
    }
    /* open the cdrom/dvdrom drive device */
    if ( (cdrom_fd = open( cdromDevice, O_RDONLY|O_NONBLOCK )) == -1 )
    {
        uError("Unable to open `%s'", cdromDevice);
        return -1;
    }

    if ( muted == True )
    {
        /* Un-mute them */
        if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &last_mixer_vol) == -1)
        {
            uError("Unable to un-mute the mixer");
            ret = -1;
        } else
            muted = False;
        if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_CD, &last_cd_vol) == -1)
        {
            uError("Unable to un-mute the CD volume");
            ret = -1;
        } else
            muted = False;
        if ( ioctl(cdrom_fd, CDROMVOLCTRL, &last_cdrom_vol) == -1 )
        {
            uError("Unable to un-mute `%s'", cdromDevice);
            ret = -1;
        } else
            muted = False;
    }
    else
    {
        /* Read and store the mixer volume, do not try to mute them
         * if we cannot read any of their values. */
        if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_VOLUME, &last_mixer_vol) == -1)
        {
            uError("Unable to read the mixer volume of `%s'", MIXER_DEV);
            ret = -1; goto LEAVE2;
        }
        if ( SOUND_IOCTL(mixer_fd, SOUND_MIXER_READ_CD, &last_cd_vol) == -1)
        {
            uError("Unable to read the CD volume of `%s'", MIXER_DEV);
            ret = -1; goto LEAVE2;
        }
        /* read and store the cdrom volume */
        if ( ioctl(cdrom_fd, CDROMVOLREAD, &last_cdrom_vol) == -1 )
        {
            uError("Unable to read the CDROM volume of `%s'", cdromDevice);
            ret = -1; goto LEAVE2;
        }

        /* Mute them! */
        vol = 0;
        if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &vol) == -1)
        {
            uError("Unable to mute mixer volume of `%s'", MIXER_DEV);
            ret = -1;
        } else
            muted = True;
        if (SOUND_IOCTL(mixer_fd, SOUND_MIXER_WRITE_CD, &vol) == -1)
        {
            uError("Unable to mute CD volume of `%s'", MIXER_DEV);
            ret = -1;
        } else
            muted = True;

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

LEAVE2:
    close(mixer_fd);
    close(cdrom_fd);

    return ret;
}

static int
ejectDisc(void)
{
    static Bool ejected = False;

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
        int fd = open(cdromDevice, O_RDONLY|O_NONBLOCK);
        if (fd == -1)
        {
            uError("Unable to open `%s'", cdromDevice);
            bailout();
            return -1;
        }
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
    int fd = open(cdromDevice, O_RDONLY|O_NONBLOCK);
    if (fd == -1)
    {
        uInfo("unable to open `%s'\n", cdromDevice);
        bailout();
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

//        char* args[] = BROWSER_ARGS;
//        if ( execvp(applications[type], application_args[type]) == -1 ) {
        if ( execvp(applications[type], arg_array) == -1 ) {
            uError("Cannot launch the %s", app_strings[type]);
        }

        XFREE(cc);
        XFREE(arg_array);
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
            int pid = fork();

            if ( pid == -1 )
            {
                uInfo("Cannot launch \"%s\"\n", kbd.customCmds[i].description);
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

                if ( execvp(arg_array[0], arg_array) == -1 ) {
                    uError("Cannot launch \"%s\"", kbd.customCmds[i].description);
                }

                XFREE(cc);
                XFREE(arg_array);
            }
            break;  /* break the for loop */
        }
    }
}

/***====================================================================***/

static void
printXkbActionMessage(FILE* file,XkbEvent* xkbev)
{
    XkbActionMessageEvent *msg= &xkbev->message;
    fprintf(file,"    message: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                                        msg->message[0],msg->message[1],
                                        msg->message[2],msg->message[3],
                                        msg->message[4],msg->message[5]);
    fprintf(file,"    key %d, event: %s,  follows: %s\n",msg->keycode,
                                     (msg->press?"press":"release"),
                                     (msg->key_event_follows?"yes":"no"));
    return;
}


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

/***====================================================================***/

static void
initialize(char* argv[])
{
    KeySym              newKS;
    XkbMessageAction    xma;
    XkbMapChangesRec    mapChangeRec;
    int                 types[1];
    int                 i;

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
//    xkb= XkbGetKeyboard(dpy,XkbGBN_AllComponentsMask,XkbUseCoreKbd);
//    xkb= XkbGetKeyboard(dpy,XkbAllComponentsMask,XkbUseCoreKbd);
    xkb = XkbGetMap(dpy, XkbAllMapComponentsMask, XkbUseCoreKbd);
    if (!xkb)
    {
        uInfo("XkbGetMap failed\n"); bailout();
    }

    /* Construct the Message Action struct */
    xma.type = XkbSA_ActionMessage;
    xma.flags = XkbSA_MessageOnPress;
    strcpy(xma.message," ");

#ifdef DEBUG
printf("num_acts:%d size_acts:%d\n", xkb->server->num_acts, xkb->server->size_acts);
printf("idx:%d has action:%d no.:%d noOfGrps:%d\n", xkb->server->key_acts[152], XkbKeyHasActions(xkb,152), XkbKeyNumActions(xkb,152), XkbKeyNumGroups(xkb,152));
#endif

    /* Add KeySym to the key codes, as they don't have any KeySyms before */
    for ( i = 0; i < NUM_PREDEF_HOTKEYS; i++ )
    {
        int code = (kbd.keycodes)[i];
        if ( code == 0 )
            continue;

        /* Assign a group to the key code */
//        types[XkbGroup1Index] = XkbKeyTypeIndex( xkb, code, XkbGroup1Index );
        types[0] = XkbOneLevelIndex;
        XkbChangeTypesOfKey(xkb, code, 1, XkbGroup1Mask, types, NULL);

        /* Change their Keysyms */
        if ( XkbResizeKeySyms( xkb, code, 1) == NULL )
        {
            uInfo("resize keysym failed\n"); bailout();
        }
        /* Add one key action to it */
        if ( XkbResizeKeyActions( xkb, code, 1) == NULL )
        {
            uInfo("resize key action failed\n"); bailout();
        }

        /* Assign a new keysym to the key code */
        newKS = 0x2200+i;       /* FIXME: I just choose a keysym that's not yet assigned */
//        newKS = 0xFFE0;
        *XkbKeySymsPtr(xkb,code) = newKS;

        /* Send the change back to the server */
        /* XkbKeyActionsMask must be here, just XkbKeySymsMask|XkbKeyTypesMask
         * is not sufficient */
        if ( XkbSetMap(dpy, XkbKeySymsMask|XkbKeyActionsMask|XkbKeyTypesMask, xkb) )
        {
#ifdef DEBUG
            printf("map set done\n");
#endif
        }
        else
        {
            uInfo("map set failed\n"); bailout();
        }


#ifdef DEBUG
        printf("%d: before: %d\n",code, XkbKeyActionsPtr(xkb,code)[0].type);
#endif
        /* Assign the Message Action to the key code */
        (&(xkb->server->acts[ xkb->server->key_acts[code] ]))[0] = (XkbAction) xma;

#ifdef DEBUG
        printf("after: %d\n",XkbKeyActionsPtr(xkb,code)[0].type);
#endif
    }

    /***************************************************************/

#if 0
    /* Send the change back to the server */
    if ( XkbSetMap(dpy, XkbAllMapComponentsMask, xkb) )
    {
#ifdef DEBUG
        printf("map set done\n");
#endif
    }
    else
    {
        uError("map set failed\n"); bailout();
    }
#endif
#if 0
    /* Commit the change back to the server */
    bzero(&mapChangeRec, sizeof(mapChangeRec));
    mapChangeRec.changed = XkbKeySymsMask | XkbKeyTypesMask;
    mapChangeRec.first_key_sym = xkb->min_key_code; /* FIXME, the value is from experimentation */
    mapChangeRec.num_key_syms = XkbNumKeys(xkb);  /* FIXME, the value is from experimentation  */
    mapChangeRec.first_key_act = xkb->min_key_code; /* FIXME, the value is from experimentation */
    mapChangeRec.num_key_acts = XkbNumKeys(xkb);  /* FIXME, the value is from experimentation  */
    mapChangeRec.first_type = 0;
    mapChangeRec.num_types = xkb->map->num_types;
    if ( XkbChangeMap(dpy, xkb, &mapChangeRec) )
    {
#ifdef DEBUG
        printf("map changed done\n");
#endif
    }
    else
    {
        uError("map changed failed\n"); bailout();
    }
#endif

    /* Commit the change back to the server. This is necessary! */
    if ( XkbSetMap(dpy, XkbKeyActionsMask, xkb) )
    {
#ifdef DEBUG
        printf("map set done\n");
#endif
    }
    else
    {
        uInfo("map set failed\n"); bailout();
    }
#if 0
    /* Commit the change back to the server */
    bzero(&mapChangeRec, sizeof(mapChangeRec));
    mapChangeRec.changed = XkbKeyActionsMask;
    mapChangeRec.first_key_act = 140; /* FIXME, the value is from experimentation */
    mapChangeRec.num_key_acts = 100;  /* FIXME, the value is from experimentation  */
    if ( XkbChangeMap(dpy, xkb, &mapChangeRec) )
    {
#ifdef DEBUG
        printf("map changed done\n");
#endif
    }
    else
    {
        uError("map changed failed\n"); bailout();
    }
#endif

#if 0
/* _test_ */
if ( xkb=XkbGetMap(dpy,XkbAllMapComponentsMask,XkbUseCoreKbd) )
    printf("map get done\n");
else {
    printf("map get failed\n"); exit(0);
}

printf("after: %d\n",XkbKeyActionsPtr(xkb,152)[0].type);
printf("idx:%d has action:%d no.:%d noOfGrps:%d\n", xkb->server->key_acts[38], XkbKeyHasActions(xkb,38), XkbKeyNumActions(xkb,38), XkbKeyNumGroups(xkb,38));
printf("idx:%d has action:%d no.:%d noOfGrps:%d\n", xkb->server->key_acts[152], XkbKeyHasActions(xkb,152), XkbKeyNumActions(xkb,152), XkbKeyNumGroups(xkb,152));

printf("num_acts:%d size_acts:%d\n", xkb->server->num_acts, xkb->server->size_acts);
/*
XkbSelectEvents(dpy,XkbUseCoreKbd,XkbAllEventsMask,XkbAllEventsMask);
*/
#endif
}


int
main(int argc, char *argv[])
{
    XkbEvent	        ev;

    int                 i, k;

    errorFile = stderr;

    /* initialize the kbd variable */
    kbd.noOfCustomCmds = 0;
    kbd.keycodes = (hotkey*) xcalloc( NUM_PREDEF_HOTKEYS, sizeof(hotkey) );

    if ( !parseArgs(argc,argv) )
        bailout();

    if (background)
    {
        if ( fork() !=0 )
        {
            if (verbose) 
                uInfo("Running in the background\n");
            exit(0);
        }
    }

    initialize(argv);

    /* Process the events in an forever loop */
    while (1) {
	XNextEvent( dpy, &ev.core );
#ifdef DEBUG
        printXkbActionMessage( stdout, &ev );
#endif
        if ( ev.type == xkbEventCode+XkbEventCode &&
             ev.any.xkb_type == XkbActionMessage )
        {
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
            } else
                /* User-defined stuffs */
                lookupUserCmd(ev.message.keycode);
        }
    }

    XCloseDisplay(dpy);
    return 0;
}

