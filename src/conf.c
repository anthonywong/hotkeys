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
#include <db.h>
#include <string.h>
#include <sys/param.h>

#include "conf.h"

char* conf_keys[] = {

    /* KEY             DEFAULT VALUE           */

    /* Specify the default keyboard to use */
    "Kbd",             "\0",

    /* CDROM device */
    "CDROM",           "/dev/cdrom",

    /* general actions */
    "PrevTrack",       "xmms --rew",
    "Play",            "xmms --play-pause",
    "Stop",            "xmms --stop",
    "Pause",           "xmms --pause",
    "NextTrack",       "xmms --fwd",
/*    "Rewind",          "\0",
 */

    "WebBrowser",      "mozilla",
    "Email",           "mozilla -mail",
    "Calculator",      "xcalc",
    "FileManager",     "gmc",
    "MyComputer",      "gmc",
    "MyDocuments",     "gmc",
    "Favorites",       "gnome-moz-remote --remote=openBookmarks",
    "Transfer",        "gftp", 
    "Record",          "grecord",
    "Shell",           "xterm -rv",
    "NewsReader",      "mozilla -news",
    "ScreenSaver",     "xscreensaver-command -activate",
    "Communities",     "mozilla -remote 'openURL(http://slashdot.org)'",
    "Search",          "mozilla -remote 'openURL(http://google.com)'",
    "Idea",            "mozilla -remote 'openURL(http://sourceforge.net)'",
    "Shopping",        "mozilla -remote 'openURL(http://thinkgeek.com)'",
    "Go",              "mozilla -remote 'openURL(http://linux.com)'",
    "Print",           "lpr",
/*
    "Screendump",      "xwd -root",
*/
    /* xosd stuffs */
//    "osd_font",        "-*-lucidatypewriter-bold-r-normal-*-*-250-*-*-*-*-*-*",
    "osd_font",        "lucidasanstypewriter-bold-24",
    "osd_color",       "LawnGreen",
    "osd_timeout",     "3",
    "osd_position",    "bottom",
    "osd_offset",      "25",
    NULL,              NULL
};

DB *         dbp;

/***====================================================================***/

char*
getConfig(char* key)
{
    DBT     k, data;

    /* Clear the structures as per db2's documentation
     * XXX: don't whether this is still needed in db3 */
    memset( &k, 0, sizeof(k) );
    memset( &data, 0, sizeof(data) );

    k.data = key;
    k.size = strlen(key) + 1;

    if ( dbp->get(dbp, NULL, &k, &data, 0) == 0 )
    {
        return data.data;
    }
    else
    {
        return NULL;
    }
}

int
setConfig(char* key, char* value, u_int32_t flags)
{
    DBT     k, data;

    /* Clear the structures as per db2's documentation */
    memset( &k, 0, sizeof(k) );
    memset( &data, 0, sizeof(data) );

    k.data = xstrdup(key);
    k.size = strlen(key) + 1;
    data.data = xstrdup(value);
    data.size = strlen(value) + 1;

    return dbp->put(dbp, NULL, &k, &data, flags);
}

static void
fillDefaults(void)
{
    char** key = conf_keys;
    int     err;

    do {
        err = setConfig(*key, *(key+1), DB_NOOVERWRITE);
        if ( err != 0 && err != DB_KEYEXIST )
        {
            uError("db: put: %s", strerror(errno));
            bailout();
        }
        key += 2;
    } while ( *key != NULL );
}

static int
getKey(FILE* fp, char* key)
{
    int     c;
    int     idx=0;

    while ( (c=fgetc(fp)) != '=' )
    {
        if (c == EOF)
            return -1;
        else if ( idx == 0 )
        {
            if ( c == '#' )
            {
                /* this line is a comment, so we read until the newline */
                while ( (c=fgetc(fp)) != '\n' )
                {
                    if (c == EOF)
                        return -1;
                }
                return 1;
            }
            else if ( c == '\n' || c == ' ' )   /* blank line or empty spaces */
            {
                continue;
            }
        }
        key[idx] = (char)c;
        if ( ++idx == MAX_KEY_LEN-1 )
            break;
    }
    key[idx] = '\0';
    return 0;
}


static int
getValue(FILE* fp, char* value)
{
    int     c;
    int     idx=0;

    while ( (c=fgetc(fp)) != '\n' )
    {
        if (c == EOF)
            return -1;
        else
        {
            value[idx] = (char)c;
            if ( ++idx == MAX_VALUE_LEN-1 )
                break;
        }
    }
    value[idx] = '\0';
    return 0;
}


static void
parseConfigFile(char* filename)
{
    FILE*   fp;
    char    key[MAX_KEY_LEN];
    char    keyvalue[MAX_VALUE_LEN];
    int     r;

    if ( (fp = fopen( filename, "r" )) != NULL )
    {
        while ( !feof(fp) )
        {
            r = getKey(fp, key);
            if ( r == -1 )      /* EOF, etc */
                break;
            else if ( r == 1 )  /* comment */
                continue;

            if ( getValue(fp, keyvalue) == -1 )
                break;

            if ( setConfig( key, keyvalue, 0 ) != 0 )
            {
                uError("db: put: %s", strerror(errno));
                bailout();
            }
        }

        fclose(fp);
    }
}


void
readConfigFile(void)
{
    DB_ENV* dbenv = NULL;
    int ret;

    char*   h;
    char    filename[MAXPATHLEN];

    /* Create the hash table */
    if ( (ret = db_create(&dbp, dbenv, 0)) != 0 )
    {
        uError("Failed in db_create: %d", ret);
        bailout();
    }

    if ( (ret = dbp->open(dbp, NULL, NULL, DB_HASH, DB_CREATE, 0664)) != 0 )
    {
        uError("Can't create hash table: %d", ret);
        bailout();
    }

    fillDefaults();

    /* parse the global config first */
    strncpy( filename, CONFDIR, MAXPATHLEN-strlen(CONFIG_NAME)-2 );
    strcat( filename, "/" );
    strcat( filename, CONFIG_NAME );
    if ( testReadable(filename) )
        parseConfigFile(filename);

    /* See whether the user has his own config file */
    if ( (h = getenv("HOME")) != NULL )
    {
        strncpy( filename, h, MAXPATHLEN-2-strlen(PACKAGE)-1-strlen(CONFIG_NAME)-1 );
        strcat( filename, "/." PACKAGE "/" );
        strcat( filename, CONFIG_NAME );
        if ( testReadable(filename) )
            parseConfigFile(filename);
    }
}
