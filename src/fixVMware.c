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

#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "hotkeys.h"

unsigned int interval;

static void
fixIt(int sig)
{
    initializeX("fix vmware");
    XFlush(dpy);
}

static void
startFixVMwareThread(void)
{
    struct sigaction s;
    sigset_t    set;

    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    pthread_sigmask( SIG_UNBLOCK, &set, NULL );

    bzero(&s, sizeof(s));
    s.sa_handler = fixIt;
    s.sa_flags = SA_RESTART;

    sigaction( SIGALRM, &s, NULL);

    while (1)
    {
        alarm(interval);
        pause();
    }
}

void
fixVMware(char* time)
{
    pthread_t       tp;
    pthread_attr_t  attr;

    interval = ( time == NULL ? 10 : atoi(time) );

    /* LinuxThread defines that we must block the signal in all
     * threads, so we block it before creating any threads */
    sigblock(sigmask(SIGALRM));

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_create( &tp, &attr, startFixVMwareThread, NULL );
}
