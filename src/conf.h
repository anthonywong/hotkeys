/* $Id$ */

#ifndef __CONF_H
#define	__CONF_H

#define MAX_KEY_LEN     32
#define MAX_VALUE_LEN   1024    /* if someone provides a value that's
                                   more than 1024 chars then he must be
                                   crazy, and deserves to be ignored */

char* getConfig(char* key);
int   setConfig(char* key, char* value, u_int32_t flags);

#endif /* __CONF_H */
