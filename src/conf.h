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

#ifndef __CONF_H
#define	__CONF_H

#define MAX_KEY_LEN     32
#define MAX_VALUE_LEN   1024    /* if someone provides a value that's
                                   more than 1024 chars then he must be
                                   crazy, and deserves to be ignored */

char* getConfig(char* key);
int   setConfig(char* key, char* value, u_int32_t flags);

#endif /* __CONF_H */
