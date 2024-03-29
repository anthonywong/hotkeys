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

#ifndef __KBDDEF_H
#define	__KBDDEF_H

#include <X11/Xlib.h>

/* Function prototypes */
Bool readDefFile(const char* filename);
static Bool parseDefFile(const char* filebuf);
static void parseError(const char* filename);

extern	Display *	dpy;

extern  int         keytypes[255];

typedef enum {
    prevTrackKey,
    playKey,
    stopKey,
    pauseKey,
    nextTrackKey,
    browserKey,
    emailKey,
    helpKey,
    communitiesKey,     /* ???, in MS kbd */
    searchKey,
    ideasKey,           /* ???, in MS kbd */
    shoppingKey,
    printKey,
    goKey,
    recordKey,
    shellKey,           /* correspond to DOS key */
    transferKey,        /* ???, in MX3000 */
    myDocumentsKey,
    myComputerKey,
    favoritesKey,
    calculatorKey,
    newsReaderKey,
    iNewsKey,
    rewindKey,
    rotateKey,          /* Rotate windows???, in MX3000 */
    screensaverKey,
    TYPE_LAUNCH,        /*** Before this point, all these keys will launch
                             applications. After this point, the functions
                             are built into the program. ***/
    ejectKey,
    volUpKey,
    volDownKey,
    muteKey,
    wakeupKey,
    powerDownKey,
    sleepKey,
    suspendKey,
    NUM_PREDEF_HOTKEYS
} hotkey;

/* A struct for const data */
typedef struct {
    char*       name;   /* literal string of a hotkey functionality in definition file */
    hotkey      key;    /* numerical index of the functionality */
    KeySym      keysym; /* the corresponding keysym defined in XF86keysym.h */
} defEntry;

extern  const defEntry    defStr[];

typedef struct {
    int         keycode;
    char*       command;
    char*       desc;
    KeySym      keysym;
} hotkeyCmd;

typedef struct {
    char*       shortName;
    char*       longName;
    int         noOfKeys;
//    hotkey*     keycodes;
    defEntry*   defCmds;
    int         noOfCustomCmds;
    hotkeyCmd*  customCmds;
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

#endif /* __KBDDEF_H */
