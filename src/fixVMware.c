#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "hotkeys.h"

unsigned int interval;

static void
fixIt(int sig)
{
    printf("hahaha\n");
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
printf("%d\n",interval);
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
