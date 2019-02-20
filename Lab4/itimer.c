/* itimer.c program */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

/*************************
 struct timeval {
    time_t      tv_sec;         // seconds 
    suseconds_t tv_usec;        // microseconds 
 };
 struct itimerval {
    struct timeval it_interval; // Interval of periodic timer 
    struct timeval it_value;    // Time until next expiration
 };
*********************/

int hh, mm, ss, tick;

void timer_handler (int sig)
{
   printf("timer_handler: signal=%d\n", sig);
}

int main ()
{
 struct itimerval itimer;
 tick = hh = mm = ss = 0;
 
 signal(SIGALRM, &timer_handler);
 
 /* Configure the timer to expire after 1 sec */
 timer.it_value.tv_sec  = 1;
 timer.it_value.tv_usec = 0;

 /* and every 1 sec after that */
 timer.it_interval.tv_sec  = 1;
 timer.it_interval.tv_usec = 0;

 setitimer (ITIMER_REAL, &timer, NULL);

 while (1);
}