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

#include "kbddef.h"

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKBbells.h>

/* Function prototypes */
static void initialize(char* argv[]);
void usage(int argc, char* argv[]);
void showKbdList(int argc, char *argv[]);
static Bool setKbdType(const char* prog, const char* type);
void setCDROMDevice(char* optarg);
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


extern	Display *       dpy;
extern	int             xkbOpcode;
extern	int             xkbEventCode;

extern	XkbDescPtr      xkb;

extern  keyboard        kbd;

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

//#define SHAREDIR        "/usr/share/hotkeys/"
//#define SHAREDIR        "/home/hajime/debian/hotkeys/hotkeys-0.2"


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
 * courtesy of Phil Hagen <phil@identityvector.com>
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

/*
SK-2501A
Fran Firman <fran@ipdata.co.nz>

158     email
163     phone
146     toptools
162     3
178     www
164     4
151     ?
161     5
165     lock
166     info
160     mute
176     vol up
174     vol down
*/

/*
SK-2500 alias liteon-ak2500, Fujitsu, logitech, trust
Hendrik Muhs <Hendrik.Muhs@Student.Uni-Magdeburg.DE>
151     Close
165     system pause
152     eject cd
178     WWW
158     record
144     |<< (Track backward)
146     hear ("abh?ren")
162     > (Play)
164     |_| (Stop)
153     >>| (Track forward)
174     Vol -
176     Vol +
160     Mute
166     Display
250     "coffee", suspend or something
161     calculator
163      apply ("?bernehmen")

Keyboard producer:      Trust
Keyboard model:         Direct Access Keyboard SK 2500
Roberto Piscitello <robepisc@freemail.it>

Vulume up               0x10    176             100ms period
Volume down             0x10    174             100ms period
Mute                    0x10    160             absent                  Should mute the sound card, not just set volume=0
Menu/?                  0x10    166             absent                  Should start hotkeys' GUI (when it will exist:)
Eject                   0x10    152             absent
Prev. track (|<<)       0x10    144             600ms period    If kept down => rewind (<<)
Next track (>>|)        0x10    153             600ms period    If kept down => fast forward (>>)
Play/Pause (>/||)       0x10    162             absent
Stop ([])               0x10    164             absent
Record                  0x10    158             absent                  Could start grecord (GNOME Sound Recorder)
Rewind                  0x10    146             absent                  grecord stop+rewind+play (any better use?)
WWW                     0x10    178             absent Netscape, of course
Calculator              0x10    161             absent
Xfer                    0x10    163             absent                  Could launch Balsa or similar(***)
Close                   0x10    151             absent                  Could call the GNOME logout window
Coffee break            0x10    250             absent Screensaver(**)
Suspend                 0x10    165             absent                  APM   suspend
Cycle windows           look below              600ms period    Should cyclically port windows to the foreground

The "Cycle windows" key requires more description:
 - when pushed:         0x10    64      KeyPress Alt_L (keysym 0xffe9)
   (in sequence)        0x18    50      KeyPress Shift_L (keysym 0xffe1)
                        0x19    23      KeyPress + KeyRelease ISO_Left_Tab   
+(keysym 0xfe20)

 - if kept down:        0x19    23      KeyPress + KeyRelease ISO_Left_Tab
+(keysym 0xfe20)
                                                (repeated with 600ms period)

 - when released:       0x19    23      KeyPress + KeyRelease ISO_Left_Tab
+(keysym 0xfe20)
   (in sequence)        0x19    64      KeyRelease Alt_L (keysym 0xffe9)
                        0x11    50      Shift_L (keysym 0xffe1)
*/


/*
 logitech cordless desktop iTouch
 Petter Knudsen <petterkn@bukharin.hiof.no>

Email:236
Home:178
Search:229
Run:230
Next:153
Prev:144
Stop;164
Play/pause:162
Vol up:176
Vol down:174
Mute:160
Sleep: 223
*/

#endif /* __HOTKEYS_H */
