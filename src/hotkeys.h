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
*/

#ifndef __HOTKEYS_H
#define	__HOTKEYS_H

#include "kbddef.h"

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKBbells.h>

/* Function prototypes */
static void initializeX(char* argv[]);
void usage(int argc, char* argv[]);
void showKbdList(int argc, char *argv[]);
static Bool setKbdType(const char* prog, const char* type);
void setCDROMDevice(char* optarg);
static void setLoglevel(int level);
static Bool parseArgs(int argc, char* argv[]);
static Display* GetDisplay(char* program, char* dpyName, int* opcodeRtrn, int* evBaseRtrn);
void bailout(void);
static int adjust_vol(int adj);
static int doMute(void);
static int ejectDisc(void);
static int closeTray(void);
static int playDisc(void);
static int launchApp(int type);
static void printXkbActionMessage(FILE* file,XkbEvent* xkbev);
void uError(char* s,...);
void uInfo(char* s,...);
void uInternalError(char* s,...);
Bool testReadable(const char* filename);

extern	Display *       dpy;
extern	int             xkbOpcode;
extern	int             xkbEventCode;
extern	XkbDescPtr      xkb;

extern  keyboard        kbd;
extern  int             loglevel;

#ifdef HAVE_XOSD
extern  xosd*           osd;
#endif

#ifdef DUMMY_MIXER
#define SOUND_IOCTL(a,b,c)      dummy_ioctl(a,b,c)
#else
#if defined (__NetBSD__) || defined (__OpenBSD__)
#define SOUND_IOCTL(a,b,c)      _oss_ioctl(a,b,c)
#else
#define SOUND_IOCTL(a,b,c)      ioctl(a,b,c)
#endif                          /* defined (__NetBSD__) || defined (__OpenBSD__) */
#endif                          /* DUMMY_MIXER */

#define MIXER_DEV       "/dev/mixer"
#define CDROM_DEV       "/dev/cdrom"
#define MAXLEVEL        100     /* highest level permitted by OSS drivers */

#define BROWSER         "mozilla"
#define BROWSER_ARGS    "mozilla"             /* 1st arg is the program name */
#define MAILER          "mozilla"
#define MAILER_ARGS     "mozilla -mail"    /* 1st arg is the program name */
#define CALCULATOR      "xcalc"
#define CALCULATOR_ARGS  "xcalc"
#define XTERM           "xterm"
#define XTERM_ARGS      "xterm"
#define FILEMANAGER     "gmc"
#define FILEMANAGER_ARGS "gmc"

#endif /* __HOTKEYS_H */
