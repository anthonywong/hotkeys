/* $Id$ */

#if HAVE_CONFIG_H
#  include <config.h>
#endif
#include "common.h"

#include <stdio.h>
#include <db2.h>
#include <string.h>
#include <sys/param.h>

#include "conf.h"

char* conf_keys[] = {

    /* KEY             DEFAULT VALUE           */

    /* general actions */
    "PrevTrack",       "xmms --rew",
    "Play",            "xmms --play",
    "Stop",            "xmms --stop",
    "Pause",           "xmms --pause",
    "NextTrack",       "xmms --fwd",
    "WebBrowser",      "mozilla",
    "Email",           "mozilla -mail",
    "Calculator",      "xcalc",
    "FileManager",     "gmc",
    "MyComputer",      "gmc",
    "MyDocuments",     "gmc",
/*
    "FTPClient",       "gftp",
*/
    "NewsReader",      "mozilla -news",
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
    "osd_font",        "-*-lucidatypewriter-bold-r-normal-*-*-250-*-*-*-*-*-*",
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

    /* Clear the structures as per db2's documentation */
    memset( &k, 0, sizeof(k) );
    memset( &data, 0, sizeof(data) );

    k.data = key;
    k.size = strlen(key) + 1;
    ;
    if ( dbp->get(dbp, NULL, &k, &data, 0) == 0 ) {
        return data.data;
    }
    else
        return NULL;
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
        fillDefaults();
    }
}


void
readConfigFile(void)
{
    char*   h;
    char    filename[MAXPATHLEN];

    /* Create the hash table */
    if ( db_open(NULL, DB_HASH, DB_CREATE, 0664, NULL, NULL, &dbp) != 0 )
    {
        uError("Can't create hash table: %s", strerror(errno));
        bailout();
    }

    /* See whether the user has his own config file */
    if ( (h = getenv("HOME")) != NULL )
    {
        strncpy( filename, h, MAXPATHLEN-2-strlen(PACKAGE)-1-strlen(CONFIG_NAME)-1 );
        strcat( filename, "/." PACKAGE "/" );
        strcat( filename, CONFIG_NAME );
        if ( testReadable(filename) )
        {
            parseConfigFile(filename);
            return;
        }
    }

    strncpy( filename, CONFDIR, MAXPATHLEN-strlen(CONFIG_NAME)-2 );
    strcat( filename, "/" );
    strcat( filename, CONFIG_NAME );
    if ( testReadable(filename) )
    {
        parseConfigFile(filename);
        return;
    }
    else
        fillDefaults();
}
