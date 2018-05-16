#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <stdlib.h>
#include <string.h>

#include <time.h>



typedef unsigned int clock_time_t;

struct timer
{
	clock_time_t start;
	clock_time_t interval;
};  




clock_time_t clock_time(void);
void timers_init(void);
void timer_next_event(struct timeval *tv);
void timer_run(void);
int timer_add_ms(char *name, unsigned int ms, void (*function)(void *), void *arg);
int timer_add_ms_unique(int tid, char *name, unsigned int ms, void (*function)(void *), void *arg);
int timer_add(int tid, char *name, struct timeval when, void (*function)(void *), void *arg);
void timer_set(struct timer *t, clock_time_t interval);
void timer_reset(struct timer *t);
void timer_restart(struct timer *t);
int timer_expired(struct timer *t);
int timer_lost(struct timer *t);
int timer_elapsed(struct timer *t);
