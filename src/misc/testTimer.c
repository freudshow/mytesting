#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "basedef.h"

typedef enum {
    FALSE, TRUE
} Boolean;

static volatile sig_atomic_t gotAlarm = 0;
/* Set nonzero on receipt of SIGALRM */

/* Retrieve and display the real time, and (if 'includeTimer' is
 TRUE) the current value and interval for the ITIMER_REAL timer */
static void displayTimes(const char *msg, Boolean includeTimer)
{
    struct itimerval itv;
    static struct timeval start;
    struct timeval curr;
    static int callNum = 0; /* Number of calls to this function */

    if (callNum == 0) /* Initialize elapsed time meter */
        if (gettimeofday(&start, NULL) == -1)
            exit(1);

    if (callNum % 20 == 0) /* Print header every 20 lines */
        DEBUG_TIME_LINE("       Elapsed   Value Interval\n");

    if (gettimeofday(&curr, NULL) == -1)
        exit(1);

    DEBUG_TIME_LINE("%-7s %6.2f", msg, curr.tv_sec - start.tv_sec + (curr.tv_usec - start.tv_usec) / 1000000.0);

    if (includeTimer)
    {
        if (getitimer(ITIMER_REAL, &itv) == -1)
            exit(1);

        DEBUG_TIME_LINE("  %6.2f  %6.2f",
                itv.it_value.tv_sec + itv.it_value.tv_usec / 1000000.0,
                itv.it_interval.tv_sec + itv.it_interval.tv_usec / 1000000.0);
    }

    callNum++;
}

static void sigalrmHandler(int sig)
{
    gotAlarm = 1;
    DEBUG_TIME_LINE("<<<<<<<<<<<<<<<<<<<<<<<------------------------>>>>>>>>>>>>>>>>>>>");
}

void testTimer()
{
    struct itimerval itv;
    clock_t prevClock;
    int maxSigs; /* Number of signals to catch before exiting */
    int sigCnt; /* Number of signals so far caught */

    sigCnt = 0;

//    sigemptyset(&sa.sa_mask);
//    sa.sa_flags = 0;
//    sa.sa_handler = sigalrmHandler;
//    if (sigaction(SIGALRM, &sa, NULL) == -1)
//        exit(1);

    signal(SIGALRM, sigalrmHandler);

    /* Set timer from the command-line arguments */

    itv.it_value.tv_sec = 2;
    itv.it_value.tv_usec = 0;
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 0;

    /* Exit after 3 signals, or on first signal if interval is 0 */

    maxSigs = 1;

    displayTimes("START:", FALSE);
    if (setitimer(ITIMER_REAL, &itv, NULL) == -1)
        exit(1);

    prevClock = clock();
    sigCnt = 0;

    for (;;)
    {

        /* Inner loop consumes at least 0.5 seconds CPU time */

        while (((clock() - prevClock) * 10 / CLOCKS_PER_SEC) < 5)
        {
            if (gotAlarm)
            { /* Did we get a signal? */
                gotAlarm = 0;
                displayTimes("ALARM:", TRUE);

                sigCnt++;
                if (sigCnt >= maxSigs)
                {
                    DEBUG_TIME_LINE("That's all folks\n");
                    exit(EXIT_SUCCESS);
                }
            }
        }

        prevClock = clock();
        displayTimes("Main: ", TRUE);
    }
}
