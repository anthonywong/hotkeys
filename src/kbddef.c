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

#if HAVE_CONFIG_H
#  include <config.h>
#endif
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <errno.h>

#include <X11/XF86keysym.h>

#include <xmlmemory.h>
#include <parser.h>

#include "hotkeys.h"
#include "kbddef.h"

#define MatchName(n,s) ((strncasecmp(n->name,s,strlen(s))==0))

/***====================================================================***/

/* Make sure the order of the entries are the same as the hotkey enum
 * in kbddef.h, to make life easier... */
const defEntry defStr[] = {
    { "PrevTrack",              prevTrackKey,       XF86XK_AudioPrev },
    { "Play",                   playKey,            XF86XK_AudioPlay },
    { "Stop",                   stopKey,            XF86XK_AudioStop },
    { "Pause",                  pauseKey,           XF86XK_AudioPause },
    { "NextTrack",              nextTrackKey,       XF86XK_AudioNext },
    { "WebBrowser",             browserKey,         XF86XK_HomePage },
    { "Email",                  emailKey,           XF86XK_Mail },
    { "Help",                   helpKey,            0 },
    { "Communities",            communitiesKey,     0 },     /* ???, in MS kbd */
    { "Search",                 searchKey,          XF86XK_Search },
    { "Idea",                   ideasKey,           XF86XK_LightBulb },     /* ???, in MS kbd */
    { "Shopping",               shoppingKey,        XF86XK_Shop },
    { "Print",                  printKey,           0 },
    { "Go",                     goKey,              0 },     /* ??? */
    { "Record",                 recordKey,          XF86XK_AudioRecord },
    { "DOS",                    DOSKey,             0 },
    { "Transfer",               transferKey,        0 },     /* ???, in MX3000 */
    { "MyDocuments",            myDocumentsKey,     0 },
    { "MyComputer",             myComputerKey,      XF86XK_MyComputer },
    { "Calculator",             calculatorKey,      XF86XK_Calculator },
    { "NewsReader",             newsReaderKey,      0 },
    { "iNews",                  iNewsKey,           0 },
    { "Rewind",                 rewindKey,          0 },
    { "Rotate",                 rotateKey,          0 },     /* ???, in MX3000 */
    { "DUMMY_TYPE_LAUNCH",      TYPE_LAUNCH,        0xFFFFFFFF },
    { "Eject",                  ejectKey,           0 },
    { "VolUp",                  volUpKey,           XF86XK_AudioRaiseVolume },
    { "VolDown",                volDownKey,         XF86XK_AudioLowerVolume },
    { "Mute",                   muteKey,            XF86XK_AudioMute },
    { "WakeUp",                 wakeupKey,          XF86XK_WakeUp },
    { "PowerDown",              powerDownKey,       XF86XK_PowerOff },
    { "Sleep",                  sleepKey,           XF86XK_Standby },   /* either this is the Standby key or the next one... */
    { "Suspend",                suspendKey,         0 },
    { NULL,                     NUM_PREDEF_HOTKEYS, 0xFFFFFFFF }
};

int keytypes[255];  /* to note whether a keycode is used to launch an
                       application or not, indexed by keycode */

/***====================================================================***/

static void
parseUserDef(xmlDocPtr doc, xmlNodePtr cur, KeySym curKeySym)
{
    hotkeyCmd*  t;
    char*       t_keycode;
    char*       t_command;
    char*       tc;

    t_keycode = xmlGetProp( cur, "keycode" );
    if ( t_keycode == NULL ) {
        uInfo("keycode not found, entry ignored\n");
        return;
    }

    t_command = xmlGetProp( cur, "command" );
    if ( t_command == NULL ) {
        uInfo("command not found, entry ignored");
        return;
    }

    /* Allocate or enlarge the memory, depending on whether memory has
     * been previously allocated to kbd.customCmds. */
    t = XREALLOC( hotkeyCmd, kbd.customCmds, kbd.noOfCustomCmds+1 );

    kbd.customCmds = t;

    /* Assign the keycode and command to it */
    kbd.customCmds[kbd.noOfCustomCmds].keycode = atoi(t_keycode);
    XFREE(t_keycode);
    kbd.customCmds[kbd.noOfCustomCmds].command = xstrdup(t_command);
    XFREE(t_command);
    kbd.customCmds[kbd.noOfCustomCmds].keysym = curKeySym;

    tc = xmlNodeListGetString( doc, cur->xmlChildrenNode, 1 );
    if ( tc != NULL && tc[0] != '\0' )
    {
        kbd.customCmds[kbd.noOfCustomCmds].desc= xstrdup(tc);
        XFREE(tc);
    }
    else
    {
        /* No description given in definition file */
        kbd.customCmds[kbd.noOfCustomCmds].desc= NULL;
    }

    kbd.noOfCustomCmds++;
}

/*
 *  upOrDown - 1 for up, -1 for down
 */
static void
getVolAdj(int upOrDown, xmlNodePtr cur)
{
    char* tc;

    tc = xmlGetProp( cur, "adj" );
    if ( tc != NULL )
    {
        int v = atoi(tc);
        if ( v > 0 && v < 100 )
        {
            if ( upOrDown == 1 )
                volUpAdj = v;
            else if ( upOrDown == -1 )
                volDownAdj = -v;
        }
    }
}


static void
parseStd(xmlDocPtr doc, xmlNodePtr cur)
{
    char*   tc;
    int     i = 0;

    do {
        /* if the name in definition file matches one of the
         * predefined name */
        if ( MatchName( cur, defStr[i].name ) )
        {
            tc = xmlGetProp( cur, "keycode" );
            if ( tc == NULL )
            {
                uInfo("keycode not found, entry ignored\n");
                return;
            }
            else
            {
                kbd.defCmds[defStr[i].key].key = atoi(tc);
                kbd.defCmds[defStr[i].key].keysym = defStr[i].keysym;
                keytypes[atoi(tc)] = ( defStr[i].key < TYPE_LAUNCH ? 1 : 0 );
                XFREE(tc);

                /* Get the volume adjustment if the tag is VolUp or VolDown */
                if ( strncmp( defStr[i].name, "VolUp", 5 ) == 0 )
                    getVolAdj( 1, cur );
                else if ( strncmp( defStr[i].name, "VolDown", 7 ) == 0 )
                    getVolAdj( -1, cur );

                return;     /* leave as nothing to be done */
            }       
        }
        i++;
    } while ( defStr[i].name != NULL );

    /* No predefined key name is matched */
    uInfo("The key command \"%s\" is invalid\n", cur->name);
}


Bool
readDefFile(const char* filename)
{
    xmlDocPtr   doc;
    xmlNsPtr    ns;
    xmlNodePtr  cur;
    char*       tc;
    KeySym      curKeySym = 0x1008FFA0; /* The keysym assigned to custom commands.
                                           According to XF86keysym.h, keysyms from
                                           0x1008FFA) is not yet assigned (at
                                           2000/3/14) */

    if ( (doc = xmlParseFile(filename)) == NULL )
        return False;

    if ( (cur = xmlDocGetRootElement(doc)) == NULL )
    {
        uInfo("File %s is empty, skipping it...\n", filename);
        xmlFreeDoc(doc);
        return False;
    }

    /* Start parsing the XML file */

    /* Get the model name */
    tc = xmlGetProp( cur, "model" );
    if ( tc == NULL )
    {
        uInfo("Model name missing in %s, skipping it...\n", filename);
        xmlFreeDoc(doc);
        return False;
    }
    else
    {
        kbd.longName = xstrdup(tc);
    }

    cur = cur->xmlChildrenNode;
    while ( cur != NULL )
    {
        if ( MatchName( cur, "comment" ) )
        {
            cur = cur->next;
            continue;   /* A comment! Continue to the next one */
        }
        else if ( MatchName( cur, "userdef" ) )
        {
            parseUserDef( doc, cur, curKeySym );
            curKeySym++;
        }
        else if ( ! MatchName( cur, "text" ) )
        {
            parseStd( doc, cur );
        }

        cur = cur->next;
    }

    xmlFreeDoc(doc);
    return True;
}

static void
parseError(const char* filename)
{
    uInfo("Error while parsing %s\n", filename);
    exit(1);
}
