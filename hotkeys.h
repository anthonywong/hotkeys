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

#ifndef __HOTKEYS_H
#define	__HOTKEYS_H

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKBbells.h>

#define VERSION "0.2"

/* Function prototypes */
void usage(int argc, char* argv[]);
void showKbdList(int argc, char *argv[]);
void setKbdType(char* prog, char* optarg);
void setCDROMDevice(char* optarg);
static Bool parseArgs(int argc, char* argv[]);
static Display* GetDisplay(char* program, char* dpyName, int* opcodeRtrn, int* evBaseRtrn);
void bailout(void);
int adjust_vol(int adj);
int doMute(void);
int ejectDisc(void);
int closeTray(void);
int playDisc(void);
int launchBrowser(void);
int launchMailer(void);
static void printXkbActionMessage(FILE* file,XkbEvent* xkbev);
void uError(char* s,...);
void uInfo(char* s,...);
void uInternalError(char* s,...);


extern	Display *	dpy;
extern	int		xkbOpcode;
extern	int		xkbEventCode;

extern	XkbDescPtr	xkb;

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


typedef enum {
    app_browser,
    app_mailer,
    app_calculator,
    app_xterm,
    app_filemanager,
    app1,
    app2,
    app3,
    app4,
    app5,
    NUM_APPS
} application;

typedef enum {
    trackBackBtn,
    playBtn,
    ejectBtn,
    stopBtn,
    trackNextBtn,
    volUpBtn,
    volDownBtn,
    muteBtn,
    browserBtn,
    emailBtn,
    helpBtn,
    wakeupBtn,
    powerDownBtn,
    communitiesBtn,     /* ???, in MS kbd */
    searchBtn,
    ideasBtn,           /* ???, in MS kbd */
    shoppingBtn,
    printBtn,
    goBtn,
    recordBtn,
    DOSBtn,
    transferBtn,        /* ???, in MX3000 */
    myDocumentsBtn,
    myComputerBtn,
    calculatorBtn,
    iNewsBtn,
    sleepBtn,
    rewindBtn,
    rotateBtn,          /* ???, in MX3000 */
    NUM_BUTTONS
} keycode;

typedef struct _keyboard {
    char*       shortName;
    char*       longName;
    int         noOfKeys;
    keycode*    keycodes;
} keyboard;

/* 
 * Keycodes of Microsoft Internet keyboard.
 *
 * courtesy of jas <atropa@picklepop.darktech.org> 
 *
Play/Pause              162
Track Repeat/Back       144
Track Skip/Next         153
Stop                    164
Eject                   152
Mail                    158
Communities             166
'Compaq'                165
Internet                163
Search                  161
Ideas                   146
Shopping                178
Print                   232
Go                      159
Mute                    160
Volume-                 174
Volume+                 176
*/

/* 
 * Keycodes of Memorex MX3000 keyboard
 *
 * courtesy of Jeffrey Panczyk <jpanczyk@is2.dal.ca>
 *
144  Back one song
146  (a picture of three files with an arrow)  on Windows it's function is Rotate...I don't know what it does.
147  Rec
148  Dos
151  Close CD
152  X*fer  (doesn't do anything under Windows)
153  forward one song
158  Eject CD  
159  My Documents
160  Mute
161  Calculator
162  Play/Pause
164  Stop
166  iNews
174  Volume Down
176  Volume Up
178  World Wide Web  normally/Wake Up if in Sleep mode
222  Power
223  Sleep
235  My Computer
236  e-Mail
237  Rew ->  (not sure what this does)
*/

/*
 * Keycodes of Silitek SK-7100 keyboard
 * (http://www.silitek.com/keyboards/sk-7100.htm)
 *
 * courtesy of Phil
 *
Keycode/Button Label
151     Close
165     CD
152     Video
178     WWW
158     U/P [PJH: No idea what this is for - never used 'doze software)
144     |<< (Track backward)
146     || (Pause)
162     > (Play)
164     |_| (Stop)
153     >>| (Track forward)
174     Vol -
176     Vol +
160     Mute
166     Display
*/

#endif /* __HOTKEYS_H */
