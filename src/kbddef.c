/* $Id$ */

#if HAVE_CONFIG_H
#  include <config.h>
#endif
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <errno.h>

#include <xmlmemory.h>
#include <parser.h>

#include "hotkeys.h"
#include "kbddef.h"

#define MatchName(n,s) ((strncasecmp(n->name,s,strlen(s))==0))

/***====================================================================***/

/* Make sure the order of the entries are the same as the hotkey enum,
   to make life easier... */
const defEntry defStr[] = {
    { "PrevTrack",              prevTrackKey },
    { "Play",                   playKey },
    { "Stop",                   stopKey },
    { "Pause",                  pauseKey },
    { "NextTrack",              nextTrackKey },
    { "WebBrowser",             browserKey },
    { "Email",                  emailKey },
    { "Help",                   helpKey },
    { "Communities",            communitiesKey },     /* ???, in MS kbd */
    { "Search",                 searchKey },
    { "Idea",                   ideasKey },           /* ???, in MS kbd */
    { "Shopping",               shoppingKey },
    { "Print",                  printKey },
    { "Go",                     goKey },
    { "Record",                 recordKey },
    { "DOS",                    DOSKey },
    { "Transfer",               transferKey },        /* ???, in MX3000 */
    { "MyDocuments",            myDocumentsKey },
    { "MyComputer",             myComputerKey },
    { "Calculator",             calculatorKey },
    { "NewsReader",             newsReaderKey },
    { "iNews",                  iNewsKey },
    { "Rewind",                 rewindKey },
    { "Rotate",                 rotateKey },          /* ???, in MX3000 */
    { "DUMMY_TYPE_LAUNCH",      TYPE_LAUNCH },
    { "Eject",                  ejectKey },
    { "VolUp",                  volUpKey },
    { "VolDown",                volDownKey },
    { "Mute",                   muteKey },
    { "WakeUp",                 wakeupKey },
    { "PowerDown",              powerDownKey },
    { "Sleep",                  sleepKey },
    { "Suspend",                suspendKey },
    { NULL,                     NUM_PREDEF_HOTKEYS }
};

int keytypes[255];  /* to note whether a keycode is used to launch an
                       application or not, indexed by keycode */

/***====================================================================***/

static void
parseUserDef(xmlDocPtr doc, xmlNodePtr cur)
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

    /* Assign it */
    kbd.customCmds[kbd.noOfCustomCmds].keycode = atoi(t_keycode);
    XFREE(t_keycode);
    kbd.customCmds[kbd.noOfCustomCmds].command = xstrdup(t_command);
    XFREE(t_command);

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
                kbd.keycodes[defStr[i].key] = atoi(tc);
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
            parseUserDef( doc, cur );
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
