/* $Id$ */

#ifndef __KBDDEF_H
#define	__KBDDEF_H

#include <X11/Xlib.h>

/* Function prototypes */
Bool readDefFile(const char* filename);
static Bool parseDefFile(const char* filebuf);
static void parseError(const char* filename);

extern	Display *	dpy;

#define KBD_DEF_DIR     "/usr/share/hotkeys"

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
    prevTrackKey,
    playKey,
    ejectKey,
    stopKey,
    pauseKey,
    nextTrackKey,
    volUpKey,
    volDownKey,
    muteKey,
    browserKey,
    emailKey,
    helpKey,
    wakeupKey,
    powerDownKey,
    communitiesKey,     /* ???, in MS kbd */
    searchKey,
    ideasKey,           /* ???, in MS kbd */
    shoppingKey,
    printKey,
    goKey,
    recordKey,
    DOSKey,
    transferKey,        /* ???, in MX3000 */
    myDocumentsKey,
    myComputerKey,
    calculatorKey,
    iNewsKey,
    sleepKey,
    suspendKey,
    rewindKey,
    rotateKey,          /* ???, in MX3000 */
    NUM_PREDEF_HOTKEYS
} hotkey;

typedef struct {
    char*       name;   /* literal string of a hotkey functionality in definition file */
    hotkey      key;    /* numerical index of the functionality */
} defEntry;

typedef struct {
    int         keycode;
    char*       command;
    char*       description;
} hotkeyCmd;

typedef struct {
    char*       shortName;
    char*       longName;
    int         noOfKeys;
    hotkey*     keycodes;
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
