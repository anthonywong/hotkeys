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

const defEntry defStr[] = {
    { "PrevTrack",              prevTrackKey },
    { "Play",                   playKey },
    { "Eject",                  ejectKey },
    { "Stop",                   stopKey },
    { "Pause",                  pauseKey },
    { "NextTrack",              nextTrackKey },
    { "VolUp",                  volUpKey },
    { "VolDown",                volDownKey },
    { "Mute",                   muteKey },
    { "WebBrowser",             browserKey },
    { "Email",                  emailKey },
    { "Help",                   helpKey },
    { "WakeUp",                 wakeupKey },
    { "PowerDown",              powerDownKey },
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
    { "iNews",                  iNewsKey },
    { "Sleep",                  sleepKey },
    { "Suspend",                suspendKey },
    { "Rewind",                 rewindKey },
    { "Rotate",                 rotateKey },          /* ???, in MX3000 */
    { NULL,                     NUM_PREDEF_HOTKEYS }
};


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
        kbd.customCmds[kbd.noOfCustomCmds].description = xstrdup(tc);
        XFREE(tc);
    }
    else
    {
        /* No description given in definition file */
        kbd.customCmds[kbd.noOfCustomCmds].description = NULL;
    }

    kbd.noOfCustomCmds++;
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
                XFREE(tc);
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

/*
    FILE*   fp;
    int     noOfKeys = 0;
    char*   filebuf;
    int     filesize;
    int a;

    if ( (fp = fopen( filename, "r" )) == NULL )
    {
        return False;
    }

    if ( fseek( fp, 0, SEEK_END ) != 0 )
    {
        perror("haha");
    }
    filesize = ftell(fp);
    if ( (filebuf = (char*) malloc(filesize)) == NULL )
    {
        uError("Insufficient memory");
        exit(1);
    }
    rewind(fp);

    fread( filebuf, filesize, 1, fp );
    if (ferror(fp) != 0 )
    {
printf(" %d %d\n", filesize, a);
        uError("Error while reading file");
        exit(1);
    }

    if ( parseDefFile(filebuf) != 0 )
        parseError(filename);
*/
}


#if 0
static Bool
parseDefFile(const char* filebuf)
{
    regmatch_t  matches[10];
    regex_t     preg;
    int         nameLen;
    int         ret;
    char        readInFunc[32]; /* our function name won't be > 31 chars */

    /* Parse the Long name */
    ret = regcomp( &preg, "^[[:blank:]]*#[[:blank:]]*Name:[[:blank:]]*([^\n]+)[[:blank:]]*$", REG_EXTENDED|REG_NEWLINE );
    if ( errno != 0 )
    {
        printf("error: %d %s\n", errno, strerror(errno));
    }
    ret = regexec( &preg, filebuf, 2, matches, 0 );

    if ( ret == REG_NOMATCH || matches[0].rm_so == -1 )
        return -1;

printf("%d %d \n", matches[0].rm_so,matches[0].rm_eo);
    nameLen = matches[1].rm_eo - matches[1].rm_so;
    kbd.longName = (char*) malloc(nameLen + 1);
printf("a %d %d\n", matches[1].rm_so, nameLen);
    strncpy( kbd.longName, filebuf + matches[1].rm_so, nameLen );
    kbd.longName[nameLen] = '\0';   /* coz no NULL by strncpy */

    printf("---%s\n", kbd.longName);
    regfree(&preg);

    /* Allocate memory for the hotkey array */
    kbd.keycodes = (hotkey*) malloc( NUM_HOTKEYS * sizeof(hotkey) );
    if ( kbd.keycodes == NULL )
    {
        uError("Insufficient memory");  bailout();
    }

    /* Parse keycodes */
    ret = regcomp( &preg, "^[[:blank:]]*([0-9]+)[[:blank:]]+([^\n]+)[[:blank:]]*$", REG_EXTENDED|REG_NEWLINE );
    if ( errno != 0 )
    {
        printf("error: %d %s\n", errno, strerror(errno));
    }

    kbd.noOfKeys = 0;
/*
    while ( regexec( &preg, filebuf, 3, matches, 0 ) == 0 )
    {
        if ( matches[0].rm_so == -1 )
            return -1;

        kbd.noOfKeys++;
        readInFunc
        for ( i = 0; i < NUM_HOTKEYS; i++ )
        {
            if ( strcmp(
defStr
        kbd.keycodes[kbd.keycodes-1].
printf("%d %d \n", matches[0].rm_so,matches[0].rm_eo);
printf("a %d %d\n", matches[1].rm_so, nameLen);
        strncpy( kbd.longName, filebuf + matches[1].rm_so, nameLen );
        printf("***%s\n", kbd.longName);

        filebuf += matches[0].rm_eo;
    }
*/

    /*
            char*       shortName;
        char*       longName;
            int         noOfKeys;
                keycode*    keycodes;



    ret = regcomp( &preg, "^[ ]*#[ ]*Name:[ ]*([^\n]+)[ ]*$", REG_EXTENDED|REG_NEWLINE );
    ret = regexec( &preg, " #    Name:    M    $ Int  \n# Name: ABC", 5, matches, 0 );
    */

    regfree(&preg);
}

#endif

static void
parseError(const char* filename)
{
    uInfo("Error while parsing %s\n", filename);
    exit(1);
}
